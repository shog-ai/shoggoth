#include "../../../src/include/camel.h"
#include "../../../src/utils/utils.h"

#include <stdlib.h>

void test_utils_strip_public_key(test_t *test) {
  char *public_key =
      "-----BEGIN RSA PUBLIC KEY-----\n"
      "MIIBCgKCAQEAu/Jk9NAQatRhcc0cLi2yQn/6btJSE9/mUpHfyoUBcYC+/qXhxTaC\n"
      "zqpl5QmNef5RDdYMORjPFYLFrGvvzsPJ204mmcLMuW90t/0nC3+4lMFkjMBiBaPC\n"
      "Mxr/0ESw6F2cP2GokxEjSQIhZm+hQ8sLFBeq2NB5iwqaMt3dnFJGG69hhhOaXdGf\n"
      "dbY8LPRU3Wuy7TXB7Eni82cza9mCSfagSpUvQBAWBXoel1KR+ssWyExFkz8zIABJ\n"
      "txPfGvwdnme+9BeZdKgZNBT5QkDNPbPP1mPoaostsBBiUJsScZ/gxrINEDNV8yhR\n"
      "N13ORv/WoSz7itTo/GNlpTnnPGbgQTftbQIDAQAB\n"
      "-----END RSA PUBLIC KEY-----\n";

  char *expected_stripped_key =
      "-----BEGIN RSA PUBLIC KEY-----\n"
      "MIIBCgKCAQEAu/Jk9NAQatRhcc0cLi2yQn/6btJSE9/mUpHfyoUBcYC+/qXhxTaC"
      "zqpl5QmNef5RDdYMORjPFYLFrGvvzsPJ204mmcLMuW90t/0nC3+4lMFkjMBiBaPC"
      "Mxr/0ESw6F2cP2GokxEjSQIhZm+hQ8sLFBeq2NB5iwqaMt3dnFJGG69hhhOaXdGf"
      "dbY8LPRU3Wuy7TXB7Eni82cza9mCSfagSpUvQBAWBXoel1KR+ssWyExFkz8zIABJ"
      "txPfGvwdnme+9BeZdKgZNBT5QkDNPbPP1mPoaostsBBiUJsScZ/gxrINEDNV8yhR"
      "N13ORv/WoSz7itTo/GNlpTnnPGbgQTftbQIDAQAB\n"
      "-----END RSA PUBLIC KEY-----\n";

  char *stripped_key = utils_strip_public_key(public_key);

  ASSERT_EQ_STR(stripped_key, expected_stripped_key, NULL);

  free(stripped_key);
}

void test_utils_extract_filename_from_path(test_t *test) {
  char *file_path = "$HOME/shoggoth/client/tmp/myfile.txt";

  const char *file_name = utils_extract_filename_from_path(file_path);

  ASSERT_EQ_STR(file_name, "myfile.txt", NULL);
}

void test_utils_get_file_extension(test_t *test) {
  char *file_path = "$HOME/shoggoth/client/tmp/myfile.txt";

  const char *file_extension = utils_get_file_extension(file_path);

  ASSERT_EQ_STR(file_extension, "txt", NULL);
}

void utils_test_suite(suite_t *suite) {
  ADD_TEST(test_utils_strip_public_key);
  ADD_TEST(test_utils_extract_filename_from_path);
  ADD_TEST(test_utils_get_file_extension);
}

int main(int argc, char **argv) {
  CAMEL_BEGIN(UNIT);

  ADD_SUITE(utils_test_suite);

  CAMEL_END();
}
