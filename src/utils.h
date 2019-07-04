#ifndef _COBJ_UTILS_H_
#define _COBJ_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static char *load_file(const char* filename, size_t *size_ptr){
    const char *ERRMSG = "nothing";
    FILE *file = fopen(filename, "r");
    if(!file){
        ERRMSG = "fopen";
        goto err;
    }
    if(fseek(file, 0, SEEK_END)){
        ERRMSG = "fseek";
        goto err;
    }
    long end = ftell(file);
    if(end < 0){
        ERRMSG = "ftell";
        goto err;
    }
    if(fseek(file, 0, 0)){
        ERRMSG = "fseek";
        goto err;
    }
    size_t size = end;
    char *buffer = malloc(size);
    if(!buffer){
        fprintf(stderr, "Couldn't allocate file buffer (%zu bytes)\n", size);
        ERRMSG = "malloc";
        goto err;
    }
    if(!fread(buffer, size, 1, file)){
        fprintf(stderr, "Couldn't read %zu bytes into file buffer\n", size);
        ERRMSG = "fread";
        goto err;
    }
    if(fclose(file)){
        ERRMSG = "fclose";
        goto err;
    }

    *size_ptr = size;
    return buffer;

err:
    fprintf(stderr, "load_file(%s): ", filename);
    perror(ERRMSG);
    return NULL;
}

#endif