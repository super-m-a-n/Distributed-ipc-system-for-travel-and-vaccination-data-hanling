/*file : skip_list.c*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "skip_list.h"
#include "m_items.h"
#include <assert.h>

/* data structure for skip list node */
struct skip_list_node {
	int level;       				// how high in terms of levels the skip list node is 
	SkipListNode * next_array;		// each node has an array of as many pointers to nodes as its level, which size is decided dynamically at creation
	M_CitizenInfo info;				// each node has a pointer to a citizen record, and the citizen id serves as a key;
	char * date;					// date of vaccination (NULL if person is not vaccinated)
};

/* data structure of skip list */
struct skip_list {
	SkipListNode header_dummy_node;		// this serves as a pointer to the first header/dummy node of skip_list (which has an array of head pointers for all pararell lists)
	int cur_level;						// the current height of the skip-list (the level of the top skip-list)
	int max_level;						// this is the maximum level-height for the top skip-list
	float prob;							// this is the probability that a new level is created for a skip-list node
};


SkipList skip_list_create(int max_level, float prob)
{
	SkipList skip_list = malloc(sizeof(struct skip_list));		// allocate memory for skip list data structure
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_create -> malloc\n");
	assert(skip_list != NULL);

	skip_list->max_level = max_level;		// assign the max level
	skip_list->prob = prob;
	skip_list->cur_level = 0;				// current level is 0 upon creation (we are at L0)

	skip_list->header_dummy_node = malloc(sizeof(struct skip_list_node));		// allocate memory for first node, which is the head-dummy node
	if (skip_list->header_dummy_node == NULL)
		fprintf(stderr, "Error : skip_list_create -> malloc\n");
	assert(skip_list->header_dummy_node != NULL);

	skip_list->header_dummy_node->next_array = malloc((max_level+1)*sizeof(SkipListNode));		// allocate memory for the array of head pointers of head node

	for (int i = 0; i <= max_level; ++i)		// initialize all header pointers to NULL
	{
		skip_list->header_dummy_node->next_array[i] = NULL;
	}

	skip_list->header_dummy_node->info = NULL;		// header-dummy node contains no real data-info
	skip_list->header_dummy_node->date = NULL;

	return skip_list;
}

bool skip_list_search(SkipList skip_list, char * value, char ** date)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_search -> skip list is NULL\n");
	assert(skip_list != NULL);

	int level = skip_list->cur_level;							// start searching from top current level
	SkipListNode cur_node = skip_list->header_dummy_node;		// start searching from the head node of the top level skip list
	SkipListNode next_node = NULL;

	while (level >= 0)
	{
		next_node = cur_node->next_array[level];		// we traverse the nodes of skip list of current level
		while (next_node != NULL)
		{
			int check; 				// check for equality of id's
			if (strlen((char *) value) == strlen((char *) m_get_citizen_id(next_node->info)))
				check = strcmp((char *) value, (char *) m_get_citizen_id(next_node->info));
			else
				check = (strlen((char *) value) > strlen((char *) m_get_citizen_id(next_node->info))) ? 1 : -1 ;

			if (!check)
			{
				*date = next_node->date;
				return true;
			}
			else if (check > 0)			// continue traversing on the same level while nodes have smaller value than the one we search for
			{				
				cur_node = next_node;
				next_node = next_node->next_array[level];
			}
			else						// stop traversing on current level if we go "too far" (nodes have bigger value than the one we search for)
				break;
		}

		level--;
	}

	return false;
}

int random_level(SkipList skip_list)
{
	int level = 0;
	float p = (float) rand() / (float) ((unsigned)RAND_MAX + 1);		// generate random probability in [0,1)
	// keep adding levels, as long as we dont exceed max level and generated probability is smaller than parameter probability
	while (p < skip_list->prob && level < skip_list->max_level)			
	{
		level++;
		p = (float) rand() / (float) ((unsigned)RAND_MAX + 1);
	}

	return level;
}

void skip_list_insert(SkipList skip_list, void * data, char * date)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_insert -> skip list is NULL\n");
	assert(skip_list != NULL);

	// first we search for the position where value should be inserted
	// and we record the path of nodes, which we will need later to update pointers

	int level = skip_list->cur_level;							// start searching from top current level
	SkipListNode cur_node = skip_list->header_dummy_node;		// start searching from the head node of the top level skip list
	SkipListNode next_node = NULL, temp_node = NULL;
	SkipListNode node_path[skip_list->max_level+1];				// the path will consist of at most as many nodes as the max height of the tallest skip-list

	for (int i = 0; i <= level; ++i)
		node_path[i] = NULL;

	while (level >= 0)
	{
		next_node = cur_node->next_array[level];
		while (next_node != NULL)
		{
			int check; 				// check for equality of id's
			if (strlen((char *) m_get_citizen_id((M_CitizenInfo) data)) == strlen((char *) m_get_citizen_id(next_node->info)))
				check = strcmp((char *) m_get_citizen_id((M_CitizenInfo) data), (char *) m_get_citizen_id(next_node->info));
			else
				check = (strlen((char *) m_get_citizen_id((M_CitizenInfo) data)) > strlen((char *) m_get_citizen_id(next_node->info))) ? 1 : -1 ;
		
			if (!check)
			{
				printf("skip_list_insert : Given value already exists. Insertion not done\n");
				return;
			}
			else if (check > 0)
			{
				cur_node = next_node;
				next_node = next_node->next_array[level];
			}
			else
			{
				node_path[level] = cur_node;		// we add a node to the node path, if it is the last node on current level, with value smaller than the one we are inserting
				break;
			}
		}

		// if we reached the end of the skip list of current level, and have not added any node of this level to the path
		if (node_path[level] == NULL)
			node_path[level] = cur_node;	// assign the last node we visited on the level

		if (!level) break;
		level--;
	}

	// now we create a new node at the base level L0
	// and initialize its components
	SkipListNode new_node = malloc(sizeof(struct skip_list_node));
	if (date != NULL)
	{
		new_node->date = (char *) malloc(strlen(date)+1);
		memcpy(new_node->date, date, strlen(date)+1);
	}
	else
		new_node->date = NULL;
	
	new_node->info = (M_CitizenInfo) data;
	new_node->level = random_level(skip_list);

	new_node->next_array = malloc((new_node->level+1) * sizeof(SkipListNode));		// generate next array, as big as the level of the new node
	if (new_node->next_array == NULL)
		fprintf(stderr, "Error : skip_list_insert -> malloc\n");
	assert(skip_list != NULL);

	// if level of new node is higher than current level, complete the path nodes, with the header node
	if (new_node->level > skip_list->cur_level)
	{
		for (int i = skip_list->cur_level + 1; i <= new_node->level; i++)
			node_path[i] = skip_list->header_dummy_node;

		// update new current level
		skip_list->cur_level = new_node->level;
	}

	// now we update the pointers of the new node, and the nodes in the recorded path
	for (int i = new_node->level; i >= 0 ; i--)
	{
		temp_node = node_path[i]->next_array[i];
		node_path[i]->next_array[i] = new_node;
		new_node->next_array[i] = temp_node;
	}

}


