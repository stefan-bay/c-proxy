#include <limits.h>
#include <stdio.h>
#include <strings.h>
#include "cache.h"

void test_cache() {
    init_cache();
    cache_insert("key", "value");
    cache_insert("stuff", "things");
    cache_insert("good", "gracious");
    print_cache();
    printf("\n");

    printf("insert: 'new': 'stuff'\n");
    cache_insert("new", "stuff");
    printf("oldest should be 'key': 'value'\n");
    print_cache();
}

void print_cache() {
    printf("CACHE:\n");
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
        printf("%s....%d\n", cache[i].key, cache[i].timestamp);
}

void init_cache() {
    cache_time = 0;
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        cache[i].key = (char *)malloc(MAX_OBJECT_SIZE);
        cache[i].value = (char *)malloc(MAX_OBJECT_SIZE);
        cache[i].valid = 0;
    }
}

void free_cache() {
    printf("freeing cache\n");
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        free(cache[i].key);
        free(cache[i].value);
    }
}

int cache_lookup(char *key) {
    cache_time++;
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (cache[i].valid && (strcmp(cache[i].key, key) == 0)) {
            cache[i].timestamp = cache_time;
            return i;
        }
    }
    return -1;
}

void cache_insert(char *key, char *value) {
    cache_time++;
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (!cache[i].valid) {
            cache[i].timestamp = cache_time;
            strcpy(cache[i].key, key);
            strcpy(cache[i].value, value);
            cache[i].valid = 1;
            return;
        }
    }
    // find oldest entry in cache
    int oldest = INT_MAX;
    int oldest_index = 0;
    int i;
    for (i = 0; i < MAX_CACHE_SIZE; i++) {
        if (cache[i].timestamp < oldest) {
            oldest = cache[i].timestamp;
            oldest_index = i;
        }
    }
    printf("making room in cache\n");
    printf("swapping %s with %s\n", cache[oldest_index].key, key);
    strcpy(cache[oldest_index].key, key);
    strcpy(cache[oldest_index].value, value);
    cache[oldest_index].timestamp = cache_time;
}
