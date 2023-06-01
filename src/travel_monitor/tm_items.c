/* file : tm_items.c (travel monitor items) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bloom.h"
#include "list.h"
#include "date.h"
#include "tm_items.h"
#include <assert.h>

struct tm_virus_info {
	char * virus_name;						// name of the virus
	Bloom bloom_filter;						// bloom filter for virus
};


struct tm_travelRequest {	// a struct that keeps information for a travel Request
	char * date;			// a date of travel
	char * virus;			// a virus for whom vaccination is checked
	int result;				// a result 1 for accepted, 0 for rejected
};

struct tm_country_info {
	char * country_name;
	int monitor_index;			// index/number of monitor that monitors this country
	List travel_requests;		// a list of travel requests (list of struct tm_travel_info)
};



TM_VirusInfo tm_virus_info_create(char * virus_name, unsigned int bloom_size, void * bit_array)
{
	TM_VirusInfo info = malloc(sizeof(struct tm_virus_info));
	if (info == NULL)
		fprintf(stderr, "Error : tm_virus_info_create -> malloc\n");
	assert(info != NULL);

	info->virus_name = malloc(strlen(virus_name) + 1);
	strcpy(info->virus_name, virus_name);

	info->bloom_filter = bloom_copy_create(bloom_size, bit_array);
	return info;
}

void tm_virus_info_destroy(TM_VirusInfo info)
{
	if (info == NULL)
		fprintf(stderr, "Error : tm_virus_info_destroy -> info is NULL\n");
	assert(info != NULL);

	free(info->virus_name);
	bloom_destroy(info->bloom_filter);
	
	free(info);
}

char * tm_get_virus_name(TM_VirusInfo info)
{
	return info->virus_name;
}

Bloom tm_get_bloom_filter(TM_VirusInfo info)
{
	return info->bloom_filter;
}

void tm_virus_info_print(TM_VirusInfo info)
{
	printf("%s\n", info->virus_name);
}


/*_______________________________________________________________*/


TM_CountryInfo tm_country_info_create(char * country_name, int monitor_index)
{
	TM_CountryInfo info = malloc(sizeof(struct tm_country_info));
	if (info == NULL)
		fprintf(stderr, "Error : tm_country_info_create -> malloc\n");
	assert(info != NULL);

	info->country_name = malloc(strlen(country_name) + 1);
	strcpy(info->country_name, country_name);
	info->monitor_index = monitor_index;
	info->travel_requests = list_create(6);

	return info;
}

void tm_country_info_destroy(TM_CountryInfo info)
{
	if (info == NULL)
		fprintf(stderr, "Error : tm_country_info_destroy -> info is NULL\n");
	assert(info != NULL);

	free(info->country_name);
	list_destroy(info->travel_requests);
	free(info);
}

char * tm_get_country_name(TM_CountryInfo info)
{
	return info->country_name;
}

int tm_get_country_monitor(TM_CountryInfo info)
{
	return info->monitor_index;
}

void tm_country_info_print(TM_CountryInfo info)
{
	printf("Country %s,  monitor %d\n", info->country_name, info->monitor_index);
}

void tm_country_add_travelRequest(TM_CountryInfo info, char * date, char * virus, int result)
{
	if (info == NULL)
		fprintf(stderr, "Error : tm_country_add_travelRequest -> info is NULL\n");
	assert(info != NULL);

	TM_TravelRequest request = tm_travelRequest_create(date, virus, result);
	list_insert_end(info->travel_requests, request);
}

void tm_get_country_travelStats(TM_CountryInfo info, char * virusName, char * date1, char * date2, int * accepted, int * rejected)
{
	*accepted = 0; *rejected = 0;
	for (ListNode node = list_first(info->travel_requests); node != NULL; node = list_next(info->travel_requests, node)) // iterate list of travelRequests for country
	{
		if (!strcmp(((TM_TravelRequest) list_value(info->travel_requests, node))->virus, virusName))		// if virus of travel request is the one checked
		{
			if (date_cmp(date1, ((TM_TravelRequest) list_value(info->travel_requests, node))->date) <= 0 && 
				date_cmp(((TM_TravelRequest) list_value(info->travel_requests, node))->date, date2) <= 0) // and if the date of request is within the given interval
			{
				if (((TM_TravelRequest) list_value(info->travel_requests, node))->result)	// update the accepted and rejected counters
					*accepted += 1;		
				else
					*rejected += 1;
			}
		}
	}
}

/*_______________________________________________________________*/

TM_TravelRequest tm_travelRequest_create(char * date, char * virus, int result)
{
	TM_TravelRequest request = malloc(sizeof(struct tm_travelRequest));
	if (request == NULL)
		fprintf(stderr, "Error : tm_travelRequest_create -> malloc\n");
	assert(request != NULL);

	request->date = malloc(strlen(date) + 1);
	strcpy(request->date, date);
	request->virus = malloc(strlen(virus) + 1);
	strcpy(request->virus, virus);
	request->result = result;

	return request;
}

void tm_travelRequest_destroy(TM_TravelRequest request)
{
	if (request == NULL)
		fprintf(stderr, "Error : tm_travelRequest_destroy -> request is NULL\n");
	assert(request != NULL);

	free(request->date);
	free(request->virus);
	free(request);
}

char * tm_get_travelRequest_date(TM_TravelRequest request)
{
	return request->date;
}

char * tm_get_travelRequest_virus(TM_TravelRequest request)
{
	return request->virus;
}

int tm_get_travelRequest_result(TM_TravelRequest request)
{
	return request->result;
}