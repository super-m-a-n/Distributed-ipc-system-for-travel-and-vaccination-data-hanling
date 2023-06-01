/* file : messages.h */
#pragma once
#include "bloom.h"

/* here we define the structure of possible messages between travelMonitor and Monitor processes */
/* all messages have constant max size in bytes, so that the ipc is more straightforward */

/* each message type has each own unique message descriptor msgd */
#define DONE -1
#define MSG0 0
#define MSG1 1
#define MSG2 2
#define MSG3 3
#define MSG4 4
#define MSG5 5
#define MSG6 6
#define MSG7 7
#define MSG8 8
#define MSG1_NO_REPLY 9		// this type of message is for when the parent forks a new child to replace an old one that terminated unexpectedly
							// in this case the parent does not expect a reply, since he already has the bloom filters saved
							// its structure is essentially identical to that of MSG1, we just use a different message descriptor because the response changes

/* message descriptors will always be the first bytes sent to indicate the type of message to expect */

/* messages */

/* initialization phase , travelMonitor sends the bufferSize and the bloom filter size to the Monitor process */
/* msg0 structure : <int bufferSize> <unsigned int bloom_size> */
#define MSG0_SIZE sizeof(int) + sizeof(unsigned int)

/* initialization phase , travelMonitor sends one subdirectory for each country to a Monitor process */
/* msg1 structure : <char subdir[30]> */
#define MSG1_SIZE 30

/* initialization phase, a Monitor process sends back a bloom filter for each virus, among all countries it monitors */
/* msg2 structure : <char virus[20]> <uint8_t bit_array[bloom_size]> */
#define MSG2_SIZE 20

/* query 1, travelMonitor needs to know for sure if a specific citizenID has been vaccinated for specific virus, and asks a monitor process */
/* msg3 structure : <char citizenID[6]> <char virus[20]> */
#define MSG3_SIZE 26

/* monitor process replies to travelMonitor regarding query 1 with an answer (YES/NO) and a date of vaccination */
/* msg4 structure : <char answer[4]> <char date[12]> */
#define MSG4_SIZE 16

/* query 4, travelMonitor asks each monitor process to find all they know about a specific citizen with given citizenID */
/* msg5 structure : <char citizenID[6]> */
#define MSG5_SIZE 6

/* monitor process replies to travelMonitor regarding query 4, with a name, surname, age and country for given citizen */
/* msg6 structure : <char name[13]> <char surname[13]> <char country[30]> <int age> */
#define MSG6_SIZE 56 + sizeof(int)

/* monitor process replies to travelMonitor regarding query 4, with a virus and vaccine status and vaccination date */
/* msg7 structure : <char virus[20]> <char status[4]> <char date[12]> */
#define MSG7_SIZE 36

/* travelMonitor tells Monitor process that handles CountryTo if the request got rejected/accepted */
/* msg8 structure : <int result>*/
#define MSG8_SIZE sizeof(int)

/* creates a message of type msg0 */
void * create_msg0(int bufferSize, unsigned int bloom_size);
/* creates a message of type msg1 */
void * create_msg1(const char * input_dir_name, char * subdir_name);
/* creates a message of type msg2 */
void * create_msg2(char * virus_name, unsigned int bloom_size, Bloom bloom_filter);
/* creates a message of type msg3 */
void * create_msg3(char * citizenID, char * virusName);
/* creates a message of type msg4 */
void * create_msg4(char * answer, char * date);
/* creates a message of type msg5 */
void * create_msg5(char * citizenID);
/* creates a message of type msg6 */
void * create_msg6(char * name, char * surname, char * country, int age);
/* creates a message of type msg7 */
void * create_msg7(char * virusName, char * status, char * date);
/* creates a message of type msg8 */
void * create_msg8(int result);

/* sends a message using the given write file descriptor, where msgd is the message descriptor id, and message is just the message */
void send_message(int write_fd, int msgd, void * message, int bufferSize);
/* reads a message using the given read file descriptor, returns the message's message descriptor id in msgd, returns the message */
void * read_message(int read_fd, int * msgd, int bufferSize);
/* deletes message (just frees allocated memory) */
void delete_message(void * message);

/* decodes and returns info of message of type msg0 */
int decode_msg0(int msgd, void * message, int * bufferSize, unsigned int * bloom_size);
/* decodes and returns info of message of type msg1 */
int decode_msg1(int msgd, void * message, char * subdir);
/* decodes and returns info of message of type msg2 */
int decode_msg2(int msgd, void * message, char * virus, void ** bit_array);
/* decodes and returns info of message of type msg3 */
int decode_msg3(int msgd, void * message, char * citizenID, char * virus);
/* decodes and returns info of message of type msg4 */
int decode_msg4(int msgd, void * message, char * answer, char * date);
/* decodes and returns info of message of type msg5 */
int decode_msg5(int msgd, void * message, char * citizenID);
/* decodes and returns info of message of type msg6 */
int decode_msg6(int msgd, void * message, char * name, char * surname, char * country, int * age);
/* decodes and returns info of message of type msg7 */
int decode_msg7(int msgd, void * message, char * virus, char * status, char * date);
/* decodes and returns info of message of type msg8 */
int decode_msg8(int msgd, void * message, int * result);

void bloomSize_init(unsigned int bloom_size);