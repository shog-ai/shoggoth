/**** client.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#define _GNU_SOURCE

#include "../../sonic.h"

#include "../../include/client.h"
#include "../../include/utils.h"

#include <arpa/inet.h>
#include <assert.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdlib.h>
#include <unistd.h>

#include <netlibc/mem.h>

/****U
 * creates a new client http request instance
 *
 ****/
sonic_client_request_t *new_client_request(sonic_method_t method, char *url) {
  sonic_client_request_t *req = ncalloc(1, sizeof(sonic_client_request_t));

  req->headers = NULL;
  req->headers_count = 0;

  char scheme[10];
  extract_scheme_from_url(url, scheme);

  if (strcmp(scheme, "http") == 0) {
    req->scheme = SCHEME_HTTP;
  } else if (strcmp(scheme, "https") == 0) {
    req->scheme = SCHEME_HTTPS;
  } else {
    nfree(req);

    client_set_errno(CLIENT_ERROR_INVALID_SCHEME);
    return NULL;
  }

  char host[256];
  extract_host_from_url(url, host);

  char hostname[256];
  u16 port = 0;
  extract_hostname_and_port_from_host(host, req->scheme, hostname, &port);

  char path[256];
  extract_path_from_url(url, path);

  req->host = nmalloc((strlen(hostname) + 1) * sizeof(char));
  strcpy(req->host, hostname);

  req->domain_name = nmalloc((strlen(req->host) + 1) * sizeof(char));
  strcpy(req->domain_name, req->host);

  req->port = port;

  req->path = nmalloc((strlen(path) + 1) * sizeof(char));
  strcpy(req->path, path);

  req->method = method;

  req->request_body = NULL;
  req->request_body_size = 0;

  req->use_response_callback = false;
  req->response_callback_func = NULL;
  req->response_callback_user_pointer = NULL;

  sonic_redirect_policy_t default_redirect_policy = {0};
  default_redirect_policy.max_hops = 10;
  default_redirect_policy.no_redirect = false;

  req->redirect_policy = default_redirect_policy;
  req->redirect_hops_count = 0;

  return req;
}

void client_set_response_callback(sonic_client_request_t *req,
                                  response_callback_func_t callback_func,
                                  void *user_pointer) {
  req->use_response_callback = true;

  req->response_callback_user_pointer = user_pointer;
  req->response_callback_func = callback_func;
}

/****
 * frees memory used by a http request instance
 *
 ****/
void client_free_request(sonic_client_request_t *req) {
  nfree(req->host);
  nfree(req->path);
  nfree(req->domain_name);

  if (req->headers_count > 0) {
    for (u64 i = 0; i < req->headers_count; i++) {
      nfree(req->headers[i].key);
      nfree(req->headers[i].value);
    }

    nfree(req->headers);
  }

  nfree(req);
}

/****
 * frees memory used by a http response instance
 *
 ****/
void client_free_response(sonic_client_response_t *resp) {
  if (resp->failed) {
    nfree(resp->error);
  }

  if (resp->headers_count > 0) {
    for (u64 i = 0; i < resp->headers_count; i++) {
      nfree(resp->headers[i].key);
      nfree(resp->headers[i].value);
    }

    nfree(resp->headers);
  }

  nfree(resp);
}

void client_request_set_body(sonic_client_request_t *req, char *request_body,
                             u64 request_body_size) {
  req->request_body = request_body;
  req->request_body_size = request_body_size;
}

void client_set_redirect_policy(sonic_client_request_t *req,
                                sonic_redirect_policy_t policy) {
  req->redirect_policy = policy;
}

/****U
 * creates a new client http response instance
 *
 ****/
sonic_client_response_t *new_client_response() {
  sonic_client_response_t *resp = nmalloc(sizeof(sonic_client_response_t));

  resp->headers = NULL;
  resp->headers_count = 0;

  resp->response_body = NULL;
  resp->response_body_size = 0;

  resp->failed = false;
  resp->error = NULL;

  resp->should_redirect = false;
  resp->redirect_location = NULL;

  return resp;
}

/****U
 * extracts the status code from a buffer containing a http response head
 *
 ****/
int http_extract_status_code(const char *head, u16 *status_code) {
  char *statusLine =
      strstr(head, "HTTP/1.1 "); // Find the start of the status line
  if (statusLine == NULL) {
    return 1; // Status line not found
  }

  statusLine += strlen("HTTP/1.1 "); // Move past "HTTP/1.1 "
  char *endOfStatusCode =
      strchr(statusLine, ' '); // Find the space after the status code

  if (endOfStatusCode == NULL) {
    return 1; // Space after status code not found
  }

  // Calculate the length of the status code
  size_t statusCodeLength = endOfStatusCode - statusLine;

  char status_buffer[256];

  strncpy(status_buffer, statusLine, statusCodeLength);
  status_buffer[statusCodeLength] = '\0'; // Null-terminate the string

  *status_code = atoi(status_buffer);

  return 0;
}

