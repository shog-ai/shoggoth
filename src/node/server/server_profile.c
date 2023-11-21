/**** server_profile.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../../templating/templating.h"
#include "../db/db.h"
#include "../og/og.h"

#include <stdlib.h>

node_ctx_t *profile_ctx = NULL;

typedef enum {
  GROUP_NULL,
  CODE,
  MODELS,
  DATASETS,
  PAPERS,
} resource_group_t;

void profile_resource_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *resource_name = sonic_get_path_segment(req, "resource_name");
  char *resource_group_str = sonic_get_path_segment(req, "resource_group");

  bool is_inner = req->matched_route->path->has_wildcard;

  char inner_path[FILE_PATH_SIZE];
  sprintf(inner_path, "");

  if (is_inner) {
    sonic_wildcard_segments_t segments = sonic_get_path_wildcard_segments(req);

    for (u64 i = 0; i < segments.count; i++) {
      char buf[FILE_PATH_SIZE];
      sprintf(buf, "/%s", segments.segments[i]);

      strcat(inner_path, buf);
    }

    sonic_free_path_wildcard_segments(segments);
  }

  resource_group_t resource_group = GROUP_NULL;

  if (strcmp(resource_group_str, "code") == 0) {
    resource_group = CODE;
  } else if (strcmp(resource_group_str, "models") == 0) {
    resource_group = MODELS;
  } else if (strcmp(resource_group_str, "datasets") == 0) {
    resource_group = DATASETS;
  } else if (strcmp(resource_group_str, "papers") == 0) {
    resource_group = PAPERS;
  }

  if (resource_group == GROUP_NULL) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid resource group";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  if (strlen(shoggoth_id) != 36) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid Shoggoth ID";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(profile_ctx, runtime_path);

  char user_profile_path[FILE_PATH_SIZE];
  sprintf(user_profile_path, "%s/pins/%s/%s", runtime_path, shoggoth_id,
          resource_group_str);

  char resource_path[FILE_PATH_SIZE];
  sprintf(resource_path, "%s/pins/%s/%s/%s", runtime_path, shoggoth_id,
          resource_group_str, resource_name);

  if (!dir_exists(user_profile_path)) {
    result_t res_peers_with_pin =
        db_get_peers_with_pin(profile_ctx, shoggoth_id);
    char *peers_with_pin = UNWRAP(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    dht_t *peers = UNWRAP(res_peers);
    free(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      sprintf(location, "%s/explorer/profile/%s", peers->items[0]->host,
              shoggoth_id);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);

    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_404, MIME_TEXT_HTML);

      char *body = "profile not found";

      sonic_response_set_body(resp, body, (u64)strlen(body));

      sonic_send_response(req, resp);

      sonic_free_server_response(resp);
    }

    free_dht(peers);
    return;
  }

  if (!dir_exists(resource_path)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "resource not found";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  char *content = NULL;

  bool is_file = false;

  char within_resource_path[FILE_PATH_SIZE];
  sprintf(within_resource_path, "%s%s", resource_path, inner_path);

  if (dir_exists(within_resource_path)) {
    is_file = false;
  } else if (file_exists(within_resource_path)) {
    is_file = true;
  }

  bool is_dir = !is_file;

  if (!is_inner) {
    result_t res_files_list = get_files_and_dirs_list(resource_path);
    files_list_t *files_list = UNWRAP(res_files_list);

    for (u64 i = 0; i < files_list->files_count; i++) {
      char link_url[FILE_PATH_SIZE];
      sprintf(link_url, "/explorer/profile/%s/%s/%s/%s", shoggoth_id,
              resource_group_str, resource_name, files_list->files[i]);

      char new_res[256];
      sprintf(new_res,
              "<a href=\\\"%s\\\"><div "
              "class=\\\"resource-item\\\"><b>%s</b></div></a>",
              link_url, files_list->files[i]);

      if (content == NULL) {
        content = malloc((strlen(new_res) + 1) * sizeof(char));
        strcpy(content, new_res);
      } else {
        content = realloc(content, (strlen(content) + strlen(new_res) + 1) *
                                       sizeof(char));
        strcat(content, new_res);
      }
    }

    if (files_list->files_count == 0) {
      char new_res[256];
      sprintf(new_res, "empty directory");

      content = malloc((strlen(new_res) + 1) * sizeof(char));
      strcpy(content, new_res);
    }

    if (content == NULL) {
      content = "";
    }

    free_files_list(files_list);
  } else {
    bool found = false;

    if (dir_exists(within_resource_path) || file_exists(within_resource_path)) {
      found = true;
    }

    if (found) {
      if (is_dir) {
        result_t res_files_list = get_files_and_dirs_list(within_resource_path);
        files_list_t *files_list = UNWRAP(res_files_list);

        for (u64 i = 0; i < files_list->files_count; i++) {

          char link_url[FILE_PATH_SIZE];
          sprintf(link_url, "/explorer/profile/%s/%s/%s%s/%s", shoggoth_id,
                  resource_group_str, resource_name, inner_path,
                  files_list->files[i]);

          char new_res[512];
          sprintf(new_res,
                  "<a href=\\\"%s\\\"><div "
                  "class=\\\"resource-item\\\"><b>%s</b></div></a>",
                  link_url, files_list->files[i]);

          if (content == NULL) {
            content = malloc((strlen(new_res) + 1) * sizeof(char));
            strcpy(content, new_res);
          } else {
            content = realloc(content, (strlen(content) + strlen(new_res) + 1) *
                                           sizeof(char));
            strcat(content, new_res);
          }
        }

        if (files_list->files_count == 0) {
          char new_res[256];
          sprintf(new_res, "empty directory");

          content = malloc((strlen(new_res) + 1) * sizeof(char));
          strcpy(content, new_res);
        }

        if (content == NULL) {
          content = "";
        }

        free_files_list(files_list);
      } else if (is_file) {
        result_t res_file_size = get_file_size(within_resource_path);
        u64 file_size = UNWRAP_U64(res_file_size);

        u64 file_size_limit = 100000; // 100KB

        if (file_size < file_size_limit) {
          result_t res_is_plain_text = is_file_plain_text(within_resource_path);
          bool is_plain_text = UNWRAP_BOOL(res_is_plain_text);

          if (!is_plain_text) {
            char new_res[256];
            sprintf(new_res, "binary file");

            content = malloc((strlen(new_res) + 1) * sizeof(char));
            strcpy(content, new_res);
          } else {
            result_t res_file_text = read_file_to_string(within_resource_path);
            char *file_text = UNWRAP(res_file_text);

            // sanitizes the string to be used as a json value
            char *escaped_json_string = escape_json_string(file_text);
            free(file_text);

            // sanitizes the string to be plain text with html tags escaped
            char *escaped_html_string = escape_html_tags(escaped_json_string);
            free(escaped_json_string);

            content = malloc((strlen(escaped_html_string) + 1) * sizeof(char));
            strcpy(content, escaped_html_string);

            free(escaped_html_string);
          }
        } else {
          char new_res[256];
          sprintf(new_res, "file too large");

          content = malloc((strlen(new_res) + 1) * sizeof(char));
          strcpy(content, new_res);
        }
      }
    } else {
      char new_res[256];
      sprintf(new_res, "not found");

      content = malloc((strlen(new_res) + 1) * sizeof(char));
      strcpy(content, new_res);
    }
  }

  char *tabs_str = malloc(512 * sizeof(char));

  if (resource_group == CODE) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item nav-active'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == MODELS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item nav-active'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == DATASETS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item nav-active'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == PAPERS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item nav-active'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  }

  char *main_content = malloc((strlen(content) + 100) * sizeof(char));

  if (is_dir) {
    sprintf(main_content, "<div class=\\\"resource-list\\\">");
    strcat(main_content, content);
    strcat(main_content, "</div>");
  } else {
    sprintf(main_content, "<div class=\\\"file-view\\\"><pre><code>");
    strcat(main_content, content);
    strcat(main_content, "</code></pre></div>");
  }

  free(content);

  char *template_data = NULL;

  if (!is_inner) {
    template_data = malloc((strlen(shoggoth_id) + strlen(resource_group_str) +
                            strlen(resource_name) + strlen(shoggoth_id) +
                            strlen(main_content) + strlen(tabs_str) + 1 + 128) *
                           sizeof(char));

    sprintf(template_data,
            "{\"title\": \"%s/%s/%s\", \"shoggoth_id\": \"%s\", "
            "\"main_content\": \"%s\", \"tabs\": \"%s\"}",
            shoggoth_id, resource_group_str, resource_name, shoggoth_id,
            main_content, tabs_str);
  } else {
    template_data = malloc((strlen(shoggoth_id) + strlen(resource_group_str) +
                            strlen(resource_name) + strlen(shoggoth_id) +
                            strlen(main_content) + strlen(tabs_str) + 1 + 128) *
                           sizeof(char));

    sprintf(template_data,
            "{\"title\": \"%s/%s/%s%s\", \"shoggoth_id\": \"%s\", "
            "\"main_content\": \"%s\", \"tabs\": \"%s\"}",
            shoggoth_id, resource_group_str, resource_name, inner_path,
            shoggoth_id, main_content, tabs_str);
  }

  free(tabs_str);
  free(main_content);

  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(profile_ctx, explorer_dir);

  char head_template_path[FILE_PATH_SIZE];
  sprintf(head_template_path, "%s/templates/head.html", explorer_dir);

  char end_template_path[FILE_PATH_SIZE];
  sprintf(end_template_path, "%s/templates/end.html", explorer_dir);

  char profile_template_path[FILE_PATH_SIZE];
  sprintf(profile_template_path, "%s/templates/profile.html", explorer_dir);

  result_t res_end_template_string = read_file_to_string(end_template_path);
  char *end_template_string = UNWRAP(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  free(end_template_string);

  result_t res_head_template_string = read_file_to_string(head_template_path);
  char *head_template_string = UNWRAP(res_head_template_string);

  char *json_template =
      "{\"title\": \"Shoggoth Explorer - %s\", \"desc\": \"%s on Shoggoth - "
      "Shoggoth is a peer-to-peer, anonymous network for publishing and "
      "distributing open-source code, Machine Learning models, datasets, and "
      "research papers.\", \"og_url\": \"%s/explorer/profile_og/%s/%s/%s%s\", "
      "\"url\": \"%s/explorer/profile/%s/%s/%s%s\", \"is_profile\": true}";
  int json_len = snprintf(NULL, 0, json_template, shoggoth_id, shoggoth_id,
                          profile_ctx->config->network.public_host, shoggoth_id,
                          resource_group_str, resource_name, inner_path,
                          profile_ctx->config->network.public_host, shoggoth_id,
                          resource_group_str, resource_name, inner_path);
  if (json_len < 0) {
    PANIC("snprintf failed");
  }
  size_t json_template_size = (size_t)json_len + 1;
  char *head_template_data = malloc(json_template_size * sizeof(char));
  snprintf(head_template_data, json_template_size, json_template, shoggoth_id,
           shoggoth_id, profile_ctx->config->network.public_host, shoggoth_id,
           resource_group_str, resource_name, inner_path,
           profile_ctx->config->network.public_host, shoggoth_id,
           resource_group_str, resource_name, inner_path);

  template_t *head_template =
      create_template(head_template_string, head_template_data);
  free(head_template_string);

  result_t res_file_content = read_file_to_string(profile_template_path);
  char *file_content = UNWRAP(res_file_content);

  template_t *template_object = create_template(file_content, template_data);
  result_t res_add =
      template_add_partial(template_object, "head", head_template);
  UNWRAP(res_add);

  res_add = template_add_partial(template_object, "end", end_template);
  UNWRAP(res_add);

  result_t res_cooked = cook_template(template_object);
  char *cooked = UNWRAP(res_cooked);

  free_template(template_object);
  free_template(head_template);
  free_template(end_template);

  free(file_content);
  free(template_data);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, cooked, (u64)strlen(cooked));

  sonic_send_response(req, resp);

  free(cooked);
  sonic_free_server_response(resp);
}

void profile_home_redirect_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");

  char location[256];
  sprintf(location, "%s/explorer/profile/%s/code",
          profile_ctx->manifest->public_host, shoggoth_id);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_HTML);

  sonic_response_add_header(resp, "Location", location);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);
}

void profile_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *resource_group_str = sonic_get_path_segment(req, "resource_group");

  resource_group_t resource_group = GROUP_NULL;

  if (strcmp(resource_group_str, "code") == 0) {
    resource_group = CODE;
  } else if (strcmp(resource_group_str, "models") == 0) {
    resource_group = MODELS;
  } else if (strcmp(resource_group_str, "datasets") == 0) {
    resource_group = DATASETS;
  } else if (strcmp(resource_group_str, "papers") == 0) {
    resource_group = PAPERS;
  }

  if (resource_group == GROUP_NULL) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid resource group";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  if (strlen(shoggoth_id) != 36) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid Shoggoth ID";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(profile_ctx, runtime_path);

  char user_profile_path[FILE_PATH_SIZE];
  sprintf(user_profile_path, "%s/pins/%s/%s", runtime_path, shoggoth_id,
          resource_group_str);

  if (!dir_exists(user_profile_path)) {
    result_t res_peers_with_pin =
        db_get_peers_with_pin(profile_ctx, shoggoth_id);
    char *peers_with_pin = UNWRAP(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    dht_t *peers = UNWRAP(res_peers);
    free(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      sprintf(location, "%s/explorer/profile/%s", peers->items[0]->host,
              shoggoth_id);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);

    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_404, MIME_TEXT_HTML);

      char *body = "profile not found";

      sonic_response_set_body(resp, body, (u64)strlen(body));

      sonic_send_response(req, resp);

      sonic_free_server_response(resp);
    }

    free_dht(peers);
    return;
  }

  char *resource_list = NULL;

  result_t res_files_list = get_files_and_dirs_list(user_profile_path);
  files_list_t *files_list = UNWRAP(res_files_list);

  for (u64 i = 0; i < files_list->files_count; i++) {
    if (strcmp(files_list->files[i], ".shoggoth") == 0) {
      continue;
    }

    char new_res[256];
    sprintf(new_res,
            "<a href='/explorer/profile/%s/%s/%s\'><div "
            "class='resource-item'><b>%s</b></div></a>",
            shoggoth_id, resource_group_str, files_list->files[i],
            files_list->files[i]);

    if (resource_list == NULL) {
      resource_list = malloc((strlen(new_res) + 1) * sizeof(char));
      strcpy(resource_list, new_res);
    } else {
      resource_list =
          realloc(resource_list,
                  (strlen(resource_list) + strlen(new_res) + 1) * sizeof(char));
      strcat(resource_list, new_res);
    }
  }

  if (files_list->files_count == 1) {
    char new_res[256];

    sprintf(new_res, "no resources found");

    resource_list = malloc((strlen(new_res) + 1) * sizeof(char));
    strcpy(resource_list, new_res);
  }

  if (resource_list == NULL) {
    resource_list = "";
  }

  free_files_list(files_list);

  char *tabs_str = malloc(512 * sizeof(char));

  if (resource_group == CODE) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item nav-active'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == MODELS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item nav-active'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == DATASETS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item nav-active'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  } else if (resource_group == PAPERS) {
    sprintf(tabs_str,
            "<a href='/explorer/profile/%s/code'><div "
            "class='profile-nav-item'>Code</div></a>"

            "<a href='/explorer/profile/%s/models'><div "
            "class='profile-nav-item'>ML Models</div></a>"

            "<a href='/explorer/profile/%s/datasets'><div "
            "class='profile-nav-item'>Datasets</div></a>"

            "<a href='/explorer/profile/%s/papers'><div "
            "class='profile-nav-item nav-active'>Papers</div></a>",
            shoggoth_id, shoggoth_id, shoggoth_id, shoggoth_id);
  }

  char *main_content = malloc((strlen(resource_list) + 100) * sizeof(char));
  sprintf(main_content, "<div class=\\\"resource-list\\\">");
  strcat(main_content, resource_list);
  strcat(main_content, "</div>");

  free(resource_list);

  char *template_data = malloc(
      (strlen(shoggoth_id) + strlen(resource_group_str) + strlen(shoggoth_id) +
       strlen(main_content) + strlen(tabs_str) + 1 + 128) *
      sizeof(char));

  sprintf(template_data,
          "{\"title\": \"%s/%s\", \"shoggoth_id\": \"%s\", "
          "\"main_content\": \"%s\", \"tabs\": \"%s\"}",
          shoggoth_id, resource_group_str, shoggoth_id, main_content, tabs_str);

  free(tabs_str);
  free(main_content);

  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(profile_ctx, explorer_dir);

  char head_template_path[FILE_PATH_SIZE];
  sprintf(head_template_path, "%s/templates/head.html", explorer_dir);

  char end_template_path[FILE_PATH_SIZE];
  sprintf(end_template_path, "%s/templates/end.html", explorer_dir);

  char profile_template_path[FILE_PATH_SIZE];
  sprintf(profile_template_path, "%s/templates/profile.html", explorer_dir);

  result_t res_end_template_string = read_file_to_string(end_template_path);
  char *end_template_string = UNWRAP(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  free(end_template_string);

  result_t res_head_template_string = read_file_to_string(head_template_path);
  char *head_template_string = UNWRAP(res_head_template_string);

  char *json_template =
      "{\"title\": \"Shoggoth Explorer - %s\", \"desc\": \"%s on Shoggoth - "
      "Shoggoth is a peer-to-peer, anonymous network for publishing and "
      "distributing open-source code, Machine Learning models, datasets, and "
      "research papers.\", \"og_url\": \"%s/explorer/profile_og/%s/%s\", "
      "\"url\": \"%s/explorer/profile/%s/%s\", \"is_profile\": true}";
  int json_len =
      snprintf(NULL, 0, json_template, shoggoth_id, shoggoth_id,
               profile_ctx->config->network.public_host, shoggoth_id,
               resource_group_str, profile_ctx->config->network.public_host,
               shoggoth_id, resource_group_str);
  if (json_len < 0) {
    PANIC("snprintf failed");
  }
  size_t json_template_size = (size_t)json_len + 1;
  char *head_template_data = malloc(json_template_size * sizeof(char));
  snprintf(head_template_data, json_template_size, json_template, shoggoth_id,
           shoggoth_id, profile_ctx->config->network.public_host, shoggoth_id,
           resource_group_str, profile_ctx->config->network.public_host,
           shoggoth_id, resource_group_str);

  template_t *head_template =
      create_template(head_template_string, head_template_data);
  free(head_template_string);

  result_t res_file_content = read_file_to_string(profile_template_path);
  char *file_content = UNWRAP(res_file_content);

  template_t *template_object = create_template(file_content, template_data);

  result_t res_partial =
      template_add_partial(template_object, "head", head_template);
  UNWRAP(res_partial);

  res_partial = template_add_partial(template_object, "end", end_template);
  UNWRAP(res_partial);

  result_t res_cooked = cook_template(template_object);
  char *cooked = UNWRAP(res_cooked);

  free_template(template_object);
  free_template(head_template);
  free_template(end_template);

  free(file_content);
  free(template_data);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, cooked, (u64)strlen(cooked));

  sonic_send_response(req, resp);

  free(cooked);
  sonic_free_server_response(resp);
}

void og_profile_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *resource_group_str = sonic_get_path_segment(req, "resource_group");

  resource_group_t resource_group = GROUP_NULL;

  if (strcmp(resource_group_str, "code") == 0) {
    resource_group = CODE;
  } else if (strcmp(resource_group_str, "models") == 0) {
    resource_group = MODELS;
  } else if (strcmp(resource_group_str, "datasets") == 0) {
    resource_group = DATASETS;
  } else if (strcmp(resource_group_str, "papers") == 0) {
    resource_group = PAPERS;
  }

  if (resource_group == GROUP_NULL) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid resource group";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  if (strlen(shoggoth_id) != 36) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid Shoggoth ID";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  u64 title_size = strlen(shoggoth_id) + strlen(resource_group_str) + 5;

  char *og_title = malloc(title_size * sizeof(char));
  sprintf(og_title, "%s / %s", shoggoth_id, resource_group_str);

  char *og_desc;
  if (resource_group == CODE) {
    og_desc = "View Code on Shoggoth";
  } else if (resource_group == MODELS) {
    og_desc = "View ML Models on Shoggoth";
  } else if (resource_group == DATASETS) {
    og_desc = "View Datasets on Shoggoth";
  } else if (resource_group == PAPERS) {
    og_desc = "View Papers on Shoggoth";
  }

  int image_size;
  unsigned char *cooked =
      generate_og_image(og_title, og_desc, 46, 24, 46, 32, &image_size);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  resp->content_type = MIME_IMAGE_PNG;
  sonic_response_set_body(resp, (char *)cooked, (u64)image_size);
  sonic_send_response(req, resp);

  free(cooked);
  free(og_title);
  sonic_free_server_response(resp);
}

void og_profile_resource_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *resource_group_str = sonic_get_path_segment(req, "resource_group");
  char *resource_name = sonic_get_path_segment(req, "resource_name");

  resource_group_t resource_group = GROUP_NULL;

  if (strcmp(resource_group_str, "code") == 0) {
    resource_group = CODE;
  } else if (strcmp(resource_group_str, "models") == 0) {
    resource_group = MODELS;
  } else if (strcmp(resource_group_str, "datasets") == 0) {
    resource_group = DATASETS;
  } else if (strcmp(resource_group_str, "papers") == 0) {
    resource_group = PAPERS;
  }

  if (resource_group == GROUP_NULL) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid resource group";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  if (strlen(shoggoth_id) != 36) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid Shoggoth ID";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  u64 title_size = strlen(shoggoth_id) + strlen(resource_group_str) +
                   strlen(resource_name) + 10;

  char *og_title = malloc(title_size * sizeof(char));
  sprintf(og_title, "%s / %s / %s", shoggoth_id, resource_group_str,
          resource_name);

  ellipsis_text(og_title, 70);

  char *og_desc;
  if (resource_group == CODE) {
    og_desc = "View Code on Shoggoth";
  } else if (resource_group == MODELS) {
    og_desc = "View ML Models on Shoggoth";
  } else if (resource_group == DATASETS) {
    og_desc = "View Datasets on Shoggoth";
  } else if (resource_group == PAPERS) {
    og_desc = "View Papers on Shoggoth";
  }

  int image_size;
  unsigned char *cooked =
      generate_og_image(og_title, og_desc, 46, 24, 46, 32, &image_size);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  resp->content_type = MIME_IMAGE_PNG;
  sonic_response_set_body(resp, (char *)cooked, (u64)image_size);
  sonic_send_response(req, resp);

  free(cooked);
  free(og_title);
  sonic_free_server_response(resp);
}

void add_profile_routes(node_ctx_t *ctx, sonic_server_t *server) {
  profile_ctx = ctx;

  sonic_add_route(server, "/explorer/profile/{shoggoth_id}", METHOD_GET,
                  profile_home_redirect_route);

  sonic_add_route(server, "/explorer/profile/{shoggoth_id}/{resource_group}",
                  METHOD_GET, profile_route);
  sonic_add_route(
      server,
      "/explorer/profile/{shoggoth_id}/{resource_group}/{resource_name}",
      METHOD_GET, profile_resource_route);

  sonic_add_route(
      server,
      "/explorer/profile/{shoggoth_id}/{resource_group}/{resource_name}/{*}",
      METHOD_GET, profile_resource_route);

  sonic_add_route(server, "/explorer/profile_og/{shoggoth_id}/{resource_group}",
                  METHOD_GET, og_profile_route);
  sonic_add_route(
      server,
      "/explorer/profile_og/{shoggoth_id}/{resource_group}/{resource_name}",
      METHOD_GET, og_profile_resource_route);
}