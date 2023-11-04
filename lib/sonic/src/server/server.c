/**** server.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#define _GNU_SOURCE

#include "../../include/server.h"
#include "../../include/utils.h"
#include "rate_limiter.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

/****
 * frees a response and all its used memory
 *
 ****/
void free_server_response(sonic_server_response_t *resp) {
  if (resp->headers_count > 0) {
    for (u64 i = 0; i < resp->headers_count; i++) {
      free(resp->headers[i].key);
      free(resp->headers[i].value);
    }

    free(resp->headers);
  }

  free(resp);
}

/****
 * sends a response to the client
 *
 ****/
void server_send_response(sonic_server_request_t *req,
                          sonic_server_response_t *resp) {
  char *content_type = content_type_to_string(resp->content_type);
  char *status_code = utils_status_code_to_string(resp->status);

  char content_length[20];
  sprintf(content_length, U64_FORMAT_SPECIFIER, resp->response_body_size);

  server_response_add_header(resp, "Server", "Sonic");
  server_response_add_header(resp, "Content-Type", content_type);
  server_response_add_header(resp, "Content-Length", content_length);

  char *head_str = NULL;

  char first_line[1024];
  sprintf(first_line, "HTTP/1.1 %s\r\n", status_code);

  head_str = realloc(head_str, (strlen(first_line) + 1) * sizeof(char));
  strcpy(head_str, first_line);

  for (u64 i = 0; i < resp->headers_count; i++) {
    char *buf = malloc(
        (strlen(resp->headers[i].key) + strlen(resp->headers[i].value) + 20) *
        sizeof(char));

    sprintf(buf, "%s: %s\r\n", resp->headers[i].key, resp->headers[i].value);

    head_str =
        realloc(head_str, (strlen(head_str) + strlen(buf) + 1) * sizeof(char));
    strcat(head_str, buf);

    free(buf);
  }

  char *head_end = "\r\n";

  head_str = realloc(head_str,
                     (strlen(head_str) + strlen(head_end) + 1) * sizeof(char));
  strcat(head_str, head_end);

  send(req->client_sock, head_str, strlen(head_str), 0);
  free(head_str);

  send(req->client_sock, resp->response_body, resp->response_body_size, 0);
}

void not_found_route(sonic_server_request_t *req) {
  char *resp_body = "404 NOT FOUND";
  sonic_server_response_t *resp =
      new_server_response(STATUS_404, MIME_TEXT_PLAIN);
  server_response_set_body(resp, resp_body, strlen(resp_body));

  server_send_response(req, resp);

  free_server_response(resp);
}

void free_path(sonic_path_t *path) {
  for (u64 i = 0; i < path->segments_count; i++) {
    free(path->segments[i].value);
  }

  free(path->segments);
  free(path);
}

result_t raw_path_to_sonic_path(char *input_path) {
  char *raw_path = strdup(input_path);

  // FIXME: run checks on raw_path to ensure there are no malicious
  // segments attempting "Path Traversal" attacks

  if (raw_path == NULL) {
    return ERR("raw path is NULL");
  }

  sonic_path_t *path = (sonic_path_t *)calloc(1, sizeof(sonic_path_t));
  path->segments = NULL;
  path->segments_count = 0;
  path->has_wildcard = false;

  char *saveptr; // For strtok_r

  char *token =
      strtok_r(raw_path, "/", &saveptr); // Use strtok_r with the saveptr

  u64 count = 0;

  while (token != NULL) {
    path->segments = (sonic_path_segment_t *)realloc(
        path->segments, (count + 1) * sizeof(sonic_path_segment_t));

    path->segments[count].is_generic = false;
    path->segments[count].is_wildcard = false;

    path->segments[count].value = strdup(token);

    if (path->segments[count].value[0] == '{' &&
        path->segments[count]
                .value[(strlen(path->segments[count].value) - 1)] == '}') {

      char *new_value =
          malloc((strlen(path->segments[count].value) - 1) * sizeof(char));

      for (u64 w = 1; w < (strlen(path->segments[count].value) - 1); w++) {
        new_value[w - 1] = path->segments[count].value[w];
      }

      new_value[strlen(path->segments[count].value) - 2] = '\0';

      free(path->segments[count].value);
      path->segments[count].value = new_value;

      path->segments[count].is_generic = true;
    } else {
      path->segments[count].is_generic = false;
    }

    if (path->segments[count].is_generic &&
        strlen(path->segments[count].value) == 1) {
      if (path->segments[count].value[0] == '*') {
        path->segments[count].is_wildcard = true;
        path->has_wildcard = true;
        path->wildcard_index = count;
      }
    }

    token = strtok_r(NULL, "/", &saveptr); // Use strtok_r with the saveptr

    count++;

    // wildcard segments must be the last segment
    // therefore break once one is found
    if (path->has_wildcard) {
      break;
    }
  }

  path->segments_count = count;

  free(raw_path);

  return OK(path);
}

