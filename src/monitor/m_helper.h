/* file : m_helper.h */
/* important helper functions and structs for Monitor are developed here */
#pragma once
#include "hash.h"

struct Monitor {
	int accepted;
	int rejected;
	int bufferSize;
	int read_fd;
	int write_fd;
	HT citizens_info;
	HT viruses_info;
	HT countries_info;
	unsigned int bloom_size;
	int max_level;
	float p;
};

/*__________________________________________________________________________*/


/*===================== INITIALIZATION PHASE ===========================*/

/* initializes the monitor structure and all its substructures needed */
struct Monitor * Monitor_init(int bufferSize, unsigned int bloom_size, int max_level, float p);
/* read bufferSize and bloom_size from travelMonitor*/
int read_buffer_bloom_size(int * bufferSize, unsigned int * bloom_size, int read_fd, int write_fd);
/* reads all the subdirectories assigned by travelMonitor, and then returns the bloom filters back */
int read_subdirs(struct Monitor * monitor);
/* reads the subdirectory indicated by char * subdir and updates structures */
int read_subdir(struct Monitor * monitor, char * subdir);
/* inserts given entry/line from file into all the necessary data structures of the monitor */
void Monitor_insert(struct Monitor * monitor, char * citizenID , char * firstName, char * lastName, char * country, unsigned int age, char * virusName, char * vacc, char * date);


/* =================== QUERY PHASE ========================= */

/* monitor process takes an action depending on the message it received */
int Monitor_take_action(struct Monitor * monitor, int msgd, void * message, char * subdir);
void vaccineStatus(struct Monitor * monitor, char * citizenID, char * virusName);

/*==================== EXIT PHASE ========================== */
/* destroys the monitor structure and all of its substructures that were created and used, closes open file descriptors */
void Monitor_del(struct Monitor * monitor);

/* ================== SIGNALS ============================== */
// prints out counries/no accepted/no rejected to a log file (triggered by SIGINT/SIGQUIT)
void m_log_file_print(struct Monitor * monitor);
// reads any new .txt from subdirectory of given country and returns the updated bloom filters (triggered by SIGUSR1)
int read_subdir_updates(struct Monitor * monitor, char * subdir);