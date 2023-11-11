/**** redis.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/redis.h"
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

redis_command_t gen_command(command_type_t com, char *filter, char *data) {
  char *com_str = NULL;

  if (com == COM_GET) {
    com_str = "JSON.GET";
  } else if (com == COM_SET) {
    com_str = "JSON.SET";
  } else if (com == COM_APPEND) {
    com_str = "JSON.ARRAPPEND";
  } else if (com == COM_DEL) {
    com_str = "JSON.DEL";
  } else if (com == COM_INCREMENT) {
    com_str = "JSON.NUMINCRBY";
  } else {
    PANIC("Invalid redis command type \n");
  }

  redis_command_t command = {.data = data, .filter = filter, .com = com_str};

  return command;
}

result_t prepare_client(char *host, u16 port) {
  int sockfd;
  struct sockaddr_in server_addr;

  // Create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Socket creation failed");
    return ERR("Redis socket creation failed");
  }

  // Set up the server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(host);

  // Connect to the Redis server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Connection to Redis server failed");
    close(sockfd);
    return ERR("Connecting to redis failed");
  }

  return OK_INT(sockfd);
}

result_t send_data(int sockfd, char *com) {
  // Send a Redis command (SET mykey myvalue)
  if (send(sockfd, com, strlen(com), 0) == -1) {
    perror("Send failed");
    close(sockfd);
    return ERR(" redis send_data failed");
  }

  // Receive and print the Redis server's response
  char buffer[1024];
  ssize_t bytes_read = 0;
  char *received_data = NULL;
  ssize_t data_len = 0;

  while ((bytes_read = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {

    received_data =
        realloc(received_data,
                (unsigned long)(data_len + bytes_read + 1) * sizeof(char));

    for (int i = 0; i < bytes_read; i++) {
      received_data[data_len + i] = buffer[i];
    }

    data_len += bytes_read;

    if (buffer[bytes_read - 2] == '\r' && buffer[bytes_read - 1] == '\n') {
      break;
    }
  }

  if (bytes_read == -1 || bytes_read == 0) {
    close(sockfd);
    perror("Receive failed");
    return ERR("receive redis data failed");
  }

  received_data[data_len] = '\0';

  return OK(received_data);
}

result_t redis_read(char *host, u16 port, redis_command_t command) {
  result_t res_sockfd = prepare_client(host, port);
  int sockfd = PROPAGATE_INT(res_sockfd);

  char *com =
      malloc((strlen(command.com) + strlen(command.filter) + 4) * sizeof(char));
  sprintf(com, "%s %s\r\n", command.com, command.filter);

  result_t res_reply = send_data(sockfd, com);
  char *reply = PROPAGATE(res_reply);

  // Close the socket
  close(sockfd);
  free(com);

  if (reply == NULL) {
    return ERR("Reply was NULL");
  }

  char *size_str = NULL;
  int size_len = 0;

  int i = 1;
  while (reply[i] != '\r' && reply[i + 1] != '\n') {
    size_str = realloc(size_str, (unsigned long)(size_len + 2) * sizeof(char));
    size_str[size_len] = reply[i];

    size_len++;

    i++;
  }

  size_str[size_len] = '\0';

  int size_int = atoi(size_str);
  free(size_str);

  char *final_reply = NULL;

  if (size_int > -1) {
    final_reply = malloc((unsigned long)(size_int + 1) * sizeof(char));

    for (int k = 0; k < size_int; k++) {
      final_reply[k] = reply[k + i + 2];
    }

    final_reply[size_int] = '\0';
  }

  free(reply);

  return OK(final_reply);
}

result_t redis_write(char *host, u16 port, redis_command_t command) {
  // escape all newlines
  char *escaped_data = remove_newlines_except_quotes(command.data);

  result_t res_sockfd = prepare_client(host, port);
  int sockfd = PROPAGATE_INT(res_sockfd);

  char *com = malloc((strlen(command.com) + strlen(command.filter) +
                      strlen(escaped_data) + 7) *
                     sizeof(char));
  sprintf(com, "%s %s '%s'\r\n", command.com, command.filter, escaped_data);

  free(escaped_data);

  result_t res_reply = send_data(sockfd, com);
  char *reply = PROPAGATE(res_reply);

  if (reply[0] == '-') {
    LOG(ERROR, "%s", reply);
    return ERR("REDIS WRITE FAILED");
  }

  free(com);

  if (reply == NULL) {
    return ERR("Redis reply was NULL");
  }

  // Close the socket
  close(sockfd);

  free(reply);

  return OK(NULL);
}
