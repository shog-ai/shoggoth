{{> head}}

# Sonic - A Simple HTTP library for C

## What is Sonic?

Sonic is an HTTP library written in the C programming language, developed by [ShogAI](https://shog.ai) for use in the Shoggoth project.

Sonic currently implements only a subset of HTTP/1.0, but full compliance with HTTP/1.1 standards is the current goal.

### ⚠️Disclaimer⚠️

This library is still early in development and should be considered unstable and experimental.

This documentation is a work in progress. It is not complete and may contain invalid information as Sonic is rapidly evolving.

## Examples


### Client

Making a GET request ->

```c
#include "sonic.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
  sonic_response_t *resp = sonic_get("https://shog.ai");

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    printf("RESPONSE BODY:\n%.*s \n", (int)resp->response_body_size, resp->response_body);

    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    sonic_free_response(resp);
  }

  return 0;
}
```

### Server

Simple server ->

```c
#include "sonic.h"

#include <stdio.h>
#include <stdlib.h>

void home_route(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
  sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hello world";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

int main() {
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  sonic_add_route(server, "/", METHOD_GET, home_route);

  printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
```

more examples can be found in /examples

You can run client examples with:

```bash
$ E=<example_name> make run-client-example
```

where <example_name> is the name of the example you want to run. For example:

```bash
$ E=simple make run-client-example
```

You can do the same for server examples, simply replace run-client-example with run-server-example:

```bash
$ E=simple make run-server-example
```

## BUILDING SONIC

### Supported platforms

Sonic currently supports only Linux and macOS operating systems.

### Requirements

* OpenSSL 1.1.1
* GNU Make


### Building

#### Clone the repository

```bash
$ git clone https://github.com/shog-ai/sonic
```

#### cd into the cloned directory

```bash
$ cd sonic
```


#### Build with make

```bash
$ make build
```

The above command will build a static library into ./target/libsonic.a which you can link with your project.

### Testing

To run the test suite, use the following command:

```bash
$ make test
```


## USING SONIC

To use sonic, include the header ./sonic.h in your project, then link with the static library in ./target/libsonic.a

You can check the /examples directory for examples of how to use sonic or read the documentation below


## STANDARDS

Since Sonic is still very early in development, none of the below standards have been fully implemented but these are the standards being targeted:

<ul>
<li>RFC 7230 (HTTP/1.1 Message Syntax and Routing)</li>
<li>RFC 7231 (HTTP/1.1 Semantics and Content)</li>
<li>RFC 7232 (HTTP/1.1 Conditional Requests)</li>
<li>RFC 7233 (HTTP/1.1 Range Requests)</li>
<li>RFC 7234 (HTTP/1.1 Caching)</li>
<li>RFC 7235 (HTTP/1.1 Authentication)</li>
</ul>


## CONTRIBUTING

Please follow the [Shoggoth contribution guidelines](/explorer/docs#contributing).


{{> end}}

