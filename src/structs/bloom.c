/*file : bloom.c*/
#include <stdlib.h>
#include <stdio.h>
#include "bloom.h"
#include <string.h>
#include <assert.h>

/* first hash function : djb2*/
unsigned long djb2(unsigned char *str) {
	unsigned long hash = 5381;
	int c; 
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

/* second hash function : sdbm*/
unsigned long sdbm(unsigned char *str) {
	unsigned long hash = 0;
	int c;

	while ((c = *str++)) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

 
/* Return the result of the Kth hash function. This function uses djb2 and sdbm. */
unsigned long hash_i(unsigned char *str, unsigned int i) {
	return djb2(str) + i*sdbm(str) + i*i;
}


Bloom bloom_create(unsigned int bloom_size)
{
	Bloom bloom = malloc(sizeof(*bloom));	// malloc bloom pointer to the bloom filter structure
	if (bloom == NULL)
		fprintf(stderr, "Error : bloom_create -> malloc\n");
	assert(bloom != NULL);

	bloom->bit_array = malloc(bloom_size * sizeof(uint8_t));	// malloc the bit array to have bloom_size 8-bit integers so total of 8*bloom_size bits
	if (bloom->bit_array == NULL)
		fprintf(stderr, "Error : bloom_create -> malloc\n");
	assert(bloom->bit_array != NULL);

	for (unsigned int i = 0; i < bloom_size; i++)
		bloom->bit_array[i] = 0;		// initially all bits and hence all bytes of bit array are set to zero

	bloom->size = bloom_size * 8;	// keep the number of bits of the bloom filter

	return bloom;

}

Bloom bloom_copy_create(unsigned int bloom_size, void * bit_array)
{
	Bloom bloom = malloc(sizeof(*bloom));	// malloc bloom pointer to the bloom filter structure
	if (bloom == NULL)
		fprintf(stderr, "Error : bloom_copy_create -> malloc\n");
	assert(bloom != NULL);

	bloom->bit_array = malloc(bloom_size * sizeof(uint8_t));	// malloc the bit array to have bloom_size 8-bit integers so total of 8*bloom_size bits
	if (bloom->bit_array == NULL)
		fprintf(stderr, "Error : bloom_copy_create -> malloc\n");
	assert(bloom->bit_array != NULL);
	
	memcpy(bloom->bit_array, bit_array, bloom_size * sizeof(uint8_t));	// copy the bit_array with the one given
	bloom->size = bloom_size * 8;
	return bloom;
}

void bloom_bit_array_copy(Bloom bloom, void * bit_array)
{
	memcpy(bloom->bit_array, bit_array, ((bloom->size)/8) * sizeof(uint8_t));	// just copy the bit_array with the one given
}

bool bloom_check(Bloom bloom, unsigned char * string)
{
	if (bloom == NULL)
		fprintf(stderr, "Error : bloom_check -> bloom is NULL\n");
	assert(bloom != NULL);

	bool maybe_in = true;	// initially assume that the given object may be into the bloom filter (either positive or false positive)
	for (int i = 0; i < K; i++)		// for all k hash functions
	{
		unsigned long pos = hash_i(string, i) % bloom->size;	// get bit position in bit array as returned from hash function
		// this following line isolates the 8-bit number where our bit of interest is found (pos/8)
		// sets all other bits of the 8-bit number to zero. If the remaining number is zero that means the bit of interest (pos%8) is also zero. Otherwise it is 1
		if ((bloom->bit_array[pos/8] & (1 << (pos % 8))) == 0)
		{							
			maybe_in = false;	// bit indicated by current hash function is zero, that means that the given object definitely isn't in the bloom-filter
			break;
		}
	}

	// if all the bits indicated by the hash-functions were 1, then right here maybe_in will be 1 as well

	return maybe_in;
}

void bloom_insert(Bloom bloom, unsigned char * string)
{
	if (bloom == NULL)
		fprintf(stderr, "Error : bloom_insert -> bloom is NULL\n");
	assert(bloom != NULL);

	for (int i = 0; i < K; i++)	// for all k hash functions
	{
		unsigned long pos = hash_i(string, i) % bloom->size;	// get bit position in bit array as returned from hash function
		// set bit at position by doing a bitwise or of the 8-bit number where our bit of interest is found
		// with another 8-bit number which has 1 only at the position of our bit of interest
		bloom->bit_array[pos/8]  = bloom->bit_array[pos/8] | (1 << (pos % 8));
		
	}

}

void bloom_destroy(Bloom bloom)
{
	if (bloom == NULL)
		fprintf(stderr, "Error : bloom_delete -> bloom is NULL\n");
	assert(bloom != NULL);

	free(bloom->bit_array);		// free bit array of bloom filter
	free(bloom);		// free the pointer to the bloom filter structure itself

}