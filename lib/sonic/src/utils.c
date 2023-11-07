/**** utils.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#define _GNU_SOURCE

#include "../sonic.h"

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <regex.h>
#include <stdlib.h>

#include <assert.h>
#include <netdb.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>

/****
 * Helper function to concatenate two strings with a separator '/'
 *
 ****/
char *concat_path(char *path1, char *path2) {
  char *result = malloc(strlen(path1) + 1 + strlen(path2) + 1);
  if (result) {
    strcpy(result, path1);
    strcat(result, "/");
    strcat(result, path2);
  }
  return result;
}

/****
 * Recursive function to list files in a directory and its subdirectories
 *
 ****/
void list_files_recursive(files_list_t *result, char *dir_path) {
  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    printf("ERROR: could not open directory: %s\n", dir_path);
    exit(1);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) { // Check if it's a regular file
      result->files_count++;
      result->files =
          (char **)realloc(result->files, result->files_count * sizeof(char *));
      result->files[result->files_count - 1] =
          concat_path(dir_path, entry->d_name);
    } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
               strcmp(entry->d_name, "..") != 0) {
      // Recursively list files in subdirectories (ignore "." and "..")
      char *subdir_path = concat_path(dir_path, entry->d_name);
      list_files_recursive(result, subdir_path);
      free(subdir_path);
    }
  }

  closedir(dir);
}

/****
 * lists files in a directory and its subdirectories
 *
 ****/
files_list_t *get_files_list_from_dir(char *dir_path) {
  files_list_t *result = calloc(1, sizeof(files_list_t));

  result->files = NULL;
  result->files_count = 0;

  list_files_recursive(result, dir_path);

  return result;
}

/****
 * takes a string and returns it without the specified prefix
 *
 ****/
char *remove_prefix(const char *prefix, const char *input) {
  // Calculate the lengths of the prefix and input strings
  size_t prefixLen = strlen(prefix);
  size_t inputLen = strlen(input);

  // Check if the prefix is longer than the input or if the input does not start
  // with the prefix
  if (prefixLen > inputLen || strncmp(prefix, input, prefixLen) != 0) {
    // If the prefix is not a prefix of the input, return a copy of the input
    char *result = (char *)malloc((inputLen + 1) * sizeof(char));
    if (result == NULL) {
      perror("Memory allocation error");
      exit(EXIT_FAILURE);
    }
    strcpy(result, input);
    return result;
  }

  // Calculate the length of the resulting string after removing the prefix
  size_t resultLen = inputLen - prefixLen;

  // Allocate memory for the resulting string
  char *result = (char *)malloc((resultLen + 1) * sizeof(char));
  if (result == NULL) {
    perror("Memory allocation error");
    exit(EXIT_FAILURE);
  }

  // Copy the remaining part of the input (after the prefix) to the result
  strcpy(result, input + prefixLen);

  return result;
}

/****
 * gets the extension of a file from it's path
 *
 ****/
// char *get_file_extension(const char *file_path) {
//   char *extension = strrchr(file_path, '.');
//   if (extension == NULL || extension == file_path) {
//     return NULL; // No extension found or the dot is the first character
//   } else {
//     return extension + 1; // Skip the dot and return the extension
//   }
// }

/****
 * converts a string to a HTTP method
 *
 ****/
sonic_method_t str_to_http_method(const char *method_str) {
  if (strcmp(method_str, "GET") == 0) {
    return METHOD_GET;
  } else if (strcmp(method_str, "HEAD") == 0) {
    return METHOD_HEAD;
  } else if (strcmp(method_str, "POST") == 0) {
    return METHOD_POST;
  } else if (strcmp(method_str, "PUT") == 0) {
    return METHOD_PUT;
  } else if (strcmp(method_str, "DELETE") == 0) {
    return METHOD_DELETE;
  } else if (strcmp(method_str, "CONNECT") == 0) {
    return METHOD_CONNECT;
  } else if (strcmp(method_str, "OPTIONS") == 0) {
    return METHOD_OPTIONS;
  } else if (strcmp(method_str, "TRACE") == 0) {
    return METHOD_TRACE;
  } else {
    printf("Invalid http method /n");
    exit(1);
  }
}

/****
 * converts a http method to a string
 *
 ****/
