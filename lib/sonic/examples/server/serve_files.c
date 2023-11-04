#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example demonstrates serving files and directories

int main() {
  // create a server
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  // add routes
  sonic_add_file_route(server, "/", METHOD_GET, "./myfile.txt",
                       MIME_TEXT_PLAIN);

  // will add all files in the directory and all subdirectories as individual
  // paths relative to the provided root path
  // also serves a html page for directories and subdirectories listing the
  // files they contain as hyperlinks
  sonic_add_directory_route(server, "/files", "./my_directory/");

  printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  // start the server
  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
