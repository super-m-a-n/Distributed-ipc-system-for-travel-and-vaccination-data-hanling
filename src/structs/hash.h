/*file : hash.h */
#pragma once
#include "list.h"

typedef struct hash_table * HT;

// a simple hash function for strings
unsigned long hash_function(unsigned char *str);
// creates hash table structure of given capacity
HT hash_create(int capacity, int type);
// returns number of elements currently in hash table
int hash_size(HT hash);
// returns number of buckets currently in hash table
int hash_capacity(HT hash);
// deletes the hash table structure
void hash_destroy(HT hash);
// inserts entry with given value
void hash_insert(HT hash, void * value);
// searches for entry with given key
void * hash_search(HT hash, void * key);
//print hash table (debugging)
void hash_print(HT hash);
// function that is used to iterate through hash table
void * hash_iterate_next(HT hash);