char *http_method_to_str(sonic_method_t method) {
  if (method == METHOD_GET) {
    return "GET";
  } else if (method == METHOD_HEAD) {
    return "HEAD";
  } else if (method == METHOD_POST) {
    return "POST";
  } else if (method == METHOD_PUT) {
    return "PUT";
  } else if (method == METHOD_DELETE) {
    return "DELETE";
  } else if (method == METHOD_CONNECT) {
    return "CONNECT";
  } else if (method == METHOD_OPTIONS) {
    return "OPTIONS";
  } else if (method == METHOD_TRACE) {
    return "TRACE";
  } else {
    printf("ERROR: cannot convert method to string: %d \n", method);
    exit(1);
  }
}

/****
 * checks if a string is an IP address
 *
 ****/
bool is_ip_address(const char *str) {
  regex_t regex;
  const char *pattern = "^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                        "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                        "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\\."
                        "(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$";

  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    return false; // Regular expression compilation failed
  }

  int result = regexec(&regex, str, 0, NULL, 0);
  regfree(&regex);

  return (result == 0);
}

/****
 * converts a domain name to an IP address
 *
 ****/
int resolve_domain_to_ip(const char *domain, char *ip_buffer,
                         size_t ip_buffer_size) {
  struct addrinfo hints;
  struct addrinfo *res;
  struct addrinfo *p;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  int status = getaddrinfo(domain, NULL, &hints, &res);
  if (status != 0) {
    return 1;
  }

  // Find the first valid address
  for (p = res; p != NULL; p = p->ai_next) {
    if (p->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      inet_ntop(AF_INET, &(ipv4->sin_addr), ip_buffer,
                (socklen_t)ip_buffer_size);
      freeaddrinfo(res); // Free the memory allocated for addrinfo
      return 0;          // Successfully resolved IP address
    }
  }

  freeaddrinfo(res); // Free the memory allocated for addrinfo
  return 1;          // IP address resolution failed
}

/****
 * extracts the host from a URL
 *
 ****/
void extract_host_from_url(const char *url, char host[]) {
  const char *start = url;
  const char *end = strstr(url, "://"); // Find the scheme delimiter

  if (end != NULL) {
    start = end + 3; // Move the start pointer past "://"
  }

  end = strchr(start, '/'); // Find the first slash to delimit the host

  if (end != NULL) {
    // Calculate the length of the host
    size_t hostLength = end - start;

    // Copy the host into the output string
    strncpy(host, start, hostLength);
    host[hostLength] = '\0';
  } else {
    // If no slash is found, the entire remaining URL is the host
    strcpy(host, start);
  }
}

/****
 * extracts the path from a URL
 *
 ****/
void extract_path_from_url(const char *url, char *path) {
  const char *start = url;
  const char *end = strstr(url, "://"); // Find the scheme delimiter

  if (end != NULL) {
    start = end + 3; // Move the start pointer past "://"
  }

  end = strchr(start, '/'); // Find the first slash to delimit the path

  if (end != NULL) {
    // Calculate the length of the path
    // size_t pathLength = strlen(end);

    // Copy the path into the output string
    strcpy(path, end);
  } else {
    // If no slash is found, there is no path, so set an empty string
    strcpy(path, "/");
  }
}

/****
 * extracts the host and port from a URL
 *
 ****/
void extract_hostname_and_port_from_host(const char *host,
                                         sonic_scheme_t scheme, char *hostname,
                                         u16 *port) {
  // Check if the host contains a colon (:) indicating a port
  const char *colon = strchr(host, ':');

  if (colon != NULL) {
    // Extract the IP address portion before the colon
    size_t ipLength = colon - host;
    strncpy(hostname, host, ipLength);
    hostname[ipLength] = '\0';

    // Extract the port portion after the colon and convert it to an integer
    *port = atoi(colon + 1);

  } else {
    // If no colon is found, it's assumed to be a domain name
    strcpy(hostname, host);

    if (scheme == SCHEME_HTTP) {
      *port = 80;
    } else {
      *port = 443;
    }
  }
}

/****
 * extracts the scheme from a URL
 *
 ****/
void extract_scheme_from_url(const char *url, char scheme[]) {
  const char *schemeDelimiter = "://";
  const char *schemeEnd = strstr(url, schemeDelimiter);

  if (schemeEnd != NULL) {
    // Calculate the length of the scheme
    size_t schemeLength = schemeEnd - url;

    // Copy the scheme into the output string
    strncpy(scheme, url, schemeLength);
    scheme[schemeLength] = '\0';
  } else {
    // If no scheme is found, return an empty string
    scheme[0] = '\0';
  }
}

/****
 * converts an integer status code to its enum equivalent
 *
 ****/
