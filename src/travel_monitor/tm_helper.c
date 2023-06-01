/* file : tm_helper.c */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "hash.h"
#include "bloom.h"
#include "tm_helper.h"
#include "tm_items.h"
#include "messages.h"
#include "date.h"

#define PERMS 0766


/*===================== INITIALIZATION PHASE ===========================*/

struct travelMonitor * travelMonitor_init(int numMonitors, int bufferSize, unsigned int bloom_size, DIR * input_dir)
{
	struct travelMonitor * tm = malloc(sizeof(struct travelMonitor));
	if (tm == NULL)
		fprintf(stderr, "Error : travelMonitor_init -> malloc \n");
	assert(tm != NULL);
	
	struct dirent * subdir;
	int countries = 0;

	while ((subdir = readdir(input_dir)) != NULL) 	// traverse all subdirectories of input_dir
	{
		if (!strcmp(subdir->d_name, ".") || !strcmp(subdir->d_name, ".."))	// if you are at . or .. just ignore
			continue;
		else
			countries += 1;
	}
	
	if (countries < numMonitors)  // do not create excess Monitors that are not needed
	{
		tm->numMonitors = countries;
		printf("\ntravelMonitor_init -> numMonitors > countries, only needed Monitors will be created\n\n");
	}
	else
		tm->numMonitors = numMonitors;

	tm->bufferSize = bufferSize;
	tm->bloom_size = bloom_size;
	tm->accepted = 0;
	tm->rejected = 0;
	
	tm->monitors_info = malloc(tm->numMonitors * sizeof(struct monitor_info *));		// create an array of monitors_info structs
	if (tm->monitors_info == NULL)
		fprintf(stderr, "Error : travelMonitor_init -> malloc \n");
	assert(tm->monitors_info != NULL);

	for (int i = 0; i < tm->numMonitors; i++)
	{
		tm->monitors_info[i] = malloc(sizeof(struct monitor_info));
		if (tm->monitors_info[i] == NULL)
			fprintf(stderr, "Error : travelMonitor_init -> malloc \n");
		assert(tm->monitors_info[i] != NULL);
		tm->monitors_info[i]->viruses_info = hash_create(10, 4);	// create the hash_table of viruses_info (virus name, bloom filter) for travelMonitor
	}	

	tm->countries_info = hash_create(10, 5);				// create the hash_table of countries_info (country name, monitor index) for travelMonitor
	bloomSize_init(bloom_size);			// initialize bloomSize for messages.c

	return tm;
}

