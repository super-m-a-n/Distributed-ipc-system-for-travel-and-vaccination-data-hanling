/* file : m_helper.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include "m_helper.h"
#include "skip_list.h"
#include "bloom.h"
#include "hash.h"
#include "list.h"
#include "m_items.h"
#include "messages.h"



/*===================== INITIALIZATION PHASE ===========================*/

struct Monitor * Monitor_init(int bufferSize, unsigned int bloom_size, int max_level, float p)
{
	struct Monitor * monitor = malloc(sizeof(struct Monitor));
	if (monitor == NULL)
		fprintf(stderr, "Error : Monitor_init -> malloc \n\n");
	assert(monitor != NULL);

	monitor->citizens_info = hash_create(100, 0);
	monitor->viruses_info = hash_create(10, 1);
	monitor->countries_info = hash_create(10, 2);
	monitor->bufferSize = bufferSize;
	monitor->bloom_size = bloom_size;
	monitor->max_level = max_level;
	monitor->p = p;
	monitor->accepted = 0;
	monitor->rejected = 0;
	bloomSize_init(bloom_size);		// initialize bloomSize for messages.c

	return monitor;
}

int read_buffer_bloom_size(int * bufferSize, unsigned int * bloom_size, int read_fd, int write_fd)
{
	int msgd;
	void * message = read_message(read_fd, &msgd, sizeof(int));		// read first message sent by travelMonitor, bufferSize is unknown so we set it to the minimum possible
	if (msgd != MSG0)		// expected message descriptor is MSG0
		return -1;
	if (decode_msg0(msgd, message, bufferSize, bloom_size) < 0)		// decode message to extract bufferSize, bloom_size
		return -1;			// unexpected message descriptor

	delete_message(message);		// message is no longer needed
	send_message(write_fd, DONE, NULL, *bufferSize);		// send back a DONE message to parent travelMonitor to notify you are ready to move on
	return 0;
}

int read_subdirs(struct Monitor * monitor)
{
	int msgd = -2;
	int no_reply = 0;		// if 1, then current Monitor process was created to replace an old one, so no need to send back the bloom filters to the parent
							// if 0 (default behavior), Monitor process sends back the bloom filters to the parent

	while (msgd != DONE)		// while travelMonitor has not notified you he is done assigning subdirs to you
	{
		void * message;
		message = read_message(monitor->read_fd, &msgd, monitor->bufferSize);		// wait here until you read message
		if (msgd == DONE)
			continue;
		// else message must be of type MSG1 or MSG1_NO_REPLY
		if (msgd == MSG1_NO_REPLY)
			no_reply = 1;

		char subdir[30];
		if (decode_msg1(msgd, message, subdir) < 0)				/* decode message of expected type (MSG1 or MSG1_NO_REPLY) */
			return -1;											// unexpected message descriptor
		if (read_subdir(monitor, subdir) < 0)					/* read subdirectory sent by travelMonitor */
			return -1;
		delete_message(message);  								/* message is no longer needed */
	}

	if (no_reply)	// if parent does not expect back any bloom filters, just return DONE
	{
		send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);
		return 0;
	}
	
	// travelMonitor is done assigning subdirs to you, so send him back the bloom filters
	M_VirusInfo virus_info;
	// iterate upon the hash-table of viruses
	while ((virus_info = hash_iterate_next(monitor->viruses_info)) != NULL)
	{
		void * response_msg = create_msg2(m_get_virus_name(virus_info), monitor->bloom_size , m_get_bloom_filter(virus_info));	// create message
		send_message(monitor->write_fd, MSG2, response_msg, monitor->bufferSize);				// send message
	}

	/* when you are done with sending the bloom filters, notify parent that you are done and ready for commands */
	send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);
	return 0;
}

int read_subdir(struct Monitor * monitor, char * subdir)
{
	struct dirent ** file_list;
	int n;
	char * country_name;

	for (int i = (strlen(subdir) - 1); i >=0; i--)		// traverse the subdir backwards to find the last '/' to find the country name
	{
		if (subdir[i] == '/')
		{
			country_name = subdir + i + 1;
			break;
		}
	}

	if ( (n = scandir(subdir, &file_list, NULL, alphasort)) < 0)		// we use scandir to iterate over files in alphabetical order
	{
		perror("[Error] : read_subdir -> scandir\n");
		return -1;
	}
	else
	{
		for (int i = 0; i < n; ++i)
		{
			if (!strcmp(file_list[i]->d_name, ".") || !strcmp(file_list[i]->d_name, ".."))	// if you are at . or .. just ignore
			{
				free(file_list[i]);
				continue;
			}

			M_CountryInfo country_info1 = (M_CountryInfo) hash_search(monitor->countries_info, country_name);
			if (country_info1 != NULL)	// if this country is already in countries HT
			{
				if (m_country_search_file(country_info1, file_list[i]->d_name) != NULL)		// and this file has already been read
				{
					free(file_list[i]);
					continue;			// just ignore it and continue
				}
			}
			
			char * line = NULL;
			char * citizenID , * firstName, * lastName, * country, * virusName, * vacc, * date;
			int age;
    		size_t length = 0;

    		FILE *file_ptr;
    		char file_path[40];
    		if (snprintf(file_path, 40, "%s/%s", subdir, file_list[i]->d_name) < 0)
    		{
    			fprintf(stderr, "[Error] : read_subdir -> snprintf\n");
    			return -1;
    		}

			file_ptr = fopen(file_path, "r");  /*open citizen records txt file , in read mode*/
			if (file_ptr == NULL)
			{
			    fprintf(stderr, "[Error] : read_subdir -> fopen, could not open file\n");
			    return -1;
			}
			else    /*following block of code reads from the file and inserts the entries of file*/
			{
				// check if file is empty first
				int c = fgetc(file_ptr);	// read char from file
				if (c == EOF) // file is empty
				{
					free(file_list[i]);
					continue;		// ignore it and continue
				}
				ungetc(c, file_ptr);	// else undo char read
			  	while(getline(&line, &length, file_ptr) != -1)
			    {
			    	date = NULL;
			    	line[strlen(line)-1] = '\0';		// remove newline character from line read from file
			      	char *str = strtok(line, " ");
			      	int i = 1;
			      	while(str != NULL)
			      	{
			         	switch (i)
			         	{
			         		case 1: citizenID = str; break;
			         		case 2: firstName = str; break;
			         		case 3: lastName = str; break;
			         		case 4: country = str; break;
			         		case 5: age = atoi(str); break;
			         		case 6: virusName = str; break;
			         		case 7: vacc = str; break;
			         		case 8: date = str; break;
			         	}

			         	i++;
			         	str = strtok(NULL, " ");
			      	}
			     
			      	Monitor_insert(monitor, citizenID, firstName, lastName, country, age, virusName, vacc, date);			     
			     }

			    free(line);
			}

			M_CountryInfo country_info = (M_CountryInfo) hash_search(monitor->countries_info, country_name);
			m_country_add_file(country_info, file_list[i]->d_name);

			fclose(file_ptr);
			free(file_list[i]);
		}

		free(file_list);
		return 0;	// normal return, everything went smoothly
	}
}


void Monitor_insert(struct Monitor * monitor, char * citizenID , char * firstName, char * lastName, char * country, unsigned int age, char * virusName, char * vacc, char * date)
{

	// search for an already existing citizen record with same ID
	M_CitizenInfo citizen_info = (M_CitizenInfo) hash_search(monitor->citizens_info, citizenID);
	// search for an already existing virus record with given name
	M_VirusInfo virus_info = (M_VirusInfo) hash_search(monitor->viruses_info, virusName);
	// search for an already existing country record with given name
	M_CountryInfo country_info = (M_CountryInfo) hash_search(monitor->countries_info, country);

	if (citizen_info != NULL)		// if a citizen record with same ID already exists
	{
		// check if new record is inconsistent
		if (strcmp(firstName, m_get_citizen_name(citizen_info)) != 0 || strcmp(lastName, m_get_citizen_surname(citizen_info)) != 0 
			|| strcmp(country, m_get_citizen_country(citizen_info)) != 0 || age != m_get_citizen_age(citizen_info))
		{
			printf("ERROR IN RECORD : %s %s %s %s %d %s %s ", citizenID, firstName, lastName, country, age, virusName, vacc); printf( (date == NULL) ? "\n" : "%s\n", date);
			printf("INCONSISTENT INPUT DATA\n\n");
			return;
		}

		if (virus_info != NULL)
		{
			char * temp_date;
			// check if new record is duplicate (same ID, but also same virus - that means, an entry with given ID already exists for given virus)
			// if it exists it is either on the vaccinated skip list or non vaccinated skip list for given virus
			if (skip_list_search(m_get_vacc_list(virus_info), citizenID, &temp_date) || skip_list_search(m_get_non_vacc_list(virus_info), citizenID, &temp_date))
			{
				printf("ERROR IN RECORD : %s %s %s %s %d %s %s ", citizenID, firstName, lastName, country, age, virusName, vacc); printf( (date == NULL) ? "\n" : "%s\n", date);
				printf("INPUT DATA DUPLICATION\n\n");
				return;
			}
		}
	}

	// at last, check for invalid data form, i.e. vaccinated == "YES" but no date is given or vaccinated = "NO" but a date is given
	if ( ( !strcmp(vacc, "YES") && date == NULL) || (!strcmp(vacc, "NO") && date != NULL) )
	{
		printf("ERROR IN RECORD : %s %s %s %s %d %s %s ", citizenID, firstName, lastName, country, age, virusName, vacc); printf( (date == NULL) ? "\n" : "%s\n", date);
		printf("INVALID INPUT DATA FORM\n\n");
		return;
	}

	if (country_info == NULL)
	{
		country_info = m_country_info_create(country);		// if given country name is new, create new country record
		hash_insert(monitor->countries_info, country_info);			// insert it into countries index for future reference
	}

	if (citizen_info == NULL)			// given record is a new citizen record (new ID)
	{
		citizen_info = m_citizen_info_create(citizenID, firstName, lastName, age, country_info);	// create new citizen record
		hash_insert(monitor->citizens_info, citizen_info);				// insert it into citizens index for future reference
	}

	if (virus_info == NULL)
	{
		virus_info = m_virus_info_create(virusName, monitor->bloom_size, monitor->max_level, monitor->p);
		hash_insert(monitor->viruses_info, virus_info);
	}

	// insert citizen into bloom filter, correct skip list, of given virus
	if (!strcmp(vacc, "YES"))
	{
		bloom_insert(m_get_bloom_filter(virus_info), (unsigned char*) citizenID);	// bloom filter of virus, keeps track of the vaccinated citizens
		skip_list_insert(m_get_vacc_list(virus_info), citizen_info, date);		// insert into vaccinated persons skip list if citizen was vaccinated
	}
	else
		skip_list_insert(m_get_non_vacc_list(virus_info), citizen_info, date);	// insert into not vaccinated skip list if citizen was not vaccinated
	
}



/* =================== QUERY PHASE ========================= */

/* wrapper function that calls a specific function to take action based on message received */
int Monitor_take_action(struct Monitor * monitor, int msgd, void * message, char * subdir)
{
	if (msgd != MSG1 && msgd != MSG3 && msgd != MSG5 && msgd != MSG8)		// Monitor handles message descriptors that refer to him only
		return -1;
	if (msgd == MSG1)
	{
		memset(subdir, '\0', strlen(subdir));
		if (decode_msg1(msgd, message, subdir) < 0)					// decode message of type MSG1
			return -1;
		// subdir is now initialized after decoding message
		// no action is taken, we just return, Monitor expects to receive a SIGUSR1 now asychronously
		send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);	// notify parent you are done
	}
	else if (msgd == MSG3)
	{
		char citizenID[6], virus[20];
		if (decode_msg3(msgd, message, citizenID, virus) < 0)		// decode message of type MSG3
			return -1;
		vaccineStatus(monitor, citizenID, virus);
	}
	else if (msgd == MSG5)
	{
		char citizenID[6];
		if (decode_msg5(msgd, message, citizenID) < 0)				// decode message of type MSG5
			return -1;
		vaccineStatus(monitor, citizenID, NULL);
	}
	else if (msgd == MSG8)
	{
		int result;
		if (decode_msg8(msgd, message, &result) < 0)
			return -1;
		if (result == 1)
			monitor->accepted += 1;
		else
			monitor->rejected += 1;

		send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);	// notify parent you are done and move on to other commands
	}

	delete_message(message);  // message no longer needed
	return 0;	
}

void vaccineStatus(struct Monitor * monitor, char * citizenID, char * virusName)
{
	// search for an existing cititzen record with given citizen ID
	M_CitizenInfo citizen_info = (M_CitizenInfo) hash_search(monitor->citizens_info, citizenID);
	if (citizen_info == NULL && virusName != NULL)
	{
		fprintf(stderr, "Error : Monitor -> vaccineStatus -> Given citizen ID does not exist in Monitor's database\n\n");
		return;
	}

	if (virusName != NULL)		// if specific virusName was given (i.e. query /travelRequest )
	{
		// search for an existing virus record with given virus name
		M_VirusInfo virus_info = (M_VirusInfo) hash_search(monitor->viruses_info, virusName);
		if (virus_info == NULL)
		{
			fprintf(stderr, "Error : Monitor -> vaccineStatus -> Given virus name does not exist in Monitors's database\n\n");
			return;
		}

		char * date = NULL;
		void * message;

		if (!skip_list_search(m_get_vacc_list(virus_info), citizenID, &date))		// if citizen id was not found into vaccinated skip list for given virus
		{
			message = create_msg4("NO", date);		// construct message
			//monitor->rejected += 1;
		}
		else
		{
			message = create_msg4("YES", date);		// construct message
			//monitor->accepted += 1;
		}

		send_message(monitor->write_fd, MSG4, message, monitor->bufferSize);	// send message
	}
	else		// no specific virusName was given (i.e. query /searchVaccinationStatus )
	{
		if (citizen_info != NULL)	// if no info found for given citizenID, we just send back DONE immediately
		{
			// create message of type MSG6
			void * message = create_msg6(m_get_citizen_name(citizen_info), m_get_citizen_surname(citizen_info), m_get_citizen_country(citizen_info), m_get_citizen_age(citizen_info));
			send_message(monitor->write_fd, MSG6, message, monitor->bufferSize);  // to start things off, send back name,surname,country,age about given citizenID 
			// and then send back all vaccination info you can find for given citizenID
			M_VirusInfo virus_info;
			// iterate upon the hash-table of viruses
			while ((virus_info = hash_iterate_next(monitor->viruses_info)) != NULL)
			{
				char * date = NULL;
				if (skip_list_search(m_get_vacc_list(virus_info), citizenID, &date))		// if citizen id was found into vaccinated skip list for given virus
				{
					void * message = create_msg7(m_get_virus_name(virus_info), "YES", date);	// create message of type MSG7
					send_message(monitor->write_fd, MSG7, message, monitor->bufferSize);
				}
				else if (skip_list_search(m_get_non_vacc_list(virus_info), citizenID, &date)) // if citizen id was found into not vaccinated list for given virus
				{
					void * message = create_msg7(m_get_virus_name(virus_info), "NO", date);
					send_message(monitor->write_fd, MSG7, message, monitor->bufferSize);
				}
				// if citizen is not associated with particular virus, then we dont send back anything
			}
			
		}
		/* when you are done with sending vaccination info, notify parent that you are done and ready for other commands */
		send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);
	}
}


/*==================== EXIT PHASE ========================== */

void Monitor_del(struct Monitor * monitor)
{
	hash_destroy(monitor->countries_info);
	hash_destroy(monitor->citizens_info);
	hash_destroy(monitor->viruses_info);
	close(monitor->read_fd);
	close(monitor->write_fd);
	free(monitor);
}


/* ================== SIGNALS ============================== */

void m_log_file_print(struct Monitor * monitor)
{
	FILE * file_ptr;
	char log_file_name[40];
	snprintf(log_file_name, 40, "log_file.%d.txt", getpid());	// create log file name
	file_ptr = fopen(log_file_name, "w");						// open file for writing (if exists already we just overwrite it)
	if (file_ptr == NULL)						
	{
		fprintf(stderr, "[Error] : Monitor -> log_file_print -> Could not open/create log file\n\n");
		return;
	}

	M_CountryInfo country_info;
	// iterate upon the hash-table of countries of Monitor
	while ((country_info = (M_CountryInfo) hash_iterate_next(monitor->countries_info)) != NULL)
		fprintf(file_ptr, "%s\n", m_get_country_name(country_info));				// print all countries that participated in travelMonitor
	fprintf(file_ptr, "TOTAL TRAVEL REQUESTS %d\n", monitor->accepted + monitor->rejected);		// print total travel requests
	fprintf(file_ptr, "ACCEPTED %d\n", monitor->accepted);								// print the #accepted
	fprintf(file_ptr, "REJECTED %d\n", monitor->rejected);								// print the #rejected
	fclose(file_ptr);
}


int read_subdir_updates(struct Monitor * monitor, char * subdir)
{
	if (read_subdir(monitor, subdir) < 0)					/* read subdirectory-country sent by travelMonitor and make the updates */
		return -1;
	// send back the bloom filters
	M_VirusInfo virus_info;
	// iterate upon the hash-table of viruses
	while ((virus_info = hash_iterate_next(monitor->viruses_info)) != NULL)
	{
		void * response_msg = create_msg2(m_get_virus_name(virus_info), monitor->bloom_size , m_get_bloom_filter(virus_info));	// create message
		send_message(monitor->write_fd, MSG2, response_msg, monitor->bufferSize);				// send message
	}

	/* when you are done with sending the updated bloom filters, notify parent that you are done and ready for other commands */
	send_message(monitor->write_fd, DONE, NULL, monitor->bufferSize);
	return 0;

}