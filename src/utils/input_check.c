/* file : input_check.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include "input_check.h"
#include "tm_helper.h"

/* checks for correct input args from terminal and initializes program parameters if so */
bool check_init_args(int argc, const char ** argv, int * numMonitors, int * bufferSize, unsigned int * bloom_size, DIR ** dir)
{
	if (argc != 9)
	{
		fprintf(stderr, "Error: wrong number of args\nUse: ./travelMonitor -m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
		return false;
	}

	if (strcmp(argv[1], "-m") != 0 || strcmp(argv[3], "-b") != 0 || strcmp(argv[5], "-s") != 0 || strcmp(argv[7], "-i") != 0)
	{
		fprintf(stderr, "Error: one or more wrong input parameters\n Use : -m -b -s -i\n");
		return false;
	}

	// check if numMonitors is indeed a positive integer
	if (!is_integer(argv[2]) || !atoi(argv[2]))
	{
		fprintf(stderr, "Error: invalid input parameter numMonitors\n Use : numMonitors --> positive integer\n");
		return false;
	}
	
	*numMonitors = atoi(argv[2]);

	// check if bufferSize is indeed a positive integer and at least 4 bytes
	if (!is_integer(argv[4]) || !atoi(argv[4]) || atoi(argv[4]) < sizeof(int))
	{
		fprintf(stderr, "Error: invalid input parameter bufferSize\n Use : bufferSize --> positive integer >= sizeof(int) \n");
		return false;
	}

	*bufferSize = atoi(argv[4]);

	// check if bloom_size is indeed a positive integer
	if (!is_integer(argv[6]) || !atoi(argv[6]))
	{
		fprintf(stderr, "Error: invalid input parameter bloom_size\n Use : bloom_size --> positive integer\n");
		return false;
	}

	*bloom_size = atoi(argv[6]);
	
	// check if given directory given is valid (can be opened)
	if ((*dir = opendir(argv[8])) == NULL)
	{
		fprintf(stderr, "Error: invalid input parameter input_dir\n Cannot open directory\n");
		return false;
	}

	return true;	// everything goes fine
}

bool is_integer(const char * string)
{
	for (int i = 0; i < strlen(string); i++)
	{
		if (string[i] < '0' || string[i] > '9')
			return false;
	}

	return true;
}

bool check_cmd_args(struct travelMonitor * travelMonitor, char * input, const char * input_dir_name)
{	
	char * citizenID , * date, * countryFrom, * countryTo, * virusName, * date1, * date2, * country;
	if (!strcmp(input, "/exit"))
	{
		exit_travelMonitor(travelMonitor);
		return true;
	}
	else
	{
		char *str = strtok(input, " ");
	    if (!strcmp(str, "/travelRequest"))
	    {
	    	int i = 0;
		    while(str != NULL)
		    {
		       switch (i)
		       {
					case 1: citizenID = str; break;
			        case 2: date = str; break;
			        case 3: countryFrom = str; break;
			        case 4: countryTo = str; break;
			        case 5: virusName = str; break;
		        }

		        i++;
		        str = strtok(NULL, " ");
		    }

	      	if (i != 6)
	      		printf("Error : unknown or invalid command\n\n");
	      	else
	      		travelRequest(travelMonitor, citizenID, date, countryFrom, countryTo, virusName);
	    }
	    else if (!strcmp(str, "/travelStats"))
	    {
	      	int i = 0;
	      	country = NULL;
	      	while(str != NULL)
	      	{
	         	switch (i)
	         	{
	         		case 1: virusName = str; break;
	         		case 2: date1 = str; break;
	         		case 3: date2 = str; break;
	         		case 4: country = str; break;
	         	}

	         	i++;
	         	str = strtok(NULL, " ");
	      	}

	      	if (i != 5 && i != 4)
	      		printf("Error : unknown or invalid command\n\n");
	      	else	  
	      		travelStats(travelMonitor, virusName, date1, date2, country);
	    }
	    else if (!strcmp(str, "/addVaccinationRecords"))
	    {
	      	int i = 0;
	      	while(str != NULL)
	      	{
	         	switch (i)
	         	{
	         		case 1: country = str; break;
	         	}
	         
	         	i++;
	         	str = strtok(NULL, " ");
	      	}

	      	if (i != 2)
	      		printf("Error : unknown or invalid command\n\n");
	      	else
	      		addVaccinationRecords(travelMonitor, country, input_dir_name);
	    }
		else if (!strcmp(str, "/searchVaccinationStatus"))
	    {
	      	int i = 0;	      
	      	while(str != NULL)
	      	{
	         	switch (i)
	         	{
	         		case 1: citizenID = str; break;
	         	}

	         	i++;
	         	str = strtok(NULL, " ");
	      	}

	      	if (i != 2)
	      		printf("Error : unknown or invalid command\n\n");
	      	else	  
	      		searchVaccinationStatus(travelMonitor, citizenID);
	    }
		else
	      	printf("Error : unknown or invalid command\n\n");
	}

	return false;  // if command was not /exit, continue receiving commands from cmd line
}