void ipc_init(struct travelMonitor * tm)
{
	for (int i = 0; i < tm->numMonitors; ++i)
	{
		// travelMonitor creates 2 fifos for each child Monitor, one for write, one for read
		char fifo_write_path[30];	  // fifo path for write purposes 
		snprintf(fifo_write_path, 30, "fifo%dW", i+1);
		char fifo_read_path[30];	 // fifo path for read purposes
		snprintf(fifo_read_path, 30, "fifo%dR", i+1);

		if (mkfifo(fifo_write_path, PERMS) < 0 && (errno != EEXIST))		// make fifo for write purposes (child monitor will use this to read)
		{
			perror("[Error] : ipc_init -> mkfifo\n");
			exit(EXIT_FAILURE);
		}

		if (mkfifo(fifo_read_path, PERMS) < 0 && (errno != EEXIST))			// make fifo for read purposes (child monitor will use this to write)
		{
			perror("[Error] : ipc_init -> mkfifo\n");
			exit(EXIT_FAILURE);
		}

		if ((tm->monitors_info[i]->pid = fork()) < 0)						 	// fork child monitor and save its pid	
		{
			perror("[Error] : ipc_init -> fork\n");
			exit(EXIT_FAILURE);
		}
		else if (!tm->monitors_info[i]->pid)									// if you are the child monitor, exec with the Monitor.c code file
		{
			if (execl("./Monitor", "Monitor", fifo_write_path, fifo_read_path, NULL) < 0)	// exec with just the fifo paths
			{
				perror("[Error] : ipc_init -> execl\n");
				exit(EXIT_FAILURE);
			}
		}
		else		// if you are the father traveMonitor, open the 2 fifos created above
		{
			if ((tm->monitors_info[i]->write_fd = open(fifo_write_path, O_RDWR | O_NONBLOCK)) < 0)		// open the fifo for write purposes (in non blocking mode)																										
			{																							// and save its file descriptor			
				perror("[Error] : ipc_init -> open (for write)\n");
				exit(EXIT_FAILURE);
			}

			if ((tm->monitors_info[i]->read_fd = open(fifo_read_path, O_RDONLY | O_NONBLOCK)) < 0)    	// open the fifo for read purposes (in read-only non blocking mode)
			{																							// and save its file descriptor
				perror("[Error] : ipc_init -> open (for read)\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	for (int i = 0; i < tm->numMonitors; ++i)		// for each Monitor process just created
	{
		void * message = create_msg0(tm->bufferSize, tm->bloom_size);		
		send_message(tm->monitors_info[i]->write_fd, MSG0, message, tm->bufferSize);	// send the bufferSize and the bloom size as the first message
	}

	int ready_monitors = 0;
	fd_set readfds;
	
	int is_set[tm->numMonitors];				// keeps track of which child monitor processes have been dealt with (have sent DONE)
	for (int i = 0; i < tm->numMonitors; ++i)
		is_set[i] = 0;							// initially all read fd are not set

	while (ready_monitors != tm->numMonitors)	// repeat until all child monitor processes info has been read by travelMonitor
	{
		FD_ZERO(&readfds);						// reinitialize the set of read fds to wait on
		int max_fd = 0;
		
		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])
			{
				FD_SET(tm->monitors_info[i]->read_fd, &readfds);			// each iteration we set the read fds not yet set
				if (tm->monitors_info[i]->read_fd > max_fd)
					max_fd = tm->monitors_info[i]->read_fd;				// find max file descriptor for first arg in select
			}
		}

		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			perror("[Error] : ipc_init -> select\n");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])		// we only care for those read fd not yet set (still pending)
			{
				if(FD_ISSET(tm->monitors_info[i]->read_fd, &readfds))		// if readfd was set this time (we can safely read without fear of blocking - there is data in the read end of pipe)
				{	
					int msgd;
					read_message(tm->monitors_info[i]->read_fd, &msgd, tm->bufferSize);	// read response message from Monitor (should be a DONE message)
					if (msgd != DONE)
					{
						fprintf(stderr, "[Error] : ipc_init -> Unexpected message descriptor\n\n");
						exit(EXIT_FAILURE);						
					}
					
					ready_monitors += 1;
					is_set[i] = 1;			// readfd has finally been set, don't worry about it no more
				}
			}
		}
	}
}

void assign_subdirs(struct travelMonitor * tm, DIR * input_dir, const char * input_dir_name)
{
	int monitor_index = 0;							// we begin assigning subdirectories from first monitor
	struct dirent ** subdir_list;
	rewinddir(input_dir);							// reset pointer to beginning of directory, if it was changed from previous calls to readdir
	int n;
	
	if ( (n = scandir(input_dir_name, &subdir_list, NULL, alphasort)) < 0)		// we use scandir to iterate over files in alphabetical order
	{
		perror("[Error] : assing_subdirs -> scandir\n");
		exit(EXIT_FAILURE);
	}
	
	for (int i = 0; i < n; ++i) 	// traverse all subdirectories of input_dir alphabetically
	{
		if (!strcmp(subdir_list[i]->d_name, ".") || !strcmp(subdir_list[i]->d_name, ".."))	// if you are at . or .. just ignore
		{
			free(subdir_list[i]);
			continue;
		}
		else
		{
			void * message = create_msg1(input_dir_name, subdir_list[i]->d_name);								// construct message
			send_message(tm->monitors_info[monitor_index]->write_fd, MSG1, message, tm->bufferSize);	// send message
			TM_CountryInfo country_info = tm_country_info_create(subdir_list[i]->d_name, monitor_index);	// new subdir means a new country, so create a new country_info struct
			hash_insert(tm->countries_info, country_info);				// insert the new country_info into the countries_info hashtable of travelMonitor
			monitor_index = (monitor_index + 1) % tm->numMonitors;		// subdirectories are assigned to Monitor processes using alphabetical round-robin scheme
		}

		free(subdir_list[i]);
    }

    free(subdir_list);

    // after assigning all subdirectories to the Monitor processes, notify them you are DONE sending subdirectories and you expect back the bloom filters
    for (int i = 0; i < tm->numMonitors; ++i)
    	send_message(tm->monitors_info[i]->write_fd, DONE, NULL, tm->bufferSize);

    closedir(input_dir);			// input_dir is no longer needed
}