char *server_get_path_segment(sonic_server_request_t *req, char *segment_name) {
  for (u64 i = 0; i < req->matched_route->path->segments_count; i++) {
    if (req->matched_route->path->segments[i].is_generic) {
      if (strcmp(segment_name, req->matched_route->path->segments[i].value) ==
          0) {

        return req->path->segments[i].value;
      }
    }
  }

  return NULL;
}

sonic_wildcard_segments_t
server_get_path_wildcard_segments(sonic_server_request_t *req) {
  char **segments = NULL;
  u64 count = 0;

  for (u64 i = req->matched_route->path->wildcard_index;
       i < req->path->segments_count; i++) {
    segments = realloc(segments, (count + 1) * sizeof(char *));

    segments[count] = req->path->segments[i].value;

    count++;
  }

  sonic_wildcard_segments_t result = {.segments = segments, .count = count};

  return result;
}

void server_free_path_wildcard_segments(sonic_wildcard_segments_t segments) {
  free(segments.segments);
}

bool paths_match(sonic_path_t *left, sonic_path_t *right) {
  // NOTE: left is route path and right is request path

  bool match = true;

  if (left->segments_count == 0 && right->segments_count == 0) {
    match = true;
    return match;
  }

  if (!left->has_wildcard) {
    if (left->segments_count != right->segments_count) {
      match = false;
      return match;
    }
  } else {
    if (left->segments_count > right->segments_count) {
      match = false;
      return match;
    }
  }

  for (u64 i = 0; i < left->segments_count; i++) {
    sonic_path_segment_t left_seg = left->segments[i];
    sonic_path_segment_t right_seg = right->segments[i];

    if (!left_seg.is_generic && !right_seg.is_generic) {
      if (strcmp(left_seg.value, right_seg.value) != 0) {
        match = false;
        break;
      }
    } else {
      if (left_seg.is_generic && !right_seg.is_generic) {
        if (left_seg.is_wildcard) {
          break;
        }

        continue;
      } else if (right_seg.is_generic) {
        // the right seg which is from the request should never be generic,
        // hence unmatching
        match = false;
        break;
      }
    }
  }

  return match;
}

sonic_middleware_response_t *
new_server_middleware_response(bool should_respond,
                               sonic_server_response_t *response) {

  if (should_respond) {
    assert(response != NULL);
  }

  sonic_middleware_response_t *resp =
      malloc(sizeof(sonic_middleware_response_t));

  resp->should_respond = should_respond;
  resp->middleware_response = response;

  return resp;
}

void free_server_middleware_response(sonic_middleware_response_t *resp) {
  if (resp->should_respond) {
    free_server_response(resp->middleware_response);
  }

  free(resp);
}

/****
 * processes a http request
 *
 ****/
void process_request(sonic_server_t *server, sonic_server_request_t *req) {
  bool should_continue = true;

  for (u64 i = 0; i < server->middlewares_count; i++) {
    sonic_middleware_response_t *middleware_resp =
        server->middlewares[i]->middleware_func(
            req, server->middlewares[i]->user_pointer);

    if (middleware_resp->should_respond) {
      should_continue = false;
      server_send_response(req, middleware_resp->middleware_response);
      break;
    }

    free_server_middleware_response(middleware_resp);
  }

  if (!should_continue) {
    return;
  }

  // to prevent path traversal
  for (u64 i = 0; i < req->path->segments_count; i++) {
    if (strcmp(req->path->segments[i].value, "..") == 0 ||
        strcmp(req->path->segments[i].value, ".") == 0) {
      char *body = "path must not contain \"..\" ";

      sonic_server_response_t *resp =
          new_server_response(STATUS_400, MIME_TEXT_PLAIN);
      server_response_set_body(resp, body, strlen(body));

      server_send_response(req, resp);
      free_server_response(resp);

      break;
    }
  }

  sonic_route_t *route = NULL;

  for (u64 i = 0; i < server->routes_count; i++) {
    sonic_route_t *current_route = server->routes[i];

    sonic_path_t *current_route_path = current_route->path;

    bool match = paths_match(current_route_path, req->path);

    if (match && current_route->method == req->method) {
      route = current_route;

      break;
    }
  }

  req->matched_route = route;

  if (route != NULL) {
    bool should_run_route = true;

    for (u64 w = 0; w < route->middlewares_count; w++) {
      sonic_middleware_response_t *middleware_resp =
          route->middlewares[w]->middleware_func(
              req, route->middlewares[w]->user_pointer);

      if (middleware_resp->should_respond) {
        should_run_route = false;
        server_send_response(req, middleware_resp->middleware_response);
        break;
      }

      free_server_middleware_response(middleware_resp);
    }

    if (should_run_route) {
      if (route->is_file_route) {
        route->file_route_func(req, route->file_route_path,
                               route->file_route_content_type);

      } else {
        route->route_func(req);
      }
    }
  } else {
    not_found_route(req);
  }
}

