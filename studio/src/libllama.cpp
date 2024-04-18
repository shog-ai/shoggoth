#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../lib/llamacpp-include/common.h"
#include "../lib/llamacpp-include/llama.h"

int run_llamacpp() {
  gpt_params params;
  params.n_ctx = 0;

  if (!gpt_params_parse(argc, argv, params))
    return 1;

  if (params.prompt.empty())
    params.prompt = "The";

  llama_backend_init();

  llama_model_params model_params = llama_model_default_params();
  model_params.n_gpu_layers = llamafile_gpu_layers(35);
  llama_model *model =
      llama_load_model_from_file(params.model.c_str(), model_params);
  if (model == NULL)
    return 2;

  llama_context_params ctx_params =
      llama_context_params_from_gpt_params(params);
  llama_context *ctx = llama_new_context_with_model(model, ctx_params);
  if (ctx == NULL)
    return 3;

  printf("%s", params.prompt.c_str());
  int n_past = 0;
  bool add_bos = llama_should_add_bos_token(llama_get_model(ctx));
  eval_string(ctx, params.prompt.c_str(), params.n_batch, &n_past, add_bos);
  struct llama_sampling_context *ctx_sampling =
      llama_sampling_init(params.sparams);
  for (;;) {
    llama_token id = llama_sampling_sample(ctx_sampling, ctx, NULL);
    llama_sampling_accept(ctx_sampling, ctx, id, true);
    if (id == llama_token_eos(model))
      break;
    printf("%s", llama_token_to_piece(ctx, id).c_str());
    fflush(stdout);
    if (!eval_id(ctx, id, &n_past))
      break;
  }
  printf("\n");

  llama_sampling_free(ctx_sampling);
  llama_free(ctx);
  llama_free_model(model);
  llama_backend_free();
}
// #include "../lib/llamacpp/llama.h"
// #include "../lib/llamacpp/common/common.h"

// static bool eval_tokens(struct llama_context *ctx_llama,
//                         std::vector<llama_token> tokens, int n_batch,
//                         int *n_past) {
//   int N = (int)tokens.size();
//   for (int i = 0; i < N; i += n_batch) {
//     int n_eval = (int)tokens.size() - i;
//     if (n_eval > n_batch)
//       n_eval = n_batch;
//     if (llama_decode(ctx_llama,
//                      llama_batch_get_one(&tokens[i], n_eval, *n_past, 0)))
//       return false; // probably ran out of context
//     *n_past += n_eval;
//   }
//   return true;
// }

// static bool eval_id(struct llama_context *ctx_llama, int id, int *n_past) {
//   std::vector<llama_token> tokens;
//   tokens.push_back(id);
//   return eval_tokens(ctx_llama, tokens, 1, n_past);
// }

// static bool eval_string(struct llama_context *ctx_llama, const char *str,
//                         int n_batch, int *n_past, bool add_bos) {
//   std::string str2 = str;
//   std::vector<llama_token> embd_inp =
//       ::llama_tokenize(ctx_llama, str2, add_bos);
//   return eval_tokens(ctx_llama, embd_inp, n_batch, n_past);
// }
