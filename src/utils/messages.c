/* file : messages.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "messages.h"
#include "bloom.h"

static unsigned int bloomSize;
void bloomSize_init(unsigned int bloom_size)
{
	bloomSize = bloom_size;
}

void * create_msg0(int bufferSize, unsigned int bloom_size)
{
	void * message = calloc(1, MSG0_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg0 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	memcpy(message, &bufferSize, sizeof(int));
	memcpy(message + sizeof(int), &bloom_size, sizeof(unsigned int));
	return message;
}

int decode_msg0(int msgd, void * message, int * bufferSize, unsigned int * bloom_size)
{
	if (msgd != MSG0)
	{
		fprintf(stderr, "[Error] : decode_msg0 -> Unexpected message descriptor\n\n");
		return -1;
	}
	memcpy(bufferSize, message, sizeof(int));
	memcpy(bloom_size, message + sizeof(int), sizeof(unsigned int));
	return 0;
}


void * create_msg1(const char * input_dir_name, char * subdir_name)
{
	void * message = calloc(1, MSG1_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg1 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	snprintf(message, MSG1_SIZE, "%s/%s", input_dir_name, subdir_name);
	return message;
}

int decode_msg1(int msgd, void * message, char * subdir)
{
	if (msgd != MSG1 && msgd != MSG1_NO_REPLY)		// check if msgd was the one expected
	{
		fprintf(stderr, "[Error] : decode_msg1 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(subdir, message, 30);
	return 0;
}

void * create_msg2(char * virus_name, unsigned int bloom_size, Bloom bloom_filter)
{
	void * message = calloc(1, MSG2_SIZE + bloom_size * sizeof(uint8_t));
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg2 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	int count = 0;
	for (unsigned int i = 0; i < bloom_size; i++)
		count += bloom_filter->bit_array[i];
	strncpy(message, virus_name, 20);
	memcpy(message + 20, bloom_filter->bit_array, bloom_size * sizeof(uint8_t));
	return message;
}

int decode_msg2(int msgd, void * message, char * virus, void ** bit_array)
{
	if (msgd != MSG2)	// check if msgd was the one expected
	{
		fprintf(stderr, "[Error] : decode_msg2 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(virus, message, 20);
	*bit_array = message + 20;
	return 0;
}

void * create_msg3(char * citizenID, char * virusName)
{
	void * message = calloc(1, MSG3_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg3 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	strncpy(message, citizenID, 6);
	strncpy(message + 6, virusName, 20);
	return message;
}

int decode_msg3(int msgd, void * message, char * citizenID, char * virus)
{
	if (msgd != MSG3)		// check if msgd was the one expected
	{
		fprintf(stderr, "[Error] : decode_msg3 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(citizenID, message, 6);
	strncpy(virus, message + 6, 20);
	return 0;
}

void * create_msg4(char * answer, char * date)
{
	void * message = calloc(1, MSG4_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg4 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	strncpy(message, answer, 4);
	if (!strcmp(answer, "YES"))
		strncpy(message + 4, date, 12);
	return message;
}

int decode_msg4(int msgd, void * message, char * answer, char * date)
{
	if (msgd != MSG4)		// check if msgd was the one expected
	{
		fprintf(stderr, "[Error] : decode_msg4 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(answer, message, 4);
	strncpy(date, message + 4, 12);
	return 0;
}

void * create_msg5(char * citizenID)
{
	void * message = calloc(1, MSG5_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg5 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	strncpy(message, citizenID, MSG5_SIZE);
	return message;
}

int decode_msg5(int msgd, void * message, char * citizenID)
{
	if (msgd != MSG5)
	{
		fprintf(stderr, "[Error] : decode_msg5 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(citizenID, message, MSG5_SIZE);
	return 0;
}

void * create_msg6(char * name, char * surname, char * country, int age)
{
	void * message = calloc(1, MSG6_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg6 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	strncpy(message, name, 13);
	strncpy(message + 13, surname, 13);
	strncpy(message + 26, country, 30);
	memcpy(message + 56, &age, sizeof(int));
	return message;
}

int decode_msg6(int msgd, void * message, char * name, char * surname, char * country, int * age)
{
	if (msgd != MSG6)
	{
		fprintf(stderr, "[Error] : decode_msg6 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(name, message, 13);
	strncpy(surname, message + 13, 13);
	strncpy(country, message + 26, 30);
	memcpy(age, message + 56, sizeof(int));
	return 0;
}

void * create_msg7(char * virusName, char * status, char * date)
{
	void * message = calloc(1, MSG7_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg7 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	strncpy(message, virusName, 20);
	strncpy(message + 20, status, 4);
	if (!strcmp(status, "YES"))
		strncpy(message + 24, date, 12);
	return message;
}

int decode_msg7(int msgd, void * message, char * virus, char * status, char * date)
{
	if (msgd != MSG7)
	{
		fprintf(stderr, "[Error] : decode_msg7 -> Unexpected message descriptor\n\n");
		return -1;
	}
	strncpy(virus, message, 20);
	strncpy(status, message + 20, 4);
	strncpy(date, message + 24, 12);
	return 0;
}

void * create_msg8(int result)
{
	void * message = calloc(1, MSG8_SIZE);
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : create_msg8 -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	memcpy(message, &result, sizeof(int));
	return message;
}

int decode_msg8(int msgd, void * message, int * result)
{
	if (msgd != MSG8)
	{
		fprintf(stderr, "[Error] : decode_msg8 -> Unexpected message descriptor\n\n");
		return -1;
	}
	memcpy(result, message, sizeof(int));
	return 0;
}


void send_message(int write_fd, int msgd, void * message, int bufferSize)
{
	size_t body_size;						// size of the body of the message (the actual data)
	int * header = calloc(1, sizeof(int));	// allocate space for the header
	if (header == NULL)
	{
		fprintf(stderr, "[Error] : send_message -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}

	*header = msgd;
	size_t header_size = sizeof(*header);	// size of the header of the message (the message descriptor)
	int * tmp_header = header;

	switch (msgd)
	{
		case DONE : body_size = 0; break;
		case MSG0 : body_size = MSG0_SIZE; break;
		case MSG1 : body_size = MSG1_SIZE; break;
		case MSG1_NO_REPLY : body_size = MSG1_SIZE; break;
		case MSG2 : body_size = MSG2_SIZE + bloomSize * sizeof(uint8_t); break;
		case MSG3 : body_size = MSG3_SIZE; break;
		case MSG4 : body_size = MSG4_SIZE; break;
		case MSG5 : body_size = MSG5_SIZE; break;
		case MSG6 : body_size = MSG6_SIZE; break;
		case MSG7 : body_size = MSG7_SIZE; break;
		case MSG8 : body_size = MSG8_SIZE; break;
		default : fprintf(stderr, "[Error] : invalid message descriptor\n"); exit(EXIT_FAILURE);
	}

	/* sending the message consists of 2 parts, the message descriptor and the body of the message */
	
	/* first we send the message descriptor */
	ssize_t ret;
	size_t total_pending = header_size;		/* total bytes pending to be sent */
	while (total_pending != 0)				/* while we have not written all of them */
	{
		size_t pending = (bufferSize < total_pending) ? bufferSize : total_pending;
		while (pending != 0 && (ret = write(write_fd, header, pending)) != 0) 		/* write in chunks of bufferSize bytes */
		{
 			if (ret == -1) 
 			{
 				if (errno == EINTR)								/* if write was interrupted by a signal continue */
 					continue;
 				if (errno == EWOULDBLOCK || errno == EAGAIN)	/* if not enough write space available yet just continue */
 					continue;
 				perror("[Error] : write -> send_message\n");	/* else a more serious error occured */
 				exit(EXIT_FAILURE);
 			}

 			pending -= ret;			/* update pending bytes counters */
 			header += ret;			 
 			total_pending -= ret;
		}
	}

	free(tmp_header);

	if(!body_size) return;  // if there is no data in message just return

	void * tmp_message = message;  // save message address to free later

	/* then we send the message itself (the data) */
	total_pending = body_size;
	while (total_pending != 0)				/* while we have not written all of them */
	{
		size_t pending = (bufferSize < total_pending) ? bufferSize : total_pending;
		while (pending != 0 && (ret = write (write_fd, message, pending)) != 0) 		/* write in chunks of at most bufferSize bytes */
		{
 			if (ret == -1) 
 			{
 				if (errno == EINTR)							  /* if write was interrupted by a signal continue */
 					continue;
 				if (errno == EWOULDBLOCK || errno == EAGAIN)  /* if not enough write space available yet just continue */
 					continue;
 				perror("[Error] : write -> send_message\n");	/* else a more serious error occured */
 				exit(EXIT_FAILURE);
 			}

 			pending -= ret;			/* update pending bytes counters */
 			message += ret;			 
 			total_pending -= ret;
		}
	}

	free(tmp_message);
}

