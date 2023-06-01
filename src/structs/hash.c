/*file : hash.c*/
#include <stdlib.h>
#include <stdio.h>
#include "hash.h"
#include "list.h"
#include "m_items.h"
#include "tm_items.h"
#include <assert.h>

#define MAX_LOAD_FACTOR 0.75

// data struct for hash table
struct hash_table {
	List *table;		// hash table implemented as an array of linked lists
	int size;			// number of elements added
	int capacity;		// number of buckets
	int type;			// 0 : table of lists of citizens info nodes. 1: table of lists of virus info nodes(parent) 2 : table of lists of countries info nodes (parent) 4 : table of lists of virus info nodes (child) 5 : table of lists of country info nodes (child)
};

unsigned long hash_function(unsigned char *str) {
	unsigned long hash = 5381;
	int c; 
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

HT hash_create(int capacity, int type)
{
	//malloc HT structure
	HT hash = malloc(sizeof(struct hash_table));
	if (hash == NULL)
		fprintf(stderr, "Error : hash_create -> malloc\n");
	assert(hash != NULL);

	// malloc hash table of given size, of pointers to lists
  	hash->table = malloc(capacity * sizeof(List));
	if (hash->table == NULL)
		fprintf(stderr, "Error : hash_create -> malloc\n");
	assert(hash->table != NULL);

	// initialize all list pointers of hashtable to NULL
  	for (int i = 0; i < capacity; ++i)
    	hash->table[i] = NULL;

    hash->capacity = capacity;
  	hash->size = 0;
  	hash->type = type;

	return hash;
}

int hash_size(HT hash)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_size -> HT hash is NULL\n");
	assert(hash != NULL);

	return hash->size;
}

int hash_capacity(HT hash)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_capacity -> HT hash is NULL\n");
	assert(hash != NULL);

	return hash->capacity;
}

void hash_destroy(HT hash)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_destroy -> HT hash is NULL\n");
	assert(hash != NULL);

	for (int i = 0; i < hash->capacity; ++i)
	{
		if (hash->table[i] != NULL)
			list_destroy(hash->table[i]);		//delete all linked lists of hash table
	}

	// at last, delete the hash and hash_table data structure
	free(hash->table);
	free(hash);
}

void * hash_search(HT hash, void * key)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_search -> HT hash is NULL\n");
	assert(hash != NULL);

	int index = (int) (hash_function((unsigned char *) key) % hash->capacity);

	if (hash->table[index] == NULL) 		// if no previous entry has hashed into that bucket
		return NULL;  						// then obviously given key does not exist into the hash-table
	else
		return list_search(hash->table[index], key);
}

// if load factor becomes too large, rehash the hash table by doubling its capacity
static void rehash(HT hash)
{
	// keep previous capacity, array of lists
	int prev_capacity = hash->capacity;
	List * prev_table = hash->table;

	hash->capacity = hash->capacity*2; 		// double hash table's capacity

	hash->table = malloc(hash->capacity * sizeof(List));  // malloc new hash table with double capacity
	if (hash->table == NULL)
		fprintf(stderr, "Error : rehash -> malloc\n");
	assert(hash->table != NULL);

	// re-initialize all list pointers of hashtable to NULL
  	for (int i = 0; i < hash->capacity; ++i)
    	hash->table[i] = NULL;

    hash->size = 0;

    // now re-insert all previous elements of the hash table to the bigger hash-table
    for (int i = 0; i < prev_capacity; i++)
    {
    	if (prev_table[i] != NULL)
    	{
    		for (ListNode node = list_first(prev_table[i]); node != NULL; node = list_next(prev_table[i], node))
    			hash_insert(hash, list_value(prev_table[i], node));
    	}
    }

    // finally, delete the previous hash-table
    // Also delete the list nodes of previous hash-tables, but not their data, since the list nodes of new hash table point to that data
    for (int i = 0; i < prev_capacity; ++i)
	{
		if (prev_table[i] != NULL)		// for each not null list of previous hash-table
		{
			ListNode node = list_dummy(prev_table[i]); 		// starting from fake-dummy node
			
			while (node != NULL) 	// for every node of list
			{				
				ListNode next = list_next(prev_table[i], node);		// save next node
				free(node);   // free node of list
				node = next;  // continue iteration of list
			}
			// at last free the struct of list
			free(prev_table[i]);
		}
	}
	free(prev_table);		// delete the hash_table itself
}

void hash_insert(HT hash, void * value)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_search -> HT hash is NULL\n");
	assert(hash != NULL);
	void * key;
	
	switch (hash->type)
	{
		case 0 : key = m_get_citizen_id((M_CitizenInfo) value); break;
		case 1 : key = m_get_virus_name((M_VirusInfo) value); break;
		case 2 : key = m_get_country_name((M_CountryInfo) value); break;
		case 4 : key = tm_get_virus_name((TM_VirusInfo) value); break;
		case 5 : key = tm_get_country_name((TM_CountryInfo) value); break;
	}

	int index = (int) (hash_function((unsigned char *) key) % hash->capacity);

	if (hash->table[index] == NULL) 		// if no previous entry has hashed into that bucket
		hash->table[index] = list_create(hash->type);		// create new bucket-list at index
	
	list_insert_end(hash->table[index], value);			// insert value at end of list
	hash->size++;

	// If after insertion, load factor becomes too large, rehash the hash table
	float load_factor = (float) hash->size / hash->capacity;
	if (load_factor > MAX_LOAD_FACTOR)
		rehash(hash);
}

void hash_print(HT hash)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_print -> HT hash is NULL\n");
	assert(hash != NULL);

	for (int i = 0; i < hash->capacity; ++i)
	{
		if (hash->table[i] != NULL)
		{
			printf("%d:chain \n\n", i);
			list_print(hash->table[i]);
		}
	}

}

void * hash_iterate_next(HT hash)
{
	if (hash == NULL)
		fprintf(stderr, "Error : hash_iterate_next -> HT hash is NULL\n");
	assert(hash != NULL);

	static ListNode cur_node = NULL;
	static int index = 0;

	if (cur_node == NULL)				// if the iteration of hash-table begins now
	{
		for (int i = 0; i < hash->capacity; ++i)
		{
			if (hash->table[i] != NULL)
			{
				cur_node = list_first(hash->table[i]);
				index = i;
				return (list_value(hash->table[i], cur_node));
			}
		}

		return NULL;		// no elements found
	}
	else
	{	// iteration has already begun
		if (list_next(hash->table[index], cur_node) != NULL)	// if current list's next element is not NULL
		{
			cur_node = list_next(hash->table[index], cur_node);		// save next node
			return (list_value(hash->table[index], cur_node));		// return next element
		}
		else
		{	// we have reached end of current list
			for (int i = index+1; i < hash->capacity; ++i)  // search for next non-empty list of hash-table
			{
				if (hash->table[i] != NULL)					// if you find such a list
				{
					cur_node = list_first(hash->table[i]);	// save first node , to start traversing the list
					index = i;
					return (list_value(hash->table[i], cur_node));	// and obviously return the element
				}
			}

			// reached end of iteration over all entries of hash table
			cur_node = NULL;		// re-initialize cur_node, index to NULL, 0 for any iteration that may follow
			index = 0;	
			return NULL;			// no remaining elements found
		}
	}

	return NULL;

}