sonic_status_t number_to_status_code(u16 status) {
  if (status == 100) {
    return STATUS_100;
  } else if (status == 101) {
    return STATUS_101;
  } else if (status == 200) {
    return STATUS_200;
  } else if (status == 201) {
    return STATUS_201;
  } else if (status == 202) {
    return STATUS_202;
  } else if (status == 203) {
    return STATUS_203;
  } else if (status == 204) {
    return STATUS_204;
  } else if (status == 205) {
    return STATUS_205;
  } else if (status == 206) {
    return STATUS_206;
  } else if (status == 300) {
    return STATUS_300;
  } else if (status == 301) {
    return STATUS_301;
  } else if (status == 302) {
    return STATUS_302;
  } else if (status == 303) {
    return STATUS_303;
  } else if (status == 304) {
    return STATUS_304;
  } else if (status == 305) {
    return STATUS_305;
  } else if (status == 307) {
    return STATUS_307;
  } else if (status == 400) {
    return STATUS_400;
  } else if (status == 401) {
    return STATUS_401;
  } else if (status == 402) {
    return STATUS_402;
  } else if (status == 403) {
    return STATUS_403;
  } else if (status == 404) {
    return STATUS_404;
  } else if (status == 405) {
    return STATUS_405;
  } else if (status == 406) {
    return STATUS_406;
  } else if (status == 407) {
    return STATUS_407;
  } else if (status == 408) {
    return STATUS_408;
  } else if (status == 409) {
    return STATUS_409;
  } else if (status == 410) {
    return STATUS_410;
  } else if (status == 411) {
    return STATUS_411;
  } else if (status == 412) {
    return STATUS_412;
  } else if (status == 413) {
    return STATUS_413;
  } else if (status == 414) {
    return STATUS_414;
  } else if (status == 415) {
    return STATUS_415;
  } else if (status == 416) {
    return STATUS_416;
  } else if (status == 417) {
    return STATUS_417;
  } else if (status == 426) {
    return STATUS_426;
  } else if (status == 429) {
    return STATUS_429;
  } else if (status == 500) {
    return STATUS_500;
  } else if (status == 501) {
    return STATUS_501;
  } else if (status == 502) {
    return STATUS_502;
  } else if (status == 503) {
    return STATUS_503;
  } else if (status == 504) {
    return STATUS_504;
  } else if (status == 505) {
    return STATUS_505;
  } else {
    printf("ERROR: cannot convert number to status_code: %d \n", status);
    exit(1);
  }
}

/****
 * converts a status code to a string
 *
 ****/
char *utils_status_code_to_string(sonic_status_t status_code) {
  if (status_code == STATUS_100) {
    return "100 Continue";
  } else if (status_code == STATUS_101) {
    return "101 Switching Protocols";
  } else if (status_code == STATUS_200) {
    return "200 OK";
  } else if (status_code == STATUS_201) {
    return "201 Created";
  } else if (status_code == STATUS_202) {
    return "202 Accepted";
  } else if (status_code == STATUS_203) {
    return "203 Non-Authoritative Information";
  } else if (status_code == STATUS_204) {
    return "204 No Content";
  } else if (status_code == STATUS_205) {
    return "205 Reset Content";
  } else if (status_code == STATUS_206) {
    return "206 Partial Content";
  } else if (status_code == STATUS_300) {
    return "300 Multiple Choices";
  } else if (status_code == STATUS_301) {
    return "301 Moved Permanently";
  } else if (status_code == STATUS_302) {
    return "302 Found";
  } else if (status_code == STATUS_303) {
    return "303 See Other";
  } else if (status_code == STATUS_304) {
    return "304 Not Modified";
  } else if (status_code == STATUS_305) {
    return "305 Use Proxy";
  } else if (status_code == STATUS_307) {
    return "307 Temporary Redirect";
  } else if (status_code == STATUS_400) {
    return "400 Bad Request";
  } else if (status_code == STATUS_401) {
    return "401 Unauthorized";
  } else if (status_code == STATUS_402) {
    return "402 Payment Required";
  } else if (status_code == STATUS_403) {
    return "403 Forbidden";
  } else if (status_code == STATUS_404) {
    return "404 Not Found";
  } else if (status_code == STATUS_405) {
    return "405 Method Not Allowed";
  } else if (status_code == STATUS_406) {
    return "406 Not Acceptable";
  } else if (status_code == STATUS_407) {
    return "407 Proxy Authentication Required";
  } else if (status_code == STATUS_408) {
    return "408 Request Timeout";
  } else if (status_code == STATUS_409) {
    return "409 Conflict";
  } else if (status_code == STATUS_410) {
    return "410 Gone";
  } else if (status_code == STATUS_411) {
    return "411 Length Required";
  } else if (status_code == STATUS_412) {
    return "412 Precondition Failed";
  } else if (status_code == STATUS_413) {
    return "413 Payload Too Large";
  } else if (status_code == STATUS_414) {
    return "414 URI Too Long";
  } else if (status_code == STATUS_415) {
    return "415 Unsupported Media Type";
  } else if (status_code == STATUS_416) {
    return "416 Range Not Satisfiable";
  } else if (status_code == STATUS_417) {
    return "417 Expectation Failed";
  } else if (status_code == STATUS_426) {
    return "426 Upgrade Required";
  } else if (status_code == STATUS_429) {
    return "429 Too Many Requests";
  } else if (status_code == STATUS_500) {
    return "500 Internal Server Error";
  } else if (status_code == STATUS_501) {
    return "501 Not Implemented";
  } else if (status_code == STATUS_502) {
    return "502 Bad Gateway";
  } else if (status_code == STATUS_503) {
    return "503 Service Unavailable";
  } else if (status_code == STATUS_504) {
    return "504 Gateway Timeout";
  } else if (status_code == STATUS_505) {
    return "505 HTTP Version Not Supported";
  } else {
    printf("ERROR: unhandled status code: %d \n", status_code);
    exit(1);
  }
}