/****U
 * given a string buffer containing a response head and a response struct, it
 * parses the headers and status code from the head into the response struct
 *
 ****/
void http_parse_response_head(sonic_client_response_t *resp,
                              char *head_buffer) {
  u16 status_code = 0;
  http_extract_status_code(head_buffer, &status_code);

  result_t res_status = number_to_status_code(status_code);
  resp->status = UNWRAP_INT(res_status);

  const char *header_start =
      strstr(head_buffer, "\r\n") + 2; // Skip the first line

  const char *header_end = strstr(header_start, "\r\n\r\n");

  const char *header_line = header_start;

  while (header_line < header_end) {
    const char *colon = strchr(header_line, ':');
    if (colon == NULL) {
      break; // Malformed header, exit loop
    }

    u64 key_len = (u64)(colon - header_line);
    char key[key_len + 1];
    strncpy(key, header_line, key_len);
    key[key_len] = '\0';

    const char *value_start = colon + 2; // Skip ": " after the key
    const char *value_end = strstr(value_start, "\r\n");

    u64 value_len = (u64)(value_end - value_start);
    char value[value_len + 1];
    strncpy(value, value_start, value_len);
    value[value_len] = '\0';

    resp->headers_count++;
    resp->headers =
        nrealloc(resp->headers, sizeof(sonic_header_t) * resp->headers_count);

    resp->headers[resp->headers_count - 1].key =
        nmalloc((strlen(key) + 1) * sizeof(char));
    strcpy(resp->headers[resp->headers_count - 1].key, key);

    resp->headers[resp->headers_count - 1].value =
        nmalloc((strlen(value) + 1) * sizeof(char));
    strcpy(resp->headers[resp->headers_count - 1].value, value);

    header_line = value_end + 2; // Move to the next header line
  }
}

typedef struct {
  sonic_client_response_t *resp;
  sonic_client_request_t *req;

  bool headers_done;
  u64 main_length;
  char *main_buffer;
  char *head_buffer;
  char *content_buffer;
  char *buffer;
  ssize_t bytes_read;
  bool is_chunked;
  u64 head_length;
  u64 content_buffer_size;
} read_struct_t;

void remove_chunk_headers(char *data, u64 *length) {
  char *src = data;
  char *dest = data;

  while (src < data + *length) {
    // Find the end of the chunk size line
    char *end = strstr(src, "\r\n");
    if (!end) {
      // No more valid chunks found
      break;
    }

    // Advance past the chunk size line and CRLF
    src = end + 2;

    // Find the end of the chunk data (next CRLF)
    char *next_crlf = strstr(src, "\r\n");
    if (!next_crlf) {
      // No more valid chunks found
      break;
    }

    // Calculate the chunk size
    size_t chunk_size = next_crlf - src;

    // Copy the chunk data
    memcpy(dest, src, chunk_size);
    dest += chunk_size;

    // Advance past the chunk data and its CRLF
    src = next_crlf + 2;
  }

  // Update the new length
  *length = dest - data;
}

