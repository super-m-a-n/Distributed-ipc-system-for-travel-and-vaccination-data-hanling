/* file : tm_helper.h */
/* important helper functions and structs for travelMonitor are developed here */
#pragma once
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "hash.h"


struct monitor_info {			// travelMonitor needs to keep some information about the monitor child processes
	pid_t pid;					// a pid
	int read_fd;				// a pipe fd where the travelMonitor reads from (the Monitor process writes)
	int write_fd;				// a pipe fd where the travelMonitor writes into (the Monitor process reads)
	HT viruses_info;			// a HT with viruses info for a set of countries associated with certain Monitor, namely a virus name and the Bloom Filters 
};

struct travelMonitor {
	int accepted;  							// total number of accepted travel requests
	int rejected;							// total number of rejected travel requests
	int numMonitors;						// number of monitors-child processes
	int bufferSize;							// the buffer size
	unsigned int bloom_size;				// the bloom size
	struct monitor_info **monitors_info;	// travelMonitor struct keeps an array of monitor info
	HT countries_info;						// a HT with country information namely a name of country and a monitor index (indicates which Monitor process "watches" that country)
};


/*________________________________________________________________________________________*/


/*===================== INITIALIZATION PHASE ===========================*/

// initializes the travelMonitor structure and all its substructures that are needed
struct travelMonitor * travelMonitor_init(int numMonitors, int bufferSize, unsigned int bloom_size, DIR * input_dir);
// initializes the ipc (creates the named pipes, forks and execs the child processes, opens the named pipes for the travelMonitor)
void ipc_init(struct travelMonitor * tm);
// assigns sub-directories of input_dir to the Monitor processes
void assign_subdirs(struct travelMonitor * tm, DIR * input_dir, const char * input_dir_name);
// waits for monitors to respond with the bloom filters and then updates structures with the bloom filters returned 
void wait_monitors_bfs(struct travelMonitor * tm);


/* =================== QUERY PHASE ========================= */

void travelRequest(struct travelMonitor * tm, char * citizenID, char * date, char * countryFrom, char * countryTo, char * virusName);
void travelStats(struct travelMonitor * tm, char * virusName, char * date1, char * date2, char * country);
void addVaccinationRecords(struct travelMonitor * tm, char * country, const char * input_dir_name);
void searchVaccinationStatus(struct travelMonitor * tm, char * citizenID);
void exit_travelMonitor(struct travelMonitor * tm);


/*==================== EXIT PHASE ========================== */

// helper functions to exit function exit_travelMonitor :
// sends SIGKILL/SIGTERM to all monitors
void term_monitors(struct travelMonitor * tm);
// wait on children monitors to exit 
void wait_monitors(struct travelMonitor * tm);
// prints out counries/no accepted/no rejected to a log file
void tm_log_file_print(struct travelMonitor * tm);
// deletes the travelMonitor structure and all its substructures that were used
void travelMonitor_del(struct travelMonitor * tm);

/*================== SIGNALS =============================== */
// upon receiving SIGCHLD checks which of its children have exited and replaces them
int replaceMonitors(struct travelMonitor * tm, const char * input_dir_name);