/****
 * queries a header from an array of headers
 *
 ****/
char *utils_get_header_value(sonic_header_t *headers, u64 headers_count,
                             char *key) {
  for (u64 i = 0; i < headers_count; i++) {
    if (strcmp(headers[i].key, key) == 0) {
      return headers[i].value;
    }
  }

  return NULL;
}

/****
 * converts a file extension to a HTTP content type (MIME type)
 *
 ****/
sonic_content_type_t file_extension_to_content_type(char *file_extension) {
  if (strcmp(file_extension, "aac") == 0) {
    return MIME_AUDIO_AAC;
  } else if (strcmp(file_extension, "mp3") == 0) {
    return MIME_AUDIO_MPEG;
  } else if (strcmp(file_extension, "wav") == 0) {
    return MIME_AUDIO_WAV;
  } else if (strcmp(file_extension, "weba") == 0) {
    return MIME_AUDIO_WEBM;
  } else if (strcmp(file_extension, "mp4") == 0) {
    return MIME_VIDEO_MP4;
  } else if (strcmp(file_extension, "3gp") == 0) {
    return MIME_VIDEO_3GPP;
  } else if (strcmp(file_extension, "webm") == 0) {
    return MIME_VIDEO_WEBM;
  } else if (strcmp(file_extension, "mpeg") == 0) {
    return MIME_VIDEO_MPEG;
  } else if (strcmp(file_extension, "avi") == 0) {
    return MIME_VIDEO_X_MSVIDEO;
  } else if (strcmp(file_extension, "zip") == 0) {
    return MIME_APPLICATION_ZIP;
  } else if (strcmp(file_extension, "xhtml") == 0) {
    return MIME_APPLICATION_XHTML;
  } else if (strcmp(file_extension, "xml") == 0) {
    return MIME_APPLICATION_XML;
  } else if (strcmp(file_extension, "tar") == 0) {
    return MIME_APPLICATION_X_TAR;
  } else if (strcmp(file_extension, "pdf") == 0) {
    return MIME_APPLICATION_PDF;
  } else if (strcmp(file_extension, "jar") == 0) {
    return MIME_APPLICATION_JAVA_ARCHIVE;
  } else if (strcmp(file_extension, "json") == 0) {
    return MIME_APPLICATION_JSON;
  } else if (strcmp(file_extension, "bin") == 0) {
    return MIME_APPLICATION_OCTET_STREAM;
  } else if (strcmp(file_extension, "doc") == 0) {
    return MIME_APPLICATION_MSWORD;
  } else if (strcmp(file_extension, "docx") == 0) {
    return MIME_APPLICATION_MSWORDX;
  } else if (strcmp(file_extension, "gz") == 0) {
    return MIME_APPLICATION_GZIP;
  } else if (strcmp(file_extension, "txt") == 0) {
    return MIME_TEXT_PLAIN;
  } else if (strcmp(file_extension, "csv") == 0) {
    return MIME_TEXT_CSV;
  } else if (strcmp(file_extension, "html") == 0) {
    return MIME_TEXT_HTML;
  } else if (strcmp(file_extension, "css") == 0) {
    return MIME_TEXT_CSS;
  } else if (strcmp(file_extension, "js") == 0) {
    return MIME_TEXT_JAVASCRIPT;
  } else if (strcmp(file_extension, "gif") == 0) {
    return MIME_IMAGE_GIF;
  } else if (strcmp(file_extension, "webp") == 0) {
    return MIME_IMAGE_WEBP;
  } else if (strcmp(file_extension, "ico") == 0) {
    return MIME_IMAGE_ICO;
  } else if (strcmp(file_extension, "bmp") == 0) {
    return MIME_IMAGE_BMP;
  } else if (strcmp(file_extension, "png") == 0) {
    return MIME_IMAGE_PNG;
  } else if (strcmp(file_extension, "jpg") == 0) {
    return MIME_IMAGE_JPEG;
  } else if (strcmp(file_extension, "jpeg") == 0) {
    return MIME_IMAGE_JPEG;
  } else if (strcmp(file_extension, "svg") == 0) {
    return MIME_IMAGE_SVG;
  } else {
    return MIME_APPLICATION_OCTET_STREAM;
  }
}

