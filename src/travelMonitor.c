/* file : travelMonitor.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include "input_check.h"
#include "tm_helper.h"
#include "tm_signals.h"


int main(int argc, char const *argv[])
{
	if (tm_setup_signals() < 0)		// setup signals
	{
		fprintf(stderr, "[Error] : travelMonitor -> main -> setup_signals\n\n");
		exit(EXIT_FAILURE);
	}

	if (tm_block_signals() < 0)		// block signals while we process commands
	{
		fprintf(stderr, "[Error] : travelMonitor -> main -> block_signals\n\n");
		exit(EXIT_FAILURE);
	}

	int numMonitors, bufferSize;
	unsigned int bloom_size;
	DIR * input_dir;
	
	/* check for correct arg input from terminal and initialize program parameters */
	if (!check_init_args(argc, argv, &numMonitors, &bufferSize, &bloom_size, &input_dir))
		exit(EXIT_FAILURE);

	/* initialization phase (part1) */
	struct travelMonitor * travelMonitor = travelMonitor_init(numMonitors, bufferSize, bloom_size, input_dir);	// initialize structures kept by travelMonitor
	ipc_init(travelMonitor);    						// create fifos, fork and exec for children Monitors, open the fifos and make them ready for use

	/* initialization phase (part2) */
	assign_subdirs(travelMonitor, input_dir, argv[8]);		// traverse input_dir and assign each subdir to a monitor process through the pipe
	wait_monitors_bfs(travelMonitor);						// wait on children to return bloom filters etc. via select

	if (tm_unblock_signals() < 0)		// unblock signals
	{
		fprintf(stderr, "[Error] : travelMonitor -> main -> unblock_signals\n\n");
		exit(EXIT_FAILURE);
	}

	/* executing queries/commands phase */
	bool exiting = false;
	while (exiting == false)
	{
		char input[100];
		if (tm_block_signals() < 0)		// block signals now that we are about to process commands
		{
			fprintf(stderr, "[Error] : travelMonitor -> main -> block_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if (tm_test_signals(travelMonitor, argv[8]) < 0)		// test signals and take necessary actions
		{	
			fprintf(stderr, "[Error] : travelMonitor -> main -> test_signals\n\n");
			exit(EXIT_FAILURE);
		}

		printf("Waiting for command/task >>  ");

		if (tm_unblock_signals() < 0)		// unblock signals 
		{
			fprintf(stderr, "[Error] : travelMonitor -> main -> unblock_signals\n\n");
			exit(EXIT_FAILURE);
		}

		if (fgets(input, 100, stdin) == NULL && errno == EINTR)		// fgets was interrupted by a signal, so ignore characters read and handle signal first
		{
			printf("\n");
			continue;
		}

		if (!strcmp(input, "\n"))
			continue;
		input[strlen(input)-1] = '\0';									// remove newline character from line read from command line

		if (tm_block_signals() < 0)		// block signals now that we are about to process commands
		{
			fprintf(stderr, "[Error] : travelMonitor -> main -> block_signals\n\n");
			exit(EXIT_FAILURE);
		}
		// signals here are blocked, so if travelMonitor is processing a user command, any signal arriving during that period will be handled later
		exiting = check_cmd_args(travelMonitor, input, argv[8]);		// check if cmd line input was correct and take necessary actions if so

		if (tm_unblock_signals() < 0)		// unblock signals 
		{
			fprintf(stderr, "[Error] : travelMonitor -> main -> unblock_signals\n\n");
			exit(EXIT_FAILURE);
		}
	}
	/* exiting now */
	exit(EXIT_SUCCESS);
}