void * read_message(int read_fd, int * msgd, int bufferSize)
{
	/* reading the message consists of 2 parts, reading the message descriptor and then reading the body of the message */
	/* first we read the message descriptor of message */
	int * header = msgd;		
	ssize_t ret;
	size_t total_pending = sizeof(*header);		/* total bytes pending to be read */
	size_t total = total_pending;
	while (total_pending != 0)				/* while we have not read all of them */
	{
		size_t pending = (bufferSize < total_pending) ? bufferSize : total_pending;			/* pending bytes will be read in this iteration */
		while (pending != 0 && (ret = read(read_fd, header, pending)) != 0) 		/* read in chunks of at most bufferSize bytes */
		{
 			if (ret == -1) 
 			{
 				if (errno == EINTR && total_pending == total)		/* if read was interrupted by a signal and you have not read anything yet, return NULL (safe interrupt)*/
 					return NULL;
 				if (errno == EINTR && total_pending != total)		/* if read was interrupted by a signal but you have already read part of message ignore signal for now (unsafe interrupt)*/
 					continue;
 				if (errno == EAGAIN || errno == EWOULDBLOCK)		/* if nothing yet to read just continue */
 					continue;
 				perror("[Error] : read -> read_message\n");		/* else a more serious error occured */
 				exit(EXIT_FAILURE);
 			}

 			pending -= ret;			/* update pending bytes counters */
 			header += ret;			 
 			total_pending -= ret;
		}
	}

	/* after reading the header , now we know the message descriptor and thus the message structure and thus the remaining bytes to be read for the body of mesage*/
	//msgd = header;
	size_t body_size;
	
	switch (*msgd)
	{
		case DONE : body_size = 0; break;
		case MSG0 : body_size = MSG0_SIZE; break;
		case MSG1 : body_size = MSG1_SIZE; break;
		case MSG1_NO_REPLY : body_size = MSG1_SIZE; break;
		case MSG2 : body_size = MSG2_SIZE + bloomSize * sizeof(uint8_t); break;
		case MSG3 : body_size = MSG3_SIZE; break;
		case MSG4 : body_size = MSG4_SIZE; break;
		case MSG5 : body_size = MSG5_SIZE; break;
		case MSG6 : body_size = MSG6_SIZE; break;
		case MSG7 : body_size = MSG7_SIZE; break;
		case MSG8 : body_size = MSG8_SIZE; break;
		default : fprintf(stderr, "[Error] : invalid message descriptor\n"); exit(EXIT_FAILURE);
	}


	if(!body_size) return NULL;  // if there is no data in message just return empty data message

	/* now  we are ready to read the message itself (the data) */
	total_pending = body_size;
	void * message = calloc(1, body_size);		// allocate space for the data of message (the message itself)
	if (message == NULL)
	{
		fprintf(stderr, "[Error] : read_message -> calloc returned NULL\n\n");
		exit(EXIT_FAILURE);
	}
	void * message_buf = message;

	while (total_pending != 0)				/* while we have not read all of them bytes */
	{
		size_t pending = (bufferSize < total_pending) ? bufferSize : total_pending;
		while (pending != 0 && (ret = read(read_fd, message_buf, pending)) != 0) 		/* read in chunks of at most bufferSize bytes */
		{
 			if (ret == -1) 
 			{
 				if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)	/* if read was interrupted by a signal or nothing yet to read just continue */
 					continue;
 				perror("[Error] : read -> read_message\n");		/* else a more serious error occured */
 				exit(EXIT_FAILURE);
 			}

 			pending -= ret;			/* update pending bytes counters */
 			message_buf += ret;			 
 			total_pending -= ret;
		}
	}

	return message;
}

void delete_message(void * message)
{
	free(message);
}