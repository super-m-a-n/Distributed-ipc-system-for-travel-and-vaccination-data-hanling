/*file : list.h*/
#pragma once

typedef struct list * List;
typedef struct list_node * ListNode;

// creates an empty list
List list_create(int type);
// destroys list
void list_destroy(List list);
// returns size of list
int list_size(List list);
// inserts node with given value after given already existing node
void list_insert_next(List list, ListNode node, void * value);
// inserts node with given value at end of list
void list_insert_end(List list, void * value);
// finds and returns data with given value
void * list_search(List list, void * key);
// returns first node of list
ListNode list_first(List list);
// returns dummy node of list
ListNode list_dummy(List list);
// returns next node from the one given
ListNode list_next(List list, ListNode node);
// returns value of given node
void * list_value(List list, ListNode node);
// prints list (debugging)
void list_print(List list);