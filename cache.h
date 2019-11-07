#import <stdlib.h>

#ifndef CACHE_H
#define CACHE_H

/* Recommended max cache and object sizes */
// #define MAX_CACHE_SIZE 1049000
#define MAX_CACHE_SIZE 10
#define MAX_OBJECT_SIZE 102400

typedef struct {
    int timestamp;
    int valid;
    char *key;   // uri for web proxy
    char *value; // response for web proxy
} cache_entry_t;

cache_entry_t cache[MAX_CACHE_SIZE];
int cache_time;


void init_cache();
void free_cache();
int  cache_lookup(char *key);
void cache_insert(char *key, char *value);
void test_cache();
void print_cache();

#endif
