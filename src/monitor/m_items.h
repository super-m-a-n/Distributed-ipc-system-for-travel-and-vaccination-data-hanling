/* file : m_items.h (monitor items) */
#pragma once
#include "bloom.h"
#include "skip_list.h"

typedef struct m_citizen_info * M_CitizenInfo;
typedef struct m_virus_info * M_VirusInfo;
typedef struct m_country_info * M_CountryInfo;

M_CitizenInfo m_citizen_info_create(char * id, char * name, char * surname, int age, M_CountryInfo country);
void m_citizen_info_destroy(M_CitizenInfo info);
char * m_get_citizen_id(M_CitizenInfo info);
char * m_get_citizen_name(M_CitizenInfo info);
char * m_get_citizen_surname(M_CitizenInfo info);
char * m_get_citizen_country(M_CitizenInfo info);
int m_get_citizen_age(M_CitizenInfo info);
void m_citizen_info_print(M_CitizenInfo info);

/*____________________________________________________________________________________________________*/

M_VirusInfo m_virus_info_create(char * virus_name, unsigned int bloom_size, int max_level, float p);
void m_virus_info_destroy(M_VirusInfo info);
char * m_get_virus_name(M_VirusInfo info);
Bloom m_get_bloom_filter(M_VirusInfo info);
SkipList m_get_vacc_list(M_VirusInfo info);
SkipList m_get_non_vacc_list(M_VirusInfo info);
void m_virus_info_print(M_VirusInfo info);

/*_____________________________________________________________________________________________________*/

M_CountryInfo m_country_info_create(char * country_name);
void m_country_info_destroy(M_CountryInfo info);
void m_country_add_file(M_CountryInfo info, char * file_name);
void * m_country_search_file(M_CountryInfo info, char * file_name);
char * m_get_country_name(M_CountryInfo info);
void m_country_population_inc(M_CountryInfo info);
unsigned long m_country_population(M_CountryInfo info);
void m_country_info_print(M_CountryInfo info);

/*_____________________________________________________________________________________________________*/