void wait_monitors_bfs(struct travelMonitor * tm)
{
	int ready_monitors = 0;
	fd_set readfds;
	
	int is_set[tm->numMonitors];					// keeps track of which child monitor processes have been dealt with (parent has read ALL their bloom filters)
	for (int i = 0; i < tm->numMonitors; ++i)
		is_set[i] = 0;							// initially all read fd are not set

	while (ready_monitors != tm->numMonitors)	// repeat until all child monitor processes info has been read by travelMonitor
	{
		FD_ZERO(&readfds);						// reinitialize the set of read fds to wait on
		int max_fd = 0;
		
		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])
			{
				FD_SET(tm->monitors_info[i]->read_fd, &readfds);			// each iteration we set the read fds not yet set
				if (tm->monitors_info[i]->read_fd > max_fd)
					max_fd = tm->monitors_info[i]->read_fd;				// find max file descriptor for first arg in select
			}
		}

		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			perror("[Error] : wait_monitors_bfs -> select\n");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])		// we only care for those read fd not yet set (still pending)
			{
				if(FD_ISSET(tm->monitors_info[i]->read_fd, &readfds))		// if readfd was set this time (we can safely read without fear of blocking - there is data in the read end of pipe)
				{	
					int msgd;
					void * message = read_message(tm->monitors_info[i]->read_fd, &msgd, tm->bufferSize);	//read message data and its header-msgd
					if (msgd != DONE)	// if message still has data (has not sent DONE msgd yet)
					{
						char virus[20]; void * bit_array;
						if (decode_msg2(msgd, message, virus, &bit_array) < 0)		// decode message of expected type (MSG2) 	
							exit(EXIT_FAILURE);
						TM_VirusInfo virus_info = tm_virus_info_create(virus, tm->bloom_size, bit_array);
						hash_insert(tm->monitors_info[i]->viruses_info, virus_info);	//update viruses_info HT
						delete_message(message);  	// message is read and decoded, no longer needed
					}
					else	// monitor process sent DONE message, that means it is done sending bloom filters and is ready for commands
					{
						ready_monitors += 1;
						is_set[i] = 1;			// readfd has finally been set, don't worry about it no more
					}
				}
			}
		}
	}
}


/* =================== QUERY PHASE ========================= */

