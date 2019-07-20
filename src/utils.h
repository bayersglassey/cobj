#ifndef _COBJ_UTILS_H_
#define _COBJ_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static int strlen_of_int(int i){
    /* Basically log(i), except that strlen of "0" is 1, and strlen of a
    negative number includes a space for the '-' */
    if(i == 0)return 1;
    if(i < 0)return strlen_of_int(-i) + 1;
    int len = 0;
    while(i != 0){
        len++;
        i /= 10;
    }
    return len;
}

static void strncpy_of_int(char *s, int i, int i_len){
    /* i_len should be strlen_of_int(i) */
    if(i == 0){
        *s = '0';
        return;
    }else if(i < 0){
        *s = '-';
        strncpy_of_int(s+1, -i, i_len-1);
        return;
    }
    while(i_len > 0){
        s[i_len - 1] = '0' + i % 10;
        i /= 10;
        i_len--;
    }
}


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