result_t process_read_buffer(read_struct_t *read_struct) {
  if (!read_struct->headers_done) {
    if (read_struct->main_length == 0) {
      read_struct->main_buffer =
          nmalloc((read_struct->bytes_read) * sizeof(char));

      for (int i = 0; i < read_struct->bytes_read; i++) {
        read_struct->main_buffer[i] = read_struct->buffer[i];
      }

      read_struct->main_length = read_struct->bytes_read;
    } else {
      read_struct->main_buffer = nrealloc(
          read_struct->main_buffer,
          (read_struct->main_length + read_struct->bytes_read) * sizeof(char));

      for (int i = 0; i < read_struct->bytes_read; i++) {
        read_struct->main_buffer[read_struct->main_length + i] =
            read_struct->buffer[i];
      }

      read_struct->main_length += read_struct->bytes_read;
    }

    char *head_end_identifier = "\r\n\r\n";

    const char *found_head_end =
        memmem(read_struct->main_buffer, read_struct->main_length,
               head_end_identifier, strlen(head_end_identifier));

    if (found_head_end != NULL) {
      u64 head_end_position = found_head_end - read_struct->main_buffer + 4;

      // printf("HEAD END POS: %llu \n", head_end_position);

      read_struct->head_length = head_end_position;
      read_struct->head_buffer =
          nmalloc((read_struct->head_length + 1) * sizeof(char));

      for (u64 i = 0; i < (read_struct->head_length); i++) {
        read_struct->head_buffer[i] = read_struct->main_buffer[i];
      }

      read_struct->head_buffer[read_struct->head_length] = '\0';

      read_struct->headers_done = true;

      http_parse_response_head(read_struct->resp, read_struct->head_buffer);

      // FIXME: properly handle chunked responses to acheive HTTP/1.1

      if (!read_struct->req->redirect_policy.no_redirect) {
        if (read_struct->resp->status == STATUS_302 ||
            read_struct->resp->status == STATUS_301) {

          char *location = utils_get_header_value(
              read_struct->resp->headers, read_struct->resp->headers_count,
              "Location");

          if (location == NULL) {
            return ERR("ERROR: no location header in redirect response");
          }

          read_struct->resp->should_redirect = true;
          read_struct->resp->redirect_location = strdup(location);
        }
      }

      int remaining_bytes = read_struct->main_length - read_struct->head_length;

      // copy if any, the remaining data in main buffer that are not
      // part of head to content
      // FIXME: you should limit the maximum amount of content to avoid DOS

      if (remaining_bytes > 0) {
        if (!read_struct->resp->should_redirect) {
          if (read_struct->req->use_response_callback) {
            read_struct->req->response_callback_func(
                &read_struct->main_buffer[read_struct->head_length],
                remaining_bytes,
                read_struct->req->response_callback_user_pointer);
          } else {

            read_struct->content_buffer_size = remaining_bytes;

            read_struct->content_buffer =
                nrealloc(read_struct->content_buffer,
                         (read_struct->content_buffer_size) * sizeof(char));

            for (u64 i = 0; i < read_struct->content_buffer_size; i++) {
              read_struct->content_buffer[i] =
                  read_struct->main_buffer[(read_struct->head_length) + i];
            }
          }
        }
      }
    }

  } else {
    if (!read_struct->resp->should_redirect) {
      if (read_struct->req->use_response_callback) {
        read_struct->req->response_callback_func(
            read_struct->buffer, read_struct->bytes_read,
            read_struct->req->response_callback_user_pointer);
      } else {
        read_struct->content_buffer_size += read_struct->bytes_read;

        read_struct->content_buffer =
            nrealloc(read_struct->content_buffer,
                     (read_struct->content_buffer_size) * sizeof(char));

        for (int i = 0; i < read_struct->bytes_read; i++) {
          read_struct->content_buffer[read_struct->content_buffer_size -
                                      read_struct->bytes_read + i] =
              read_struct->buffer[i];
        }
      }
    }
  }

  return OK_BOOL(false);
}

result_t read_response(sonic_client_request_t *req,
                       sonic_client_response_t *resp, SSL *ssl,
                       int client_sock) {

  char buffer[2048];

  read_struct_t read_struct = {0};
  read_struct.req = req;
  read_struct.resp = resp;

  if (req->scheme == SCHEME_HTTP) {
    while ((read_struct.bytes_read =
                recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {

      read_struct.buffer =
          nrealloc(read_struct.buffer, read_struct.bytes_read * sizeof(char));
      for (int i = 0; i < read_struct.bytes_read; i++) {
        read_struct.buffer[i] = buffer[i];
      }

      result_t res_should_break = process_read_buffer(&read_struct);
      bool should_break = false;

      if (is_err(res_should_break)) {
        should_break = true;
      } else {
        should_break = res_should_break.ok_bool;
      }

      free_result(res_should_break);

      if (should_break) {
        break;
      }
    }
  } else {
    while ((read_struct.bytes_read = SSL_read(ssl, buffer, sizeof(buffer))) >
           0) {

      read_struct.buffer =
          nrealloc(read_struct.buffer, read_struct.bytes_read * sizeof(char));
      for (int i = 0; i < read_struct.bytes_read; i++) {
        read_struct.buffer[i] = buffer[i];
      }

      result_t res_should_break = process_read_buffer(&read_struct);
      bool should_break = false;

      if (is_err(res_should_break)) {
        should_break = true;
      } else {
        should_break = res_should_break.ok_bool;
      }

      free_result(res_should_break);

      if (should_break) {
        break;
      }
    }
  }

  // FIXME: properly handle chunked responses to acheive HTTP/1.1

  *req = *read_struct.req;
  *resp = *read_struct.resp;

  if (read_struct.bytes_read == -1) {
    perror("SONIC: Receiving response failed");
    close(client_sock);
    return ERR("Receiving response failed");
  }

  // Close the client socket.
  close(client_sock);

  if (!read_struct.req->use_response_callback) {
    resp->response_body = read_struct.content_buffer;
    resp->response_body_size = read_struct.content_buffer_size;
  }

  nfree(read_struct.main_buffer);
  nfree(read_struct.head_buffer);
  nfree(read_struct.buffer);

  return OK(NULL);
}

// Function to initialize OpenSSL and create an SSL context
SSL_CTX *init_openssl() {
  SSL_CTX *ctx;

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ctx = SSL_CTX_new(TLS_client_method());
  if (ctx == NULL) {
    printf("ERROR WHILE INIT OPENSSL \n");
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  return ctx;
}

// Function to establish an SSL connection
result_t create_ssl_connection(int client_sock, SSL_CTX *ctx, char *hostname) {
  SSL *ssl;
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, client_sock);

  SSL_set_tlsext_host_name(ssl, hostname);

  if (SSL_connect(ssl) == -1) {
    printf("ERROR WHILE CREATING SSL CONNECTION \n");
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(client_sock);
    return ERR("ERROR WHILE CREATING SSL CONNECTION");
  }

  return OK(ssl);
}

void free_ssl(SSL *ssl, SSL_CTX *ssl_ctx) {
  // Clean up
  SSL_free(ssl);
  SSL_CTX_free(ssl_ctx);
}

// Function to send an HTTP request over an SSL connection
int send_ssl(SSL *ssl, const char *request_str, size_t request_len) {
  if (SSL_write(ssl, request_str, request_len) <= 0) {
    printf("ERROR WHILE SEND SSL \n");
    ERR_print_errors_fp(stderr);
    return -1;
  }
  return 0;
}

/****
 * sends the constructed request to the server using the connected socket
 *
 ****/
result_t actually_send_request(sonic_client_request_t *req,

                               struct sockaddr_in server_addr) {

  sonic_client_response_t *resp = new_client_response();

  char content_length_str[32];
  sprintf(content_length_str, U64_FORMAT_SPECIFIER, req->request_body_size);
  client_request_add_header(req, "Content-Length", content_length_str);

  client_request_add_header(req, "Accept", "*/*");
  client_request_add_header(req, "Accept-Encoding", "identity");
  client_request_add_header(req, "User-Agent", "Sonic");
  client_request_add_header(req, "Connection", "close");
  client_request_add_header(req, "Host", req->domain_name);

  result_t res_method_str = http_method_to_str(req->method);
  char *method_str = PROPAGATE(res_method_str);

  char *head_str = NULL;

  char first_line[1024];
  // FIXME: uses HTTP/1.0 instead of HTTP/1.1 to avoid chunked responses which
  // are not implemented yet
  sprintf(first_line, "%s %s HTTP/1.0\r\n", method_str, req->path);

  head_str = nrealloc(head_str, (strlen(first_line) + 1) * sizeof(char));
  strcpy(head_str, first_line);

  for (u64 i = 0; i < req->headers_count; i++) {
    char *buf = nmalloc(
        (strlen(req->headers[i].key) + strlen(req->headers[i].value) + 20) *
        sizeof(char));

    sprintf(buf, "%s: %s\r\n", req->headers[i].key, req->headers[i].value);

    head_str =
        nrealloc(head_str, (strlen(head_str) + strlen(buf) + 1) * sizeof(char));
    strcat(head_str, buf);

    nfree(buf);
  }

  char *head_end = "\r\n";

  head_str = nrealloc(head_str,
                      (strlen(head_str) + strlen(head_end) + 1) * sizeof(char));
  strcat(head_str, head_end);

  if (req->scheme == SCHEME_HTTP) {
    int client_sock;

    // Create a socket for the client.
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
      return ERR("Socket creation failed");
    }

    // Connect to the server.
    if (connect(client_sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
      close(client_sock);

      return ERR("Connection failed");
    }

    if (send(client_sock, head_str, strlen(head_str), 0) == -1) {
      perror("SONIC: Sending request head failed");
      close(client_sock);
      return ERR("Sending request head failed");
    }

    nfree(head_str);

    if (req->request_body_size > 0) {
      if (send(client_sock, req->request_body, req->request_body_size, 0) ==
          -1) {
        perror("Sending request content failed");
        close(client_sock);
        return ERR("Sending request content failed");
      }
    }

    result_t res_read = read_response(req, resp, NULL, client_sock);
    void *val = PROPAGATE(res_read);
    val = (void *)val;
  } else {
    // Implement HTTPS using OpenSSL in this block
    // Initialize OpenSSL and create an SSL context
    SSL_CTX *ssl_ctx = init_openssl();

    // Create a socket for the client
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
      perror("Socket creation failed");
      return ERR("Socket creation failed");
    }

    // Connect to the server
    if (connect(client_sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {
      perror("Connection failed");
      close(client_sock);
      return ERR("Connection failed");
    }

    // Establish the SSL connection
    result_t res_ssl =
        create_ssl_connection(client_sock, ssl_ctx, req->domain_name);
    SSL *ssl = PROPAGATE(res_ssl);

    if (send_ssl(ssl, head_str, strlen(head_str)) == -1) {
      perror("Sending request head failed");
      close(client_sock);
      return ERR("Sending request head failed");
    }

    nfree(head_str);

    if (req->request_body_size > 0) {
      // Send the HTTPS request
      if (send_ssl(ssl, req->request_body, req->request_body_size) == -1) {
        perror("Sending request failed");
        SSL_free(ssl);
        close(client_sock);
        return ERR("Sending request failed");
      }
    }

    result_t res_read = read_response(req, resp, ssl, client_sock);

    free_ssl(ssl, ssl_ctx);
    void *val = PROPAGATE(res_read);
    val = (void *)val;
  }

  return OK(resp);
}

/****U
 * sets a response as failed
 *
 ****/
sonic_client_response_t *fail_response(char *error) {
  sonic_client_response_t *resp = new_client_response();

  resp->failed = true;
  resp->error = strdup(error);

  return resp;
}

/****
 * sends a http request
 *
 ****/
sonic_client_response_t *client_send_request(sonic_client_request_t *req) {
  struct sockaddr_in server_addr;

  char server_ip[256] = {0};

  if (is_ip_address(req->host)) {
    strcpy(server_ip, req->host);
  } else {
    int failed = resolve_domain_to_ip(req->host, server_ip, sizeof(server_ip));

    if (failed != 0) {
      return fail_response("Domain resolution failed \n");
    }
  }

  assert(server_ip != NULL);

  // Configure server address.
  server_addr.sin_family = AF_INET;
  inet_pton(AF_INET, server_ip, &(server_addr.sin_addr));
  server_addr.sin_port = htons(req->port);

  // Send the HTTP request.

  result_t res_resp = actually_send_request(req, server_addr);
  sonic_client_response_t *resp = NULL;

  if (is_err(res_resp)) {
    resp = fail_response(res_resp.error_message);
  } else {
    resp = VALUE(res_resp);
  }

  free_result(res_resp);

  assert(resp != NULL);

  if (resp->should_redirect) {
    if (req->redirect_hops_count < req->redirect_policy.max_hops) {
      req->redirect_hops_count++;

      sonic_redirect_policy_t redirect_policy = req->redirect_policy;
      u64 redirect_hops_count = req->redirect_hops_count;
      bool use_response_callback = req->use_response_callback;
      response_callback_func_t response_callback_func =
          req->response_callback_func;
      void *response_callback_user_pointer =
          req->response_callback_user_pointer;
      char *redirect_location = resp->redirect_location;

      if (resp->response_body_size > 0) {
        nfree(resp->response_body);
      }
      sonic_free_response(resp);

      sonic_client_request_t *new_req =
          new_client_request(METHOD_GET, redirect_location);

      // The new request needs to inherit some properties from the parent
      new_req->redirect_policy = redirect_policy;
      new_req->redirect_hops_count = redirect_hops_count;
      new_req->use_response_callback = use_response_callback;
      new_req->response_callback_func = response_callback_func;
      new_req->response_callback_user_pointer = response_callback_user_pointer;

      sonic_client_response_t *new_resp = client_send_request(new_req);
      sonic_free_request(new_req);

      nfree(redirect_location);

      resp = new_resp;

    } else {
      char message[256];
      sprintf(message,
              "Maximum redirect hops of " U64_FORMAT_SPECIFIER " reached",
              req->redirect_policy.max_hops);

      nfree(resp->redirect_location);
      sonic_free_response(resp);

      return fail_response(message);
    }
  }

  return resp;
}

/****U
 * adds a header to a http request
 *
 ****/
void client_request_add_header(sonic_client_request_t *req, char *key,
                               char *value) {
  sonic_header_t new_header;

  new_header.key = strdup(key);

  new_header.value = strdup(value);

  if (req->headers_count == 0) {
    req->headers = nmalloc((req->headers_count + 1) * sizeof(sonic_header_t));
  } else {
    req->headers = nrealloc(req->headers,
                            (req->headers_count + 1) * sizeof(sonic_header_t));
  }

  req->headers[req->headers_count] = new_header;

  req->headers_count++;
}
