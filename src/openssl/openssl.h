/**** openssl.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef NODE_OPENSSL_H
#define NODE_OPENSSL_H

#include "../utils/utils.h"

result_t openssl_verify_signature(char *public_key_path, char *signature_hex,
                                  char *data);

result_t openssl_generate_key_pair(const char *private_key_file,
                                   const char *public_key_file);

result_t openssl_hash_data(char *data, unsigned long long data_len);

result_t openssl_sign_data(char *private_key_path, char *data);

#endif
