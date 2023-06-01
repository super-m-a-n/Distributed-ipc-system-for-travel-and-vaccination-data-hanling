/*file : skip_list.h*/
#pragma once
#include <stdbool.h>

typedef struct skip_list_node * SkipListNode;
typedef struct skip_list * SkipList;

/* create a skip_list and return a pointer to the structure */
SkipList skip_list_create(int max_level, float prob);
/* search the skip list for a specific value */
bool skip_list_search(SkipList skip_list, char * value, char ** date);
/* insert given data into skip list*/
void skip_list_insert(SkipList skip_list, void * data, char * date);
/* function that returns a random level for a new node , given a probability inside the skip-list structure */
int random_level(SkipList skip_list);
/* delete node with given value */
void skip_list_delete(SkipList skip_list, char * value);
/* delete the skip_list structure and all of its components*/
void skip_list_destroy(SkipList skip_list);
/* prints all the levels of the skip_list (for debugging purposes) */
void skip_list_print(SkipList skip_list);
/* prints the data of all the nodes of the skip_list */
void skip_list_print_data(SkipList skip_list);