void travelRequest(struct travelMonitor * tm, char * citizenID, char * date, char * countryFrom, char * countryTo, char * virusName)
{
	int result;
	if (!date_check(date))
	{
		fprintf(stderr, "[Error] : travelRequest -> Invalid date\n\n");
		return;
	}

	for (int i = 0; i < strlen(citizenID); i++)	// check for an integer citizenID
	{
		if (citizenID[i] < '0' || citizenID[i] > '9')
		{
			fprintf(stderr, "[Error] : travelRequest -> CitizenID is not a string of numbers\n\n");
			return;
		}
	}

	if (strlen(citizenID) > 5)
	{
		fprintf(stderr, "[Error] : travelRequest -> CitizenID has at most 5 digits\n\n");
		return;
	}

	// search for the country countryFrom in the HT of countries of travelMonitor
	TM_CountryInfo countryFrom_info = (TM_CountryInfo) hash_search(tm->countries_info, countryFrom);
	if (countryFrom_info == NULL)
	{
		fprintf(stderr, "[Error] : travelRequest -> Given countryFrom does not exist in travelMonitor's database\n\n");
		return;
	}

	// search for the country countryTo in the HT of countries of travelMonitor
	TM_CountryInfo countryTo_info = (TM_CountryInfo) hash_search(tm->countries_info, countryTo);
	if (countryTo_info == NULL)
	{
		fprintf(stderr, "[Error] : travelRequest -> Given countryTo does not exist in travelMonitor's database\n\n");
		return;
	}

	int monitor_index = tm_get_country_monitor(countryFrom_info);		// get the index of monitor that "watches" the specific countryFrom
	int monitor_index_to = tm_get_country_monitor(countryTo_info);		// get the index of monitor that "watches" the specific countryTo
	// search for given virusName in the viruses HT of countryFrom of travelMonitor
	TM_VirusInfo virus_info = (TM_VirusInfo) hash_search(tm->monitors_info[monitor_index]->viruses_info, virusName);
	if (virus_info == NULL)
	{
		fprintf(stderr, "[Error] : travelRequest -> Given virus is not checked in given countryFrom\n\n");
		/* TODO maybe consider this an rejected request as well */
		return;
	}

	if (!bloom_check(tm_get_bloom_filter(virus_info), (unsigned char *) citizenID))
	{
		printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n\n");
		tm->rejected += 1; result = 0;
	}
	else	// bloom filter replied with MAYBE so send query to Monitor process to find out for sure
	{
		void * message = create_msg3(citizenID, virusName);							// construct message
		send_message(tm->monitors_info[monitor_index]->write_fd, MSG3, message, tm->bufferSize);	// send message	
		int msgd;
		void * response_msg = read_message(tm->monitors_info[monitor_index]->read_fd, &msgd, tm->bufferSize);	//read response message from Monitor process
		char answer[4], vacc_date[12];
		if (decode_msg4(msgd, response_msg, answer, vacc_date) < 0)		// decode message of expected type (MSG4)
			exit(EXIT_FAILURE);
		if (!strcmp(answer, "NO"))	
		{
			printf("REQUEST REJECTED - YOU ARE NOT VACCINATED\n\n");
			tm->rejected += 1; result = 0;
		}
		else if (!strcmp(answer, "YES") && !date_half_year_check(vacc_date, date))
		{
			printf("REQUEST REJECTED - YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n\n");
			tm->rejected += 1; result = 0;
		}
		else if (!strcmp(answer, "YES") && date_half_year_check(vacc_date, date) < 0)
		{
			printf("REQUEST REJECTED - YOU ARE NOT VACCINATED (VACCINATION FOUND BUT IS AFTER THE TRAVEL DATE)\n\n");
			tm->rejected += 1; result = 0;	
		}
		else if (!strcmp(answer, "YES") && date_half_year_check(vacc_date, date))
		{
			printf("REQUEST ACCEPTED - HAPPY TRAVELS\n\n");
			tm->accepted += 1; result = 1;
		}

		delete_message(response_msg);		// no longer need message
	}

	// notify Monitor process that handles countryTo, whether the request got accepted or rejected
	void * message = create_msg8(result);		// construct message
	send_message(tm->monitors_info[monitor_index_to]->write_fd, MSG8, message, tm->bufferSize);	// send message
	int msgd;
	read_message(tm->monitors_info[monitor_index_to]->read_fd, &msgd, tm->bufferSize);		// read response message (should be a DONE message)
	if (msgd != DONE)
	{
		fprintf(stderr, "[Error] : travelRequest -> Unexpected message descriptor\n\n");
		exit(EXIT_FAILURE);						
	}

	tm_country_add_travelRequest(countryTo_info, date, virusName, result);		// save the travel Request for the countryTo
}

