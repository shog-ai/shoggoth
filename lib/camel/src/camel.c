/**** camel.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Camel testing framework, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../camel.h"
#include "../include/internal.h"
#include "../include/tuwi.h"

#include <netlibc/fs.h>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

bool __files_equal(char *first_file, char *second_file) {
  char command1[1024]; // Buffer for the first checksum command
  char command2[1024]; // Buffer for the second checksum command

  // Create the shell command to calculate the checksum of file1
  snprintf(command1, sizeof(command1), "md5sum \"%s\" 2>/dev/null", first_file);

  // Create the shell command to calculate the checksum of file2
  snprintf(command2, sizeof(command2), "md5sum \"%s\" 2>/dev/null",
           second_file);

  // Execute the shell commands using popen and capture their output
  FILE *fp1 = popen(command1, "r");
  FILE *fp2 = popen(command2, "r");

  if (fp1 == NULL || fp2 == NULL) {
    perror("popen");
    exit(EXIT_FAILURE);
  }

  char checksum1[32], checksum2[32]; // Buffer to store checksums
  fgets(checksum1, sizeof(checksum1), fp1);
  fgets(checksum2, sizeof(checksum2), fp2);

  // Close the file pointers
  pclose(fp1);
  pclose(fp2);

  // Compare the checksums
  if (strcmp(checksum1, checksum2) == 0) {
    return true; // Checksums match
  } else {
    return false; // Checksums do not match
  }
}

char *__read_file(char *path) {
  char *content = UNWRAP(read_file_to_string(path));

  return content;
}

int __await_process(process_t *process) {
  int status;

  // Wait for the child process to exit
  if (waitpid(process->pid, &status, 0) == -1) {
    perror("waitpid");
    return 1;
  }

  int exit_status = WEXITSTATUS(status);

  return exit_status;
}

void __kill_process(process_t *process, int signal) {
  // Send a kill signal to the process
  if (kill(process->pid, signal) == -1) {
    printf("kill process failed \n");
    exit(1);
  }
}

process_t *__run_executable(char *output_path, char *executable_path, ...) {
  process_t *process = malloc(sizeof(process_t));

  pid_t pid = fork();

  process->pid = pid;

  if (pid == -1) {
    printf("could not fork child process \n");
    exit(1);
  } else if (pid == 0) {
    // CHILD PROCESS

    int output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
      printf("could not open output file \n");
      exit(1);
    }

    // Redirect stdout and stderr to the output file
    dup2(output_fd, STDOUT_FILENO);
    dup2(output_fd, STDERR_FILENO);

    va_list args;
    va_start(args, executable_path);

    // Count the number of variadic arguments
    int num_args = 0;
    const char *arg;
    while ((arg = va_arg(args, const char *)) != NULL) {
      num_args++;
    }

    va_end(args);

    // Create an array to hold the arguments
    char **arg_list = (char **)malloc((num_args + 2) * sizeof(char *));
    if (arg_list == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }

    // Fill the argument array
    va_start(args, executable_path);
    arg_list[0] = (char *)executable_path;
    for (int i = 1; i <= num_args; i++) {
      arg_list[i] = (char *)va_arg(args, const char *);
    }
    arg_list[num_args + 1] = NULL; // End of argument list
    va_end(args);

    // Execute the command
    if (execv(executable_path, arg_list) == -1) {
      perror("execv");
      exit(EXIT_FAILURE);
    }

    printf("error occured while launching executable \n");
    close(output_fd);
    exit(1);
  } else {
    // PARENT PROCESS

    usleep(1000);
  }

  return process;
}

camel_ctx_t *__init_camel_ctx() {
  camel_ctx_t *ctx = malloc(sizeof(camel_ctx_t));

  ctx->runner = NULL;
  ctx->fail_fast = false;
  ctx->parallel = false;
  ctx->return_error_code = true;

  return ctx;
}

void __init_runner(camel_ctx_t *ctx, int argc, char **argv, char *file_name,
                   runner_type_t runner_type) {
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      ctx->parallel = true;
    } else if (strcmp(argv[i], "-f") == 0) {
      ctx->fail_fast = true;
    } else if (strcmp(argv[i], "-e") == 0) {
      ctx->return_error_code = false;
    }
  }

  runner_t *test_runner = (runner_t *)malloc(sizeof(runner_t));

  test_runner->runner_type = runner_type;

  test_runner->suites_count = 0;
  test_runner->test_suites = NULL;

  test_runner->passed = 0;
  test_runner->failed = 0;
  test_runner->total_tests_count = 0;
  test_runner->total_assertions = 0;

  test_runner->runner_name = malloc((strlen(file_name) + 1) * sizeof(char));
  strcpy(test_runner->runner_name, file_name);

  ctx->runner = test_runner;
}

void __fail_fast(camel_ctx_t *ctx) {
  camel_tui_reset_color();
  printf("FAIL FAST! \n");

  if (ctx->return_error_code) {
    exit(1);
  } else {
    exit(0);
  }
}

void __add_suite(camel_ctx_t *ctx, suite_func_t suite_func,
                 const char *suite_name) {
  suite_t *test_suite = (suite_t *)malloc(sizeof(suite_t));

  test_suite->test_cases = NULL;
  test_suite->cases_count = 0;

  test_suite->passed = 0;
  test_suite->failed = 0;
  test_suite->total_assertions = 0;

  test_suite->suite_name =
      (char *)malloc((strlen(suite_name) + 1) * sizeof(char));
  strcpy(test_suite->suite_name, suite_name);

  if (ctx->runner->suites_count == 0) {
    ctx->runner->test_suites =
        (suite_t **)malloc((ctx->runner->suites_count + 1) * sizeof(suite_t *));

    ctx->runner->test_suites[ctx->runner->suites_count] = test_suite;
  } else {
    ctx->runner->test_suites = (suite_t **)realloc(
        ctx->runner->test_suites,
        (ctx->runner->suites_count + 1) * sizeof(suite_t *));

    ctx->runner->test_suites[ctx->runner->suites_count] = test_suite;
  }

  ctx->runner->suites_count++;

  suite_func(test_suite);
}

test_t *__add_test(suite_t *test_suite, test_func_t test_func, char *test_name,
                   int repeat_count) {
  test_t *test_case = (test_t *)malloc(sizeof(test_t));

  test_case->test_name = (char *)malloc((strlen(test_name) + 1) * sizeof(char));
  strcpy(test_case->test_name, test_name);

  test_case->test_func = test_func;

  test_case->repeat_count = repeat_count;

  test_case->has_setup = false;
  test_case->has_teardown = false;
  test_case->test_failed = false;
  test_case->total_assertions = 0;

  test_suite->test_cases = (test_t **)realloc(
      test_suite->test_cases, (test_suite->cases_count + 1) * sizeof(test_t *));

  test_suite->test_cases[test_suite->cases_count] = test_case;

  test_suite->cases_count++;

  return test_case;
}

teardown_t *__add_teardown(test_t *test_case, teardown_func_t teardown_func,
                           char *teardown_name) {
  if (!test_case->has_setup) {
    camel_tui_set_color_rgb(ERROR_COLOR);
    printf("ERROR: Cannot add teardown without setup for test (%s) \n",
           test_case->test_name);
    camel_tui_reset_color();
    exit(1);
  }

  teardown_t *teardown = (teardown_t *)malloc(sizeof(teardown_t));

  teardown->teardown_name =
      (char *)malloc((strlen(teardown_name) + 1) * sizeof(char));
  strcpy(teardown->teardown_name, teardown_name);

  teardown->teardown_func = teardown_func;

  test_case->has_teardown = true;
  test_case->teardown = teardown;

  return teardown;
};

setup_t *__add_setup(test_t *test_case, setup_func_t setup_func,
                     char *setup_name) {
  setup_t *setup = (setup_t *)malloc(sizeof(setup_t));

  setup->setup_name = (char *)malloc((strlen(setup_name) + 1) * sizeof(char));
  strcpy(setup->setup_name, setup_name);

  setup->setup_func = setup_func;

  test_case->has_setup = true;
  test_case->setup = setup;

  return setup;
}

void *run_test(void *test_args) {
  test_args_t *args = (test_args_t *)test_args;

  bool failed = false;

  for (int i = 0; i < args->test_case->repeat_count; i++) {
    struct timespec start, end;

    if (args->test_case->has_setup) {
      void *setup_result = args->test_case->setup->setup_func();

      args->test_case->setup->setup_result = setup_result;
    }

    // Start the timer
    clock_gettime(CLOCK_MONOTONIC, &start);

    args->test_case->test_func(args->test_case);

    // End the timer
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;

    long long time_ns = (end.tv_sec - start.tv_sec) * 1000000000LL +
                        (end.tv_nsec - start.tv_nsec);

    double time_sec = (end.tv_sec - start.tv_sec) +
                      (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

    char run_time[100];
    if (time_ns < 1000) {
      sprintf(run_time, "%lldns", time_ns);
    } else if (time_ms < 1000.0) {
      sprintf(run_time, "%.2fms", time_ms);
    } else {
      sprintf(run_time, "%.2fs", time_sec);
    }

    if (!args->test_case->test_failed) {
      camel_tui_set_color_rgb(SUCCESS_COLOR);
      if (i > 0) {
        printf("PASS: %s(REPEAT %d) -> %s \n", args->test_case->test_name,
               i + 1, run_time);
      } else {
        printf("PASS: %s -> %s \n", args->test_case->test_name, run_time);
      }

      fflush(stdout);
      camel_tui_reset_color();

    } else {
      failed = true;

      camel_tui_set_color_rgb(ERROR_COLOR);
      if (i > 0) {
        printf("FAIL: %s(REPEAT %d) -> %s \n", args->test_case->test_name,
               i + 1, run_time);
      } else {
        printf("FAIL: %s -> %s \n", args->test_case->test_name, run_time);
      }

      fflush(stdout);
      camel_tui_reset_color();

      if (args->ctx->fail_fast) {
        __fail_fast(args->ctx);
      }
    }

    if (args->test_case->has_teardown) {

      args->test_case->teardown->teardown_func(args->test_case);
    }

    args->test_case->test_failed = false;

    fflush(stdout);
  }

  args->suite->total_assertions += args->test_case->total_assertions;

  if (failed) {
    args->test_case->test_failed = true;
  }

  if (!args->test_case->test_failed) {
    args->suite->passed++;
  } else {
    args->suite->failed++;
  }

  if (args->test_case->has_setup) {
    free(args->test_case->setup->setup_name);
    free(args->test_case->setup);
  }

  if (args->test_case->has_teardown) {
    free(args->test_case->teardown->teardown_name);
    free(args->test_case->teardown);
  }

  free(args->test_case->test_name);
  free(args->test_case);

  free(args);

  return NULL;
}

void run_suite(camel_ctx_t *ctx, suite_t *suite) {
  printf("SUITE -> %s \n", suite->suite_name);

  pthread_t test_threads[suite->cases_count];

  for (uint32_t k = 0; k < suite->cases_count; k++) {

    test_args_t *test_args = malloc(sizeof(test_args_t));
    test_args->suite = suite;
    test_args->test_case = suite->test_cases[k];
    test_args->ctx = ctx;

    if (ctx->parallel) {
      if (pthread_create(&test_threads[k], NULL, run_test, test_args) != 0) {
        printf("ERROR: could not spawn test thread");

        exit(1);
      }
    } else {
      run_test(test_args);
    }

    ctx->runner->total_tests_count++;
  }

  if (ctx->parallel) {
    for (uint32_t p = 0; p < suite->cases_count; p++) {
      pthread_join(test_threads[p], NULL);
    }
  }

  camel_tui_reset_color();

  printf("SUITE SUMMARY (%s): %d passed,", suite->suite_name, suite->passed);

  if (suite->failed > 0) {
    camel_tui_set_color_rgb(ERROR_COLOR);
  }

  printf(" %d failed", suite->failed);
  camel_tui_reset_color();

  printf(", %d asserts, "
         "%d tests \n",
         suite->total_assertions, suite->cases_count);
}

int __run_all(camel_ctx_t *ctx) {
  uint32_t exit_code = 0;

  char *runner_type_str = NULL;
  if (ctx->runner->runner_type == UNIT) {
    runner_type_str = "UNIT";
  } else if (ctx->runner->runner_type == INTEGRATION) {
    runner_type_str = "INTEGRATION";
  } else if (ctx->runner->runner_type == FUNCTIONAL) {
    runner_type_str = "FUNCTIONAL";
  } else if (ctx->runner->runner_type == FUZZ) {
    runner_type_str = "FUZZ";
  }

  printf("STARTING %s TEST RUNNER "
         "======================== %s\n",
         runner_type_str, ctx->runner->runner_name);

  for (uint32_t i = 0; i < ctx->runner->suites_count; i++) {
    printf("\n");

    run_suite(ctx, ctx->runner->test_suites[i]);

    ctx->runner->passed += ctx->runner->test_suites[i]->passed;
    ctx->runner->failed += ctx->runner->test_suites[i]->failed;
    ctx->runner->total_assertions +=
        ctx->runner->test_suites[i]->total_assertions;

    free(ctx->runner->test_suites[i]->suite_name);
    free(ctx->runner->test_suites[i]->test_cases);
    free(ctx->runner->test_suites[i]);
  }

  printf("TEST SUMMARY (%s): %d passed,", ctx->runner->runner_name,
         ctx->runner->passed);

  if (ctx->runner->failed > 0) {
    camel_tui_set_color_rgb(ERROR_COLOR);
  }

  printf(" %d failed", ctx->runner->failed);
  camel_tui_reset_color();

  printf(", %d suites, "
         "%d asserts, %d tests \n",
         ctx->runner->suites_count, ctx->runner->total_assertions,
         ctx->runner->total_tests_count);

  printf("FINISHED TEST RUNNER "
         "===================================\n\n");

  if (ctx->runner->failed > 0) {
    exit_code = 1;
  }

  if (!ctx->return_error_code) {
    exit_code = 0;
  }

  // free resources
  free(ctx->runner->runner_name);
  free(ctx->runner->test_suites);
  free(ctx->runner);
  free(ctx);

  return exit_code;
}

camel_rgb_t camel_rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (camel_rgb_t){.r = r, .g = g, .b = b};
}

void camel_tui_set_color_rgb(camel_rgb_t color) {
  tuwi_set_color_rgb(rgb(color.r, color.g, color.b)); // TUWI library
};

void camel_tui_reset_color() {
  tuwi_reset_color(); // TUWI library
};