/****
 * converts a content type (MIME type) to a string
 *
 ****/
char *content_type_to_string(sonic_content_type_t content_type) {
  if (content_type == MIME_AUDIO_AAC) {
    return "audio/acc";
  } else if (content_type == MIME_AUDIO_MPEG) {
    return "audio/mpeg";
  } else if (content_type == MIME_AUDIO_WAV) {
    return "audio/wav";
  } else if (content_type == MIME_AUDIO_WEBM) {
    return "audio/webm";
  } else if (content_type == MIME_VIDEO_MP4) {
    return "video/mp4";
  } else if (content_type == MIME_VIDEO_3GPP) {
    return "video/3gpp";
  } else if (content_type == MIME_VIDEO_WEBM) {
    return "video/webm";
  } else if (content_type == MIME_VIDEO_MPEG) {
    return "video/mpeg";
  } else if (content_type == MIME_VIDEO_X_MSVIDEO) {
    return "video/x-msvideo";
  } else if (content_type == MIME_APPLICATION_ZIP) {
    return "application/zip";
  } else if (content_type == MIME_APPLICATION_XHTML) {
    return "application/xhtml+xml";
  } else if (content_type == MIME_APPLICATION_XML) {
    return "application/xml";
  } else if (content_type == MIME_APPLICATION_X_TAR) {
    return "application/x-tar";
  } else if (content_type == MIME_APPLICATION_PDF) {
    return "application/pdf";
  } else if (content_type == MIME_APPLICATION_JAVA_ARCHIVE) {
    return "application/java-archive";
  } else if (content_type == MIME_APPLICATION_JSON) {
    return "application/json";
  } else if (content_type == MIME_APPLICATION_OCTET_STREAM) {
    return "application/octet-stream";
  } else if (content_type == MIME_APPLICATION_MSWORD) {
    return "application/msword";
  } else if (content_type == MIME_APPLICATION_MSWORDX) {
    return "application/"
           "vnd.openxmlformats-officedocument.wordprocessingml.document";
  } else if (content_type == MIME_APPLICATION_GZIP) {
    return "application/gzip";
  } else if (content_type == MIME_TEXT_XML) {
    return "text/xml";
  } else if (content_type == MIME_TEXT_PLAIN) {
    return "text/plain";
  } else if (content_type == MIME_TEXT_CSV) {
    return "text/csv";
  } else if (content_type == MIME_TEXT_HTML) {
    return "text/html";
  } else if (content_type == MIME_TEXT_CSS) {
    return "text/css";
  } else if (content_type == MIME_TEXT_JAVASCRIPT) {
    return "text/javascript";
  } else if (content_type == MIME_IMAGE_GIF) {
    return "image/gif";
  } else if (content_type == MIME_IMAGE_WEBP) {
    return "image/webp";
  } else if (content_type == MIME_IMAGE_ICO) {
    return "image/vnd.microsoft.icon";
  } else if (content_type == MIME_IMAGE_BMP) {
    return "image/bmp";
  } else if (content_type == MIME_IMAGE_PNG) {
    return "image/png";
  } else if (content_type == MIME_IMAGE_JPEG) {
    return "image/jpeg";
  } else if (content_type == MIME_IMAGE_SVG) {
    return "image/svg";
  } else {
    printf("ERROR: unhandled content type \n");
    exit(1);
  }
}