void travelStats(struct travelMonitor * tm, char * virusName, char * date1, char * date2, char * country)
{
	int rejected = 0, accepted = 0;

	if (!date_check(date1) || !date_check(date2) || date_cmp(date1, date2) > 0 )	// check for valid dates
	{
		fprintf(stderr, "Error : travelStats -> Invalid dates\n\n");
		return;
	}

	if (country != NULL)		// if a country was given as an argument
	{
		// search for the given country in the HT of countries of travelMonitor
		TM_CountryInfo country_info = (TM_CountryInfo) hash_search(tm->countries_info, country);
		if (country_info == NULL)
		{
			fprintf(stderr, "[Error] : travelStats -> Given country does not exist in travelMonitor's database\n\n");
			return;
		}

		tm_get_country_travelStats(country_info, virusName, date1, date2, &accepted, &rejected);
		printf("TOTAL REQUESTS %d\n", accepted + rejected);		// print stats
		printf("ACCEPTED %d\n", accepted);
		printf("REJECTED %d\n\n", rejected);
	}
	else		// no specific country was given, so do the same thing for all countries in database
	{
		TM_CountryInfo country_info;
		// iterate upon the hash-table of countries of travelMonitor
		while ((country_info = (TM_CountryInfo) hash_iterate_next(tm->countries_info)) != NULL)
		{
			int temp_accepted = 0, temp_rejected = 0;
			tm_get_country_travelStats(country_info, virusName, date1, date2, &temp_accepted, &temp_rejected);
			rejected += temp_rejected; accepted += temp_accepted;		// sum the accepted and rejected for all countries
		}

		printf("TOTAL REQUESTS %d\n", accepted + rejected);  // print total stats
		printf("ACCEPTED %d\n", accepted);
		printf("REJECTED %d\n\n", rejected);
	}
}

void addVaccinationRecords(struct travelMonitor * tm, char * country, const char * input_dir_name)
{
	// first things first we check if given country is valid - is in travelMonitor's database
	// search for the country in the HT of countries of travelMonitor
	TM_CountryInfo country_info = (TM_CountryInfo) hash_search(tm->countries_info, country);
	if (country_info == NULL)
	{
		fprintf(stderr, "[Error] : addVaccinationRecords -> Given country does not exist in travelMonitor's database\n\n");
		return;
	}

	int monitor_index = tm_get_country_monitor(country_info);		// get the index of monitor that "watches" the specific countryFrom
	void * message = create_msg1(input_dir_name, country);										// construct message
	send_message(tm->monitors_info[monitor_index]->write_fd, MSG1, message, tm->bufferSize);	// send message
	int msgd_done;
	read_message(tm->monitors_info[monitor_index]->read_fd, &msgd_done, tm->bufferSize);		// read response message (should be a DONE message)
	if (msgd_done != DONE)
	{
		fprintf(stderr, "[Error] : addVaccinationRecords -> Unexpected message descriptor\n\n");
		exit(EXIT_FAILURE);						
	}
	
	if( kill(tm->monitors_info[monitor_index]->pid, SIGUSR1) < 0)				// sends a SIGUSR1 to Monitor process
		perror("[Error] addVaccinationRecords -> kill\n");

	int msgd = -2;
	while (msgd != DONE)		// now we read all the updated bloom filters sent from Monitor process
	{
		void * response_msg = read_message(tm->monitors_info[monitor_index]->read_fd, &msgd, tm->bufferSize);	//read response message
		if (msgd != DONE)	// if message still has data (Monitor has not sent DONE msgd yet)
		{
			char virus[20]; void * bit_array;
			if (decode_msg2(msgd, response_msg, virus, &bit_array) < 0)		// decode message of expected type (MSG2) 	
				exit(EXIT_FAILURE);
			TM_VirusInfo virus_info = hash_search(tm->monitors_info[monitor_index]->viruses_info, virus);		// search for the virus of message into Monitors HT of viruses
			if (virus_info != NULL)		// if found (virus already exists)
			{
				// just update the bloom filter (existing bit array) of virus
				bloom_bit_array_copy(tm_get_bloom_filter(virus_info), bit_array);
			}
			else  // if not found (virus is a new virus)
			{
				virus_info = tm_virus_info_create(virus, tm->bloom_size, bit_array);		// create new virus_info 
				hash_insert(tm->monitors_info[monitor_index]->viruses_info, virus_info);	//update viruses_info HT
			}

			delete_message(response_msg);  	// message is read and decoded, no longer needed
		}

	}

	printf("travelMonitor -> Bloom filters structures have been updated\n\n");
}

