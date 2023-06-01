/* file : Monitor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "input_check.h"
#include "m_helper.h"
#include "m_signals.h"
#include "messages.h"


int main(int argc, char const *argv[])
{
	if (m_setup_signals() < 0)		// setup signals
	{
		fprintf(stderr, "[Error] : Monitor -> main -> setup_signals\n\n");
		exit(EXIT_FAILURE);
	}

	if (m_block_signals() < 0)		// block signals while we process commands
	{
		fprintf(stderr, "[Error] : Monitor -> main -> block_signals\n\n");
		exit(EXIT_FAILURE);
	}

	srand((unsigned int)time(NULL));
	const char * fifo_read_path = argv[1]; 		/* argv[1] is fifo path for read (parent writes) */
	const char * fifo_write_path = argv[2];		/* argv[2] is fifo path for write (parent reads) */

	/* initialization phase (part1) */
	int read_fd = open(fifo_read_path, O_RDONLY);		// open write/read pipes
	if (read_fd < 0)
	{
		perror("[Error] : Monitor : main -> open (for read)\n");
		exit(EXIT_FAILURE);
	}		

	int write_fd = open(fifo_write_path, O_RDWR | O_NONBLOCK);
	if (write_fd < 0)
	{
		perror("[Error] : Monitor : main -> open (for write)\n");
		exit(EXIT_FAILURE);
	}

	/* initialization phase (part2) */		
	int bufferSize;	unsigned int bloom_size;	
	if (read_buffer_bloom_size(&bufferSize, &bloom_size, read_fd, write_fd) < 0)	// read bufferSize and bloom_size from travelMonitor
		exit(EXIT_FAILURE);

	/* initialization phase (part3) */
	struct Monitor * monitor = Monitor_init(bufferSize, bloom_size, 8, 0.5); 	// initialize structures kept by Monitor
	monitor->read_fd = read_fd;
	monitor->write_fd = write_fd;

	/* initialization phase (final part) */
	if (read_subdirs(monitor) < 0)
		exit(EXIT_FAILURE);

	if (m_unblock_signals() < 0)		// unblock signals
	{
		fprintf(stderr, "[Error] : Monitor -> main -> unblock_signals\n\n");
		exit(EXIT_FAILURE);
	}

	void * message;
	int msgd;
	char subdir[30] = "";

	while(1)
	{
		if (m_block_signals() < 0)		// block signals now that we are about to process commands
		{
			fprintf(stderr, "[Error] : Monitor -> main -> block_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if (m_test_signals(monitor, subdir) < 0)		// test signals and take necessary actions
		{	
			fprintf(stderr, "[Error] : Monitor -> main -> test_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if (m_unblock_signals() < 0)		// unblock signals 
		{
			fprintf(stderr, "[Error] : Monitor -> main -> unblock_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if ((message = read_message(read_fd, &msgd, bufferSize)) == NULL)		// wait here until you read message or get interrupted by a signal
			continue;	// if message returned was NULL,  that means read was safely interrupted by signal so we handle the signal first and then read the message


		if (m_block_signals() < 0)		// block signals now that we are about to process commands
		{
			fprintf(stderr, "[Error] : Monitor -> main -> block_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if (Monitor_take_action(monitor, msgd, message, subdir) < 0)	// take action depending on the message descriptor and the message just read
			exit(EXIT_FAILURE);

		if (m_unblock_signals() < 0)		// receive any pending signals
		{
			fprintf(stderr, "[Error] : Monitor -> main -> block_signals\n\n");
			exit(EXIT_FAILURE);
		}

		// repeat until you get killed/terminated
	}

	Monitor_del(monitor);
	exit(EXIT_SUCCESS);	

}