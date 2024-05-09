#include "jansson.h"
#include "../janssonpath.h"
#include <stdio.h>

int main(int argn,char**argv){
    if(argn<3)return -1;
    puts("path:");
    puts(argv[2]);
    json_error_t error;
    json_t * json=json_load_file(argv[1],0,&error);
    json_t *result=json_path_get(json,argv[2]);
    char * r=json_dumps(result,JSON_INDENT(2)|JSON_ENCODE_ANY);
    puts("result:");
    if(r)puts(r);
    json_decref(result);
    json_decref(json);
    free(r);
    return 0;
}