void skip_list_delete(SkipList skip_list, char * value)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_delete -> skip list is NULL\n");
	assert(skip_list != NULL);

	// first we search top to bottom for the position where value should be positioned
	// and we record the predecessors nodes of the node to be deleted, which we will need later to update pointers

	int level = skip_list->cur_level;					// start searching from top current level
	SkipListNode cur_node = skip_list->header_dummy_node;		// start searching from the head node of the top level skip list
	SkipListNode next_node = NULL;
	SkipListNode target_node = NULL;
	SkipListNode predecessors[level+1];						    // the predecessors will be  at most as many nodes as the current height of the tallest skip-list

	for (int i = 0; i <= level; ++i)
		predecessors[i] = NULL;

	while (level >= 0)
	{
		next_node = cur_node->next_array[level];
		while (next_node != NULL)
		{
			int check; 				// check for equality of id's
			if (strlen((char *) value) == strlen((char *) m_get_citizen_id(next_node->info)))
				check = strcmp((char *) value, (char *) m_get_citizen_id(next_node->info));
			else
				check = (strlen((char *) value) > strlen((char *) m_get_citizen_id(next_node->info))) ? 1 : -1 ;

			if (!check)   // next node has the target value as primary key, hence we currently are at the predecessor
			{
				predecessors[level] = cur_node;
				target_node = next_node;
				break;
			}
			else if (check > 0)
			{
				cur_node = next_node;
				next_node = next_node->next_array[level];
			}
			else
				break;  // just break, target value node is not as high as our current level
		}

		level--;
	}

	if (target_node == NULL)
	{
		printf("skip_list_delete : Given value does not exist. Deletion not done\n");
		return;
	}

	// now we update the pointers of the predecessors, and prepare target node deletion
	for (int i = target_node->level; i >= 0 ; i--)
	{
		predecessors[i]->next_array[i] = target_node->next_array[i];
	}

	// check if any of the levels which we are deleting the node from will be left empty
	// if that's the case decrement the current level accordingly
	for (int i = target_node->level; i >= 0 ; i--)
	{
		if (skip_list->header_dummy_node->next_array[i] == NULL)
			skip_list->cur_level--;
	}

	// free all alloced components of target node, and the target node itself
	free(target_node->next_array);
	if (target_node->date != NULL)
		free(target_node->date);
	free(target_node);

}


void skip_list_destroy(SkipList skip_list)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_destroy -> skip list is NULL\n");
	assert(skip_list != NULL);

	// we will just traverse and delete the nodes from the L0 base list, since all the nodes at level 0 are connected
	SkipListNode node = skip_list->header_dummy_node;		// begin traversal from header dummy node
	SkipListNode temp_node;

	// delete dummy node and all nodes of list (including all of their components)
	while (node != NULL)
	{
		temp_node = node;
		node = node->next_array[0];		// traversal of level zero list
		free(temp_node->next_array);
		if (temp_node->date != NULL)
			free(temp_node->date);
		free(temp_node);
	}

	free(skip_list); 		// delete the skip_list structure
}

void skip_list_print(SkipList skip_list)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_print -> skip list is NULL\n");
	assert(skip_list != NULL);

	SkipListNode node = NULL;
	SkipListNode head = skip_list->header_dummy_node;
	int level = skip_list->cur_level;

	while (level >= 0)
	{
		node = head;
		printf("\nLevel %d : ", level);
		while(node != NULL)
		{
			if (node->info != NULL)
				printf(" %s ", (char *) m_get_citizen_id(node->info));
				//citizen_info_print(node->info);
			node = node->next_array[level];
		}

		level--;
	}

	printf("\n\n");

}

void skip_list_print_data(SkipList skip_list)
{
	if (skip_list == NULL)
		fprintf(stderr, "Error : skip_list_print -> skip list is NULL\n");
	assert(skip_list != NULL);

	// we will just traverse the nodes from the L0 base list, since all the nodes at level 0 are connected
	SkipListNode node = skip_list->header_dummy_node;		// begin traversal from header dummy node

	// traverse all the way to the end
	while (node != NULL)
	{
		if (node->info != NULL)
			m_citizen_info_print(node->info);
		node = node->next_array[0];		// traversal of level zero list
	}

	printf("\n\n");
}