/****
 * a route handler that serves a file
 *
 ****/
void serve_file(sonic_server_request_t *req, char *file_path,
                sonic_content_type_t content_type) {

  int fd;
  struct stat file_info;
  char *file_contents;

  // Open the file
  fd = open(file_path, O_RDONLY);
  if (fd == -1) {
    perror("Error opening file");
    exit(1);
  }

  // Get information about the file (e.g., its size)
  if (fstat(fd, &file_info) == -1) {
    perror("Error getting file information");
    close(fd);
    exit(1);
  }

  // Map the file into memory
  file_contents = mmap(0, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_contents == MAP_FAILED) {
    perror("Error mapping file to memory");
    close(fd);
    exit(1);
  }

  sonic_server_response_t *resp = new_server_response(STATUS_200, content_type);
  server_response_set_body(resp, file_contents, file_info.st_size);

  server_send_response(req, resp);

  free_server_response(resp);

  // Unmap the file when done
  munmap(file_contents, file_info.st_size);

  // Close the file
  close(fd);
}

/****
 * handler function for the root path of a served directory
 * lists all the files in the directory with links to each one
 *
 ****/
void directory_home_route(sonic_server_request_t *req) {
  req = (sonic_server_request_t *)req;

  sonic_server_response_t *resp =
      new_server_response(STATUS_200, MIME_TEXT_HTML);

  char *html = NULL;

  char *head = "<!DOCTYPE html>\n"
               "<html lang=\"en\">\n"
               "<head>\n"
               "<title>Files</title>\n"
               "<meta charset=\"utf-8\" />\n"
               "<meta name=\"viewport\" content=\"width=device-width, "
               "initial-scale=1\" />\n"

               "<style>\n"
               "body {\n"
               "  font-family: 'Lucida Console', monospace;\n"
               "}\n"
               "</style>\n"
               "</head>\n"

               "<body>\n";

  char *tail = "</body>\n</html>";

  char *body = "<h1>TODO: list files</h1>\n";

  html =
      malloc((strlen(head) + strlen(body) + strlen(tail) + 1) * sizeof(char));
  strcpy(html, head);
  strcat(html, body);
  strcat(html, tail);

  server_response_set_body(resp, html, strlen(html));

  server_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  server_send_response(req, resp);

  free_server_response(resp);
}

/****
 * adds a route that serves a directory
 *
 ****/
void server_add_directory_route(sonic_server_t *server, char *path,
                                char *directory_path) {
  sonic_files_list_t files_list = get_files_list_from_dir(directory_path);

  for (u64 i = 0; i < files_list.files_count; i++) {
    char *file_item_str = files_list.files[i];

    char relative_path[256];
    strcpy(relative_path, path);
    char *without_prefix = remove_prefix(directory_path, file_item_str);
    strcat(relative_path, without_prefix);
    free(without_prefix);

    char *file_extension = get_file_extension(relative_path);

    sonic_content_type_t content_type = MIME_APPLICATION_OCTET_STREAM;

    if (file_extension != NULL) {
      content_type = file_extension_to_content_type(file_extension);
    }

    server_add_file_route(server, relative_path, METHOD_GET, file_item_str,
                          content_type);
  }

  server_add_route(server, path, METHOD_GET, directory_home_route);

  for (u64 i = 0; i < files_list.files_count; i++) {
    free(files_list.files[i]);
  }

  free(files_list.files);
}