void searchVaccinationStatus(struct travelMonitor * tm, char * citizenID)
{
	int found = 0;

	for (int i = 0; i < strlen(citizenID); i++)	// check for an integer citizenID
	{
		if (citizenID[i] < '0' || citizenID[i] > '9')
		{
			fprintf(stderr, "[Error] : searchVaccinationStatus -> CitizenID is not a string of numbers\n\n");
			return;
		}
	}

	if (strlen(citizenID) > 5)
	{
		fprintf(stderr, "[Error] : searchVaccinationStatus -> CitizenID has at most 5 digits\n\n");
		return;
	}

	for ( int i = 0; i < tm->numMonitors; ++i)		// travelMonitor sends message to all Monitor child processes
	{
		void * message = create_msg5(citizenID);										// construct message
		send_message(tm->monitors_info[i]->write_fd, MSG5, message, tm->bufferSize);	// send message
	}

	int done_monitors = 0;
	fd_set readfds;
	
	int is_set[tm->numMonitors];				// keeps track of which child monitor processes have been dealt with (parent has read all info about citizenID from them)
	for (int i = 0; i < tm->numMonitors; ++i)
		is_set[i] = 0;							// initially all read fd are not set

	while (done_monitors != tm->numMonitors)	// repeat until all child monitor processes info has been read by travelMonitor
	{
		FD_ZERO(&readfds);						// reinitialize the set of read fds to wait on
		int max_fd = 0;
		
		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])
			{
				FD_SET(tm->monitors_info[i]->read_fd, &readfds);			// each iteration we set the read fds not yet set
				if (tm->monitors_info[i]->read_fd > max_fd)
					max_fd = tm->monitors_info[i]->read_fd;				// find max file descriptor for first arg in select
			}
		}

		if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			perror("[Error] : searchVaccinationStatus -> select\n");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (!is_set[i])		// we only care for those read fd not yet set (still pending)
			{
				if(FD_ISSET(tm->monitors_info[i]->read_fd, &readfds))		// if readfd was set this time (we can safely read without fear of blocking - there is data in the read end of pipe)
				{	
					int msgd;
					void * message = read_message(tm->monitors_info[i]->read_fd, &msgd, tm->bufferSize);	//read message data and its header-msgd
					if (msgd != DONE)	// if message still has data (has not sent DONE msgd yet)
					{
						if (msgd == MSG6)		// MSG6 means Monitor sent name, surname, country, age about given citizenID
						{
							char name[13], surname[13], country[30]; int age;
							if (decode_msg6(msgd, message, name, surname, country, &age) < 0)
								exit(EXIT_FAILURE);
							found = 1;							// at least one monitor process found given citizenID
							printf("%s %s %s %s\n", citizenID, name, surname, country);
							printf("AGE %d\n", age);
						}
						else if (msgd == MSG7)	// MSG7 means Monitor sent vaccination info about given citizenID (virusname, vaccination status, date)
						{
							char virus[20] , status[4], date[12];
							if (decode_msg7(msgd, message, virus, status, date) < 0)
								exit(EXIT_FAILURE);
							printf("%s ", virus);
							if (!strcmp(status, "YES"))
								printf("VACCINATED ON %s\n", date);
							else
								printf("NOT YET VACCINATED\n");
						}
						else		// otherwise we have an IPC error, which should NEVER occur
						{
							fprintf(stderr, "[Error] : searchVaccinationStatus -> Unexpected message descriptor\n\n");
							exit(EXIT_FAILURE);
						}
					
						delete_message(message);  	// message is read and decoded, no longer needed
					}
					else	// monitor process sent DONE message, that means it is done sending info about citizenID and is ready for other commands
					{
						done_monitors += 1;
						is_set[i] = 1;			// readfd has finally been set, don't worry about it no more
					}
				}
			}
		}
	}

	if (!found)
	{
		printf("[Error] : searchVaccinationStatus -> No Monitor Process found any info about given CitizenID\n");
		printf("CitizenID : %s does not exist in database\n\n", citizenID);
	}
	else
		printf("\n");	
}

void exit_travelMonitor(struct travelMonitor * tm)
{
	term_monitors(tm);		// terminate child monitor processes
	wait_monitors(tm);		// call wait() on the children to make sure they all exited
	tm_log_file_print(tm);		// print info into log file
	travelMonitor_del(tm);	// lastly, cleans up all memory
}


/*==================== EXIT PHASE ========================== */

void term_monitors(struct travelMonitor * tm)
{
	for (int i = 0; i < tm->numMonitors; ++i)
	{
		if(kill(tm->monitors_info[i]->pid, SIGKILL) < 0)		// sends a SIGKILL to all monitor processes
			perror("[Error] term_monitors -> kill\n");
	}

}

void wait_monitors(struct travelMonitor * tm)
{
	int monitors_exited = 0;
	int status;

	while (monitors_exited != tm->numMonitors)
	{
		if (wait(&status) < 0)
		{
			perror("[Error] : wait_monitors -> wait\n");
			continue;
		}

		monitors_exited += 1;
	}
}

void tm_log_file_print(struct travelMonitor * tm)
{
	FILE * file_ptr;
	char log_file_name[40];
	snprintf(log_file_name, 40, "log_file.%d.txt", getpid());	// create log file name
	file_ptr = fopen(log_file_name, "w");						// open file for writing
	if (file_ptr == NULL)						
	{
		fprintf(stderr, "[Error] : log_file_print -> Could not open/create log file\n\n");
		return;
	}

	TM_CountryInfo country_info;
	// iterate upon the hash-table of countries of travelMonitor
	while ((country_info = (TM_CountryInfo) hash_iterate_next(tm->countries_info)) != NULL)
		fprintf(file_ptr, "%s\n", tm_get_country_name(country_info));				// print all countries that participated in travelMonitor
	fprintf(file_ptr, "TOTAL TRAVEL REQUESTS %d\n", tm->accepted + tm->rejected);		// print total travel requests
	fprintf(file_ptr, "ACCEPTED %d\n", tm->accepted);								// print the #accepted
	fprintf(file_ptr, "REJECTED %d\n", tm->rejected);								// print the #rejected
	fclose(file_ptr);
}

void travelMonitor_del(struct travelMonitor * tm)
{
	for (int i = 0; i < tm->numMonitors; ++i)
		hash_destroy(tm->monitors_info[i]->viruses_info);

	for (int i = 0; i < tm->numMonitors; ++i)
	{
		close(tm->monitors_info[i]->write_fd);		/* close the write/read file descriptors */
		close(tm->monitors_info[i]->read_fd);
		/* and now, assuming child has closed his read/write file descriptors, unlink the fifos */
		char fifo_write_path[30];	   
		snprintf(fifo_write_path, 30, "fifo%dW", i+1);	// fifo path for write purposes
		char fifo_read_path[30];	 
		snprintf(fifo_read_path, 30, "fifo%dR", i+1);	// fifo path for read purposes
		unlink(fifo_write_path);	// unlink the fifos
		unlink(fifo_read_path);
		free(tm->monitors_info[i]);
	}
	free(tm->monitors_info);
	hash_destroy(tm->countries_info);
	free(tm);
}

/*================== SIGNALS =============================== */
int replaceMonitors(struct travelMonitor * tm, const char * input_dir_name)
{
	/* NOTE : we assume a child is unexpectedly terminated only when it is not in a middle of an IPC with the parent */
	/* otherwise chaos may ensue */ /* this assumption was also suggested by Mr.Doulas on Piazza */
	int status, msgd;
	pid_t pid;
	/* one or more children were killed, so wait on them first */
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0 )		/* loop until an error occurs or no remaining children have changed state */
	{
		for (int i = 0; i < tm->numMonitors; ++i)
		{
			if (tm->monitors_info[i]->pid == pid)		// find the index of the Monitor child process that got terminated
			{	
				// close the write and read ends of parent fifos, childrens read and write ends have been automatically closed upon termination
				close(tm->monitors_info[i]->write_fd);
				close(tm->monitors_info[i]->read_fd);

				// reobtain the write and read fifo paths
				char fifo_write_path[30];	  // fifo path for write purposes 
				snprintf(fifo_write_path, 30, "fifo%dW", i+1);
				char fifo_read_path[30];	 // fifo path for read purposes
				snprintf(fifo_read_path, 30, "fifo%dR", i+1);

				if ((tm->monitors_info[i]->pid = fork()) < 0)			// fork new child monitor to replace the terminated one, and save its pid	
				{
					perror("[Error] : replaceMonitors -> fork\n");
					return -1;
				}
				else if (!tm->monitors_info[i]->pid)					// if you are the new child monitor, exec with the Monitor.c code file
				{
					if (execl("./Monitor", "Monitor", fifo_write_path, fifo_read_path, NULL) < 0)
					{
						perror("[Error] : replaceMonitors -> execl\n");
						return -1;
					}	
				}
				else		// if you are the father traveMonitor, reopen the 2 fifos
				{
					if ((tm->monitors_info[i]->write_fd = open(fifo_write_path, O_RDWR | O_NONBLOCK)) < 0)		// open the fifo for write purposes (in non blocking mode)																										
					{																							// and save its file descriptor			
						perror("[Error] : replaceMonitors -> open (for write)\n");
						return -1;
					}

					if ((tm->monitors_info[i]->read_fd = open(fifo_read_path, O_RDONLY | O_NONBLOCK)) < 0)    	// open the fifo for read purposes (in read-only non blocking mode)
					{																							// and save its file descriptor
						perror("[Error] : replaceMonitors -> open (for read)\n");
						return -1;
					}

					void * message = create_msg0(tm->bufferSize, tm->bloom_size);		
					send_message(tm->monitors_info[i]->write_fd, MSG0, message, tm->bufferSize);	// send the bufferSize and the bloom size as the first message
					int msgd0;
					read_message(tm->monitors_info[i]->read_fd, &msgd0, tm->bufferSize);		// read response message from Monitor (should be a DONE message)
					if (msgd0 != DONE)
					{
						fprintf(stderr, "[Error] : replaceMonitors -> Unexpected message descriptor\n\n");
						return -1;						
					}

					// find all countries that the terminated Monitor child process was handling
					// and assign them to the newly created Monitor child process

					TM_CountryInfo country_info;
					// iterate upon the hash-table of countries
					while ((country_info = hash_iterate_next(tm->countries_info)) != NULL)
					{
						if (tm_get_country_monitor(country_info) == i)		// country was being handled by terminated Monitor child process
						{													// so newly created Monitor should handle it now
							void * message = create_msg1(input_dir_name, tm_get_country_name(country_info));			// construct message
							send_message(tm->monitors_info[i]->write_fd, MSG1_NO_REPLY, message, tm->bufferSize);	    // send message
						}
					}

					// after assigning all subdirectories-countries to the new Monitor process
					//notify it you are DONE sending subdirectories
    				send_message(tm->monitors_info[i]->write_fd, DONE, NULL, tm->bufferSize);
					read_message(tm->monitors_info[i]->read_fd, &msgd, tm->bufferSize);			// Monitor process just created, replies with a DONE message
					if (msgd != DONE)															// implying everything went fine, and is ready for commands
					{
						fprintf(stderr, "[Error] : replaceMonitors -> Unexpected message descriptor\n\n");
						return -1;
					}

					break;
				}
			}
		}
	}

	if (pid < 0)		// an error occured with wait()
	{
		perror("[Error] : replaceMonitors -> wait\n");
		return -1;
	}

	printf("\nMonitor child processes (1 or more) were unexpectedly killed and got replaced\n\n");

	return 0;
}