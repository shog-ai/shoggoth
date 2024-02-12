/**** openssl.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../utils/utils.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#define RSA_KEY_BITS 2048

/****
 * generates an RSA key-pair
 *
 ****/
result_t openssl_generate_key_pair(const char *private_key_file,
                                   const char *public_key_file) {
  RSA *rsa_keypair = NULL;
  FILE *private_key_fp = NULL;
  FILE *public_key_fp = NULL;

  // Generate a new RSA key pair
  rsa_keypair = RSA_new();
  BIGNUM *e = BN_new();
  BN_set_word(e, RSA_F4);

  if (RSA_generate_key_ex(rsa_keypair, RSA_KEY_BITS, e, NULL) != 1) {
    return ERR("RSA_generate_key_ex failed");
  }

  // Write the private key to a file
  private_key_fp = fopen(private_key_file, "w");
  if (!private_key_fp) {
    PANIC("fopen private key failed");
  }

  if (!PEM_write_RSAPrivateKey(private_key_fp, rsa_keypair, NULL, NULL, 0, NULL,
                               NULL)) {
    return ERR("PEM_write_RSAPrivateKey failed");
  }

  fclose(private_key_fp);

  // Write the public key to a file
  public_key_fp = fopen(public_key_file, "w");
  if (!public_key_fp) {
    ERR("fopen public key failed");
  }

  if (!PEM_write_RSAPublicKey(public_key_fp, rsa_keypair)) {
    ERR("PEM_write_RSAPublicKey");
  }

  fclose(public_key_fp);
  RSA_free(rsa_keypair);
  BN_free(e);

  return OK(NULL);
}

/****U
 * generates a SHA256 hash of a byte array
 *
 ****/
result_t openssl_hash_data(char *data, size_t data_len) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  char *hash_str = (char *)malloc(2 * SHA256_DIGEST_LENGTH + 1);

  SHA256_Init(&sha256);
  SHA256_Update(&sha256, data, data_len);
  SHA256_Final(hash, &sha256);

  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(&hash_str[i * 2], "%02x", hash[i]);
  }

  hash_str[2 * SHA256_DIGEST_LENGTH] = '\0'; // Null-terminate the string

  return OK(hash_str);
}

// Function to sign data using an RSA private key
result_t openssl_sign_data(char *private_key_path, char *data) {
  // Load the RSA private key from the file
  FILE *keyFile = fopen(private_key_path, "r");
  if (keyFile == NULL) {
    perror("Error opening private key file");
    return ERR("Error opening private key file");
  }

  RSA *rsa = RSA_new();
  rsa = PEM_read_RSAPrivateKey(keyFile, &rsa, NULL, NULL);
  fclose(keyFile);

  if (rsa == NULL) {
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  // Create an EVP_PKEY structure from the RSA key
  EVP_PKEY *pkey = EVP_PKEY_new();
  if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
    RSA_free(rsa);
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  // Create a signature context
  EVP_MD_CTX *md_ctx = EVP_MD_CTX_create();
  if (md_ctx == NULL) {
    EVP_PKEY_free(pkey);
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  // Initialize the signature context with the EVP_PKEY
  if (EVP_SignInit(md_ctx, EVP_sha256()) != 1) {
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_destroy(md_ctx);
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  // Sign the data
  if (EVP_SignUpdate(md_ctx, data, strlen(data)) != 1) {
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_destroy(md_ctx);
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  // Get the length of the signature
  unsigned int sig_len = (unsigned int)EVP_PKEY_size(pkey);
  unsigned char *signature = (unsigned char *)malloc(sig_len);

  if (EVP_SignFinal(md_ctx, signature, &sig_len, pkey) != 1) {
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_destroy(md_ctx);
    free(signature);
    ERR_print_errors_fp(stderr);
    return ERR("sign data failed");
  }

  EVP_PKEY_free(pkey);
  EVP_MD_CTX_destroy(md_ctx);

  // Convert the signature to a hexadecimal string
  char *hex_signature = (char *)malloc(sig_len * 2 + 1);
  for (size_t i = 0; i < sig_len; i++) {
    sprintf(hex_signature + (i * 2), "%02x", signature[i]);
  }
  hex_signature[sig_len * 2] = '\0';

  free(signature);

  return OK(hex_signature);
}

result_t openssl_verify_signature(const char *public_key,
                                  const char *signature_hex, const char *data) {
  // Create a BIO object to read the base64-encoded public key
  BIO *bio = BIO_new(BIO_s_mem());
  if (bio == NULL) {
    ERR_print_errors_fp(stderr);
    return ERR("create BIO failed");
  }

  if (BIO_write(bio, public_key, (int)strlen(public_key)) <= 0) {
    BIO_free(bio);
    ERR_print_errors_fp(stderr);
    return ERR("BIO write failed");
  }

  RSA *rsa = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);
  BIO_free(bio);

  // Create an EVP_PKEY structure from the RSA public key
  EVP_PKEY *pkey = EVP_PKEY_new();
  if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
    RSA_free(rsa);
    ERR_print_errors_fp(stderr);
    return ERR("create key failed");
  }

  // Create a verification context
  EVP_MD_CTX *md_ctx = EVP_MD_CTX_create();
  if (md_ctx == NULL) {
    EVP_PKEY_free(pkey);
    ERR_print_errors_fp(stderr);
    return ERR("create context failed");
  }

  // Initialize the verification context with the EVP_PKEY
  if (EVP_VerifyInit(md_ctx, EVP_sha256()) != 1) {
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_destroy(md_ctx);
    ERR_print_errors_fp(stderr);
    return ERR("init context failed");
  }

  // Load the signature in hexadecimal format
  size_t sig_len = strlen(signature_hex) / 2;
  unsigned char *signature = (unsigned char *)malloc(sig_len);

  for (size_t i = 0; i < sig_len; i++) {
    sscanf(signature_hex + 2 * i, "%02hhx", &signature[i]);
  }

  // Load the data that was signed
  if (EVP_VerifyUpdate(md_ctx, data, strlen(data)) != 1) {
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_destroy(md_ctx);
    free(signature);
    ERR_print_errors_fp(stderr);
    return ERR("load data failed");
  }

  // Perform the signature verification
  int verifyResult =
      EVP_VerifyFinal(md_ctx, signature, (unsigned int)sig_len, pkey);

  // Cleanup resources
  EVP_PKEY_free(pkey);
  EVP_MD_CTX_destroy(md_ctx);
  free(signature);

  return OK_INT(verifyResult);
}
