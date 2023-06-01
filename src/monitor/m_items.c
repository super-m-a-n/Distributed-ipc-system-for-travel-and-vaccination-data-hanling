/* file : m_items.c (monitor items) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bloom.h"
#include "skip_list.h"
#include "list.h"
#include "m_items.h"
#include <assert.h>

struct m_citizen_info {
	char * id;
	char * name;
	char * surname;
	int age;
	M_CountryInfo country;
};

struct m_virus_info {
	char * virus_name;						// name of the virus
	Bloom bloom_filter;						// bloom filter for virus
	SkipList vaccinated_persons;			// vaccinated persons skip list for virus
	SkipList not_vaccinated_persons;		// not vaccinated persons skip list for virus
};

struct m_country_info {
	char * country_name;
	List read_files;
	unsigned long population;
};

M_CitizenInfo m_citizen_info_create(char * id, char * name, char * surname, int age, M_CountryInfo country)
{
	M_CitizenInfo info = malloc(sizeof(struct m_citizen_info));
	if (info == NULL)
		fprintf(stderr, "Error : m_citizen_info_create -> malloc\n");
	assert(info != NULL);

	info->id = malloc(strlen(id) + 1);
	strcpy(info->id, id);
	info->name = malloc(strlen(name) + 1);
	strcpy(info->name, name);
	info->surname = malloc(strlen(surname) + 1);
	strcpy(info->surname, surname);
	info->age = age;
	info->country = country;
	m_country_population_inc(country);		// new citizen from given country was recorded and inserted into database

	return info;
}

void m_citizen_info_destroy(M_CitizenInfo info)
{
	if (info == NULL)
		fprintf(stderr, "Error : m_citizen_info_delete -> malloc\n");
	assert(info != NULL);

	free(info->id);
	free(info->name);
	free(info->surname);
	free(info);
}

char * m_get_citizen_id(M_CitizenInfo info)
{
	return info->id;
}

char * m_get_citizen_name(M_CitizenInfo info)
{
	return info->name;
}

char * m_get_citizen_surname(M_CitizenInfo info)
{
	return info->surname;
}

char * m_get_citizen_country(M_CitizenInfo info)
{
	return m_get_country_name(info->country);
}

int m_get_citizen_age(M_CitizenInfo info)
{
	return info->age;
}

void m_citizen_info_print(M_CitizenInfo info)
{
	printf("%s %s %s %s %d\n", info->id, info->name, info->surname, m_get_country_name(info->country), info->age);
}

/*_______________________________________________________________________________________________________________*/


M_VirusInfo m_virus_info_create(char * virus_name, unsigned int bloom_size, int max_level, float p)
{
	M_VirusInfo info = malloc(sizeof(struct m_virus_info));
	if (info == NULL)
		fprintf(stderr, "Error : m_virus_info_create -> malloc\n");
	assert(info != NULL);

	info->virus_name = malloc(strlen(virus_name) + 1);
	strcpy(info->virus_name, virus_name);

	info->bloom_filter = bloom_create(bloom_size);
	info->vaccinated_persons = skip_list_create(max_level, p);
	info->not_vaccinated_persons = skip_list_create(max_level, p);

	return info;
}

void m_virus_info_destroy(M_VirusInfo info)
{
	if (info == NULL)
		fprintf(stderr, "Error : m_virus_info_destroy -> info is NULL\n");
	assert(info != NULL);

	free(info->virus_name);
	bloom_destroy(info->bloom_filter);
	skip_list_destroy(info->vaccinated_persons);
	skip_list_destroy(info->not_vaccinated_persons);

	free(info);
}

char * m_get_virus_name(M_VirusInfo info)
{
	return info->virus_name;
}

Bloom m_get_bloom_filter(M_VirusInfo info)
{
	return info->bloom_filter;
}

SkipList m_get_vacc_list(M_VirusInfo info)
{
	return info->vaccinated_persons;
}

SkipList m_get_non_vacc_list(M_VirusInfo info)
{
	return info->not_vaccinated_persons;
}

void m_virus_info_print(M_VirusInfo info)
{
	printf("%s\n", info->virus_name);
	printf("Vaccinated People skip list:\n\n");
	skip_list_print(info->vaccinated_persons);
	printf("Not Vaccinated People skip list:\n\n");
	skip_list_print(info->not_vaccinated_persons);
}

/*_______________________________________________________________*/


M_CountryInfo m_country_info_create(char * country_name)
{
	M_CountryInfo info = malloc(sizeof(struct m_country_info));
	if (info == NULL)
		fprintf(stderr, "Error : m_country_info_create -> malloc\n");
	assert(info != NULL);

	info->country_name = malloc(strlen(country_name) + 1);
	strcpy(info->country_name, country_name);
	info->population = 0;
	info->read_files = list_create(3);

	return info;
}

void m_country_info_destroy(M_CountryInfo info)
{
	if (info == NULL)
		fprintf(stderr, "Error : m_country_info_destroy -> info is NULL\n");
	assert(info != NULL);

	free(info->country_name);
	list_destroy(info->read_files);
	free(info);
}

void m_country_add_file(M_CountryInfo info, char * file_name)
{
	if (info == NULL)
		fprintf(stderr, "Error : m_country_add_file -> info is NULL\n");
	assert(info != NULL);

	list_insert_end(info->read_files, file_name);
}

void * m_country_search_file(M_CountryInfo info, char * file_name)
{
	if (info == NULL)
		fprintf(stderr, "Error : m_country_search_file -> info is NULL\n");
	assert(info != NULL);

	return list_search(info->read_files, file_name);
}

char * m_get_country_name(M_CountryInfo info)
{
	return info->country_name;
}

void m_country_population_inc(M_CountryInfo info)
{
	info->population++;
}

unsigned long m_country_population(M_CountryInfo info)
{
	return info->population;
}

void m_country_info_print(M_CountryInfo info)
{
	printf("%s\n", info->country_name);
}

/*______________________________________________________________________*/