/****
 * adds a file route
 *
 ****/
void server_add_file_route(sonic_server_t *server, char *path,
                           sonic_method_t method, char *file_path,
                           sonic_content_type_t content_type) {
  sonic_route_t *new_route = server_add_route(server, path, method, NULL);

  new_route->is_file_route = true;
  new_route->file_route_func = serve_file;
  new_route->file_route_path = strdup(file_path);
  new_route->file_route_content_type = content_type;
}

/****
 * adds a route to a http server
 *
 ****/
sonic_route_t *server_add_route(sonic_server_t *server, char *path,
                                sonic_method_t method,
                                sonic_route_func_t route_func) {
  sonic_route_t *new_route = calloc(1, sizeof(sonic_route_t));

  new_route->is_file_route = false;

  result_t res_path = raw_path_to_sonic_path(path);
  new_route->path = res_path.ok_ptr;
  sonic_free_result(res_path);

  new_route->method = method;
  new_route->route_func = route_func;

  new_route->middlewares = NULL;
  new_route->middlewares_count = 0;

  server->routes = realloc(server->routes, (server->routes_count + 1) *
                                               sizeof(sonic_route_t *));
  server->routes[server->routes_count] = new_route;

  server->routes_count++;

  return new_route;
}

void server_add_route_middleware(sonic_route_t *route,
                                 sonic_middleware_func_t middleware_func,
                                 void *user_pointer) {

  sonic_route_middleware_t *new_middleware =
      malloc(sizeof(sonic_route_middleware_t));

  new_middleware->middleware_func = middleware_func;
  new_middleware->user_pointer = user_pointer;

  route->middlewares =
      realloc(route->middlewares, (route->middlewares_count + 1) *
                                      sizeof(sonic_route_middleware_t *));

  route->middlewares[route->middlewares_count] = new_middleware;

  route->middlewares_count++;
}

void server_add_server_middleware(sonic_server_t *server,
                                  sonic_middleware_func_t middleware_func,
                                  void *user_pointer) {

  sonic_route_middleware_t *new_middleware =
      malloc(sizeof(sonic_route_middleware_t));

  new_middleware->middleware_func = middleware_func;
  new_middleware->user_pointer = user_pointer;

  server->middlewares =
      realloc(server->middlewares, (server->middlewares_count + 1) *
                                       sizeof(sonic_route_middleware_t *));

  server->middlewares[server->middlewares_count] = new_middleware;

  server->middlewares_count++;
}

void server_add_rate_limiter_middleware(sonic_route_t *route,
                                        u64 requests_count, u64 duration,
                                        char *message) {
  rate_limiter_t *rate_limiter =
      new_rate_limiter(requests_count, duration, message);

  pthread_t rate_limiter_thread;
  if (pthread_create(&rate_limiter_thread, NULL, rate_limiter_background,
                     rate_limiter) != 0) {
    printf("SONIC ERROR: could not spawn rate limiter background thread");
  }

  sonic_add_middleware(route, rate_limiter_middleware_function, rate_limiter);
}

void server_add_server_rate_limiter_middleware(sonic_server_t *server,
                                               u64 requests_count, u64 duration,
                                               char *message) {

  rate_limiter_t *rate_limiter =
      new_rate_limiter(requests_count, duration, message);

  pthread_t rate_limiter_thread;
  if (pthread_create(&rate_limiter_thread, NULL, rate_limiter_background,
                     rate_limiter) != 0) {
    printf(
        "SONIC ERROR: could not spawn server rate limiter background thread");
  }

  sonic_add_server_middleware(server, rate_limiter_middleware_function,
                              rate_limiter);
}

/****
 * extracts the method from a buffer containing a http request head
 *
 ****/
void extract_request_method(const char *head_buffer, char request_method[]) {
  // Find the first space in the HTTP request header
  const char *space_ptr = strchr(head_buffer, ' ');

  if (space_ptr != NULL) {
    // Calculate the length of the request method
    size_t method_length = space_ptr - head_buffer;

    // Copy the request method to the output array
    strncpy(request_method, head_buffer, method_length);
    request_method[method_length] = '\0'; // Null-terminate the string
  } else {
    // No space found in the header; invalid HTTP request
    printf("EROR: Invalid HTTP request header.\n");
    exit(1);
  }
}

/****
 * extracts the path from a buffer containing a http request head
 *
 ****/
void extract_request_path(const char *head_buffer, char request_path[]) {
  // Find the start and end positions of the request path
  const char *start = strchr(head_buffer, ' '); // Find the first space
  if (start == NULL) {
    // If no space is found, return an empty string
    printf("ERROR: when parsing request path, no space was found in head \n");
    exit(1);
  }

  start++; // Move to the character after the first space

  const char *end = strchr(start, ' '); // Find the next space
  if (end == NULL) {
    // If no second space is found, return an empty string
    printf("ERROR: when parsing request path, no second space was found in "
           "head \n");
    exit(1);
  }

  // Calculate the length of the request path
  size_t length = end - start;

  // Copy the request path to the output string
  strncpy(request_path, start, length);
  request_path[length] = '\0';
}

/****
 * given a string buffer containing a request head and a request struct, it
 * parses the headers, method, and path from the head into the response struct
 *
 ****/
result_t parse_request_headers(sonic_server_request_t *req, char *head_buffer) {
  char request_method[64];
  extract_request_method(head_buffer, request_method);
  req->method = str_to_http_method(request_method);

  char request_path[256];
  extract_request_path(head_buffer, request_path);

  result_t res_path = raw_path_to_sonic_path(request_path);
  if (sonic_is_err(res_path)) {
    return res_path;
  }

  req->path = res_path.ok_ptr;
  sonic_free_result(res_path);

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

    req->headers_count++;
    req->headers =
        realloc(req->headers, sizeof(sonic_header_t) * req->headers_count);

    req->headers[req->headers_count - 1].key =
        malloc((strlen(key) + 1) * sizeof(char));
    strcpy(req->headers[req->headers_count - 1].key, key);

    req->headers[req->headers_count - 1].value =
        malloc((strlen(value) + 1) * sizeof(char));
    strcpy(req->headers[req->headers_count - 1].value, value);

    header_line = value_end + 2; // Move to the next header line
  }

  return OK(NULL);
}

/****
 * instantiates a new request struct
 *
 ****/
sonic_server_request_t *new_server_request() {
  sonic_server_request_t *req = malloc(sizeof(sonic_server_request_t));

  // Initialize fields to NULL or default values
  req->method = METHOD_GET;
  req->path = NULL;
  req->headers = NULL;
  req->headers_count = 0;

  req->request_body = NULL;
  req->request_body_size = 0;

  return req;
}

/****
 * frees a request and all its used memory
 *
 ****/
void free_server_request(sonic_server_request_t *req) {
  free(req->client_ip);

  free_path(req->path);

  if (req->headers_count > 0) {
    for (u64 i = 0; i < req->headers_count; i++) {
      free(req->headers[i].key);
      free(req->headers[i].value);
    }

    free(req->headers);
  }

  free(req);
}

typedef struct {
  sonic_server_t *http_server;
  int client_sock;
  struct sockaddr_in client_addr;
} handle_request_thread_args_t;

/****
 * handles a HTTP request. runs in a new separate thread
 *
 ****/
void *handle_request(void *thread_args) {
  pthread_detach(pthread_self());

  handle_request_thread_args_t *args =
      (handle_request_thread_args_t *)thread_args;

  sonic_server_request_t *req = new_server_request();

  bool headers_done = false;

  char *main_buffer = NULL;
  u64 main_length = 0;

  char *head_buffer = NULL;
  u64 head_length = 0;

  u64 content_length = 0;
  char *content_buffer = NULL;
  u64 content_buffer_cursor = 0;

  char buffer[2048];
  ssize_t bytes_read;

  char *parse_headers_error = NULL;
  char *content_length_error = NULL;

  while ((bytes_read = recv(args->client_sock, buffer, sizeof(buffer), 0)) >
         0) {
    if (!headers_done) {
      if (main_length == 0) {
        main_buffer = malloc((bytes_read) * sizeof(char));

        for (int i = 0; i < bytes_read; i++) {
          main_buffer[i] = buffer[i];
        }

        main_length = bytes_read;
      } else {
        main_buffer =
            realloc(main_buffer, (main_length + bytes_read) * sizeof(char));

        for (int i = 0; i < bytes_read; i++) {
          main_buffer[main_length + i] = buffer[i];
        }

        main_length += bytes_read;
      }

      char *head_end_identifier = "\r\n\r\n";

      const char *found_head_end =
          memmem(main_buffer, main_length, head_end_identifier,
                 strlen(head_end_identifier));

      if (found_head_end != NULL) {
        u64 head_end_position = found_head_end - main_buffer + 4;

        head_length = head_end_position;
        head_buffer = malloc((head_length + 1) * sizeof(char));

        for (u64 i = 0; i < (head_length); i++) {
          head_buffer[i] = main_buffer[i];
        }

        head_buffer[head_length] = '\0';

        headers_done = true;

        result_t res_parse = parse_request_headers(req, head_buffer);
        if (sonic_is_err(res_parse)) {
          parse_headers_error = strdup(res_parse.error_message);
        }

        sonic_free_result(res_parse);

        // calculate content length
        char *content_length_str = utils_get_header_value(
            req->headers, req->headers_count, "Content-Length");

        if (content_length_str == NULL) {
          content_length = 0;
        } else {
          content_length = atoi(content_length_str);
        }

        // allocate the content
        // limit the maximum amount of content to avoid DOS
        u64 content_length_limit = 1000000; // 1 MB

        if (content_length > content_length_limit) {
          content_length_error = strdup("content length too large");
        } else {
          content_buffer = malloc(content_length * sizeof(char));

          // copy if any, the remaining data in main buffer that are not
          // part of head to content
          for (u64 i = head_length; i < main_length; i++) {
            content_buffer[i - head_length] = main_buffer[i];

            // update content_buffer_cursor
            content_buffer_cursor += 1;
          }
        }
      }

    } else {
      // add all subsequent buffers to content buffer. ensure to check if
      // it won't exceed content_length

      for (int i = 0; i < bytes_read; i++) {
        if ((content_buffer_cursor + i) > content_length) {
          content_length_error =
              strdup("The received content exceeded declared content length");

          break;
        }

        content_buffer[content_buffer_cursor + i] = buffer[i];
      }

      // Increment content_buffer_cursor by bytes_read
      content_buffer_cursor += bytes_read;
    }

    if (headers_done && content_length == 0) {
      break;
    } else if (content_length > 0 &&
               (ssize_t)(head_length + content_length) <= bytes_read) {
      break;
    } else if (content_buffer_cursor >= content_length) {
      break;
    }
  }

  if (bytes_read == -1) {
    free(req);
    close(args->client_sock);
    free(args);

    return NULL;
  }

  if (head_length == 0) {
    free(req);
    close(args->client_sock);
    free(args);

    return NULL;
  };

  if (parse_headers_error != NULL) {
    sonic_server_response_t *resp =
        new_server_response(STATUS_400, MIME_TEXT_PLAIN);
    server_response_set_body(resp, parse_headers_error,
                             strlen(parse_headers_error));

    server_send_response(req, resp);
    free_server_response(resp);

    free(parse_headers_error);

    free(main_buffer);
    free(head_buffer);
    free(content_buffer);
    free_server_request(req);

    close(args->client_sock);
    free(args);

    return NULL;
  };

  if (content_length_error != NULL) {
    sonic_server_response_t *resp =
        new_server_response(STATUS_400, MIME_TEXT_PLAIN);
    server_response_set_body(resp, content_length_error,
                             strlen(content_length_error));

    server_send_response(req, resp);
    free_server_response(resp);

    free(content_length_error);

    free(main_buffer);
    free(head_buffer);
    free(content_buffer);
    free_server_request(req);

    close(args->client_sock);
    free(args);

    return NULL;
  };

  req->request_body = content_buffer;
  req->request_body_size = content_length;

  req->client_ip = strdup(inet_ntoa(args->client_addr.sin_addr));
  req->client_port = (u64)args->client_addr.sin_port;

  req->client_sock = args->client_sock;

  process_request(args->http_server, req);

  free(main_buffer);
  free(head_buffer);
  free(content_buffer);
  free_server_request(req);

  close(args->client_sock);
  free(args);

  return NULL;
}

#include <poll.h>

/****
 * starts a HTTP server
 *
 ****/
int start_server(sonic_server_t *http_server) {
  int server_sock;
  // int client_sock;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Create a socket for the server.
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock == -1) {
    perror("SONIC: Socket creation failed");
    return 1;
  }

  int option = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  // setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

  // Configure server address.
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(http_server->host);
  server_addr.sin_port = htons(http_server->port);

  // Bind the socket to the specified address and port.
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("SONIC: Binding failed");
    close(server_sock);
    return 1;
  }

  // Listen for incoming connections.
  if (listen(server_sock, 5) == -1) {
    perror("SONIC: Listening failed");
    close(server_sock);
    return 1;
  }

  http_server->server_socket = server_sock;
  struct pollfd fds[1];
  fds[0].fd = server_sock;
  fds[0].events = POLLIN;

  int timeout_ms = 100;

  while (1 && !http_server->should_exit) {
    // Use poll to check for activity on the server socket.
    int ready = poll(fds, 1, timeout_ms); // Wait indefinitely for activity.

    if (ready == -1) {
      perror("SONIC: Poll error");
      break;
    } else if (ready == 0) {
      // printf("Timeout occurred...\n");
      if (http_server->should_exit) {
        return 0;
      }

    } else if (ready > 0) {
      // Check if the server socket has data to accept.
      if (fds[0].revents & POLLIN) {
        int client_sock =
            accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
          perror("SONIC: Accepting connection failed");
          continue;
        }

        pthread_t handle_request_thread;
        handle_request_thread_args_t *handle_request_thread_args =
            malloc(sizeof(handle_request_thread_args_t));
        handle_request_thread_args[0] =
            (handle_request_thread_args_t){.http_server = http_server,
                                           .client_sock = client_sock,
                                           .client_addr = client_addr};

        if (pthread_create(&handle_request_thread, NULL, handle_request,
                           handle_request_thread_args) != 0) {
          printf("SONIC ERROR: Could not spawn handle request thread");
          return 1;
        }
      }
    }
  }

  return 0;
}

void free_route(sonic_route_t *route) {
  free_path(route->path);

  if (route->is_file_route) {
    free(route->file_route_path);
  }

  for (u64 i = 0; i < route->middlewares_count; i++) {
    free(route->middlewares[i]);
  }

  if (route->middlewares_count > 0) {
    free(route->middlewares);
  }

  free(route);
}

/****
 * stops a http server
 *
 ****/
void stop_server(sonic_server_t *server) {
  server->should_exit = true;
  shutdown(server->server_socket, SHUT_RDWR);
  close(server->server_socket);

  for (u64 i = 0; i < server->routes_count; i++) {
    free_route(server->routes[i]);
  }

  for (u64 i = 0; i < server->middlewares_count; i++) {
    free(server->middlewares[i]);
  }

  if (server->middlewares_count > 0) {
    free(server->middlewares);
  }

  if (server->routes_count > 0) {
    free(server->routes);
  }
}

/****
 * instantiates a new HTTP server
 *
 ****/
sonic_server_t *new_server(char *host, u16 port) {
  sonic_server_t *server = malloc(sizeof(sonic_server_t));

  server->port = port;
  server->host = host;
  server->routes = NULL;
  server->routes_count = 0;
  server->should_exit = false;
  server->server_socket = 0;

  server->middlewares = NULL;
  server->middlewares_count = 0;

  return server;
}

/****
 * instantiates a new HTTP server response
 *
 ****/
sonic_server_response_t *
new_server_response(sonic_status_t status, sonic_content_type_t content_type) {

  sonic_server_response_t *resp = malloc(sizeof(sonic_server_response_t));

  resp->headers = NULL;
  resp->headers_count = 0;

  resp->status = status;
  resp->content_type = content_type;

  resp->response_body = NULL;
  resp->response_body_size = 0;

  return resp;
}

/****
 * sets the body of a response
 *
 ****/
void server_response_set_body(sonic_server_response_t *resp,
                              char *response_body, u64 response_body_size) {
  if (response_body_size > 0) {
    resp->response_body = response_body;
  } else {
    resp->response_body = NULL;
  }

  resp->response_body_size = response_body_size;
}

/****
 * adds a header to a response
 *
 ****/
void server_response_add_header(sonic_server_response_t *resp, char *key,
                                char *value) {
  sonic_header_t new_header;

  new_header.key = malloc((strlen(key) + 1) * sizeof(char));
  strcpy(new_header.key, key);

  new_header.value = malloc((strlen(value) + 1) * sizeof(char));
  strcpy(new_header.value, value);

  if (resp->headers_count == 0) {
    resp->headers = malloc((resp->headers_count + 1) * sizeof(sonic_header_t));
  } else {
    resp->headers = realloc(resp->headers,
                            (resp->headers_count + 1) * sizeof(sonic_header_t));
  }

  resp->headers[resp->headers_count] = new_header;

  resp->headers_count++;
}