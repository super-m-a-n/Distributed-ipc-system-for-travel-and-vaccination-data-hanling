/* file : tm_items.h (travel monitor items) */
#pragma once
#include "bloom.h"

typedef struct tm_virus_info * TM_VirusInfo;
typedef struct tm_country_info * TM_CountryInfo;
typedef struct tm_travelRequest * TM_TravelRequest;

TM_VirusInfo tm_virus_info_create(char * virus_name, unsigned int bloom_size, void * bit_array);
void tm_virus_info_destroy(TM_VirusInfo info);
char * tm_get_virus_name(TM_VirusInfo info);
Bloom tm_get_bloom_filter(TM_VirusInfo info);
void tm_virus_info_print(TM_VirusInfo info);

/*_____________________________________________________________________________________________________*/

TM_CountryInfo tm_country_info_create(char * country_name, int monitor_index);
void tm_country_info_destroy(TM_CountryInfo info);
char * tm_get_country_name(TM_CountryInfo info);
int tm_get_country_monitor(TM_CountryInfo info);
void tm_country_info_print(TM_CountryInfo info);
void tm_country_add_travelRequest(TM_CountryInfo info, char * date, char * virus, int result);
void tm_get_country_travelStats(TM_CountryInfo info, char * virusName, char * date1, char * date2, int * accepted, int * rejected);

/*_____________________________________________________________________________________________________*/

TM_TravelRequest tm_travelRequest_create(char * date, char * virus, int result);
void tm_travelRequest_destroy(TM_TravelRequest request);
char * tm_get_travelRequest_date(TM_TravelRequest request);
char * tm_get_travelRequest_virus(TM_TravelRequest request);
int tm_get_travelRequest_result(TM_TravelRequest request);