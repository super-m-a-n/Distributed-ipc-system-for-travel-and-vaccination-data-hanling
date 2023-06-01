/* file : tm_signals.h */
#pragma once
#include "tm_helper.h"

// initializes signal handlers, masks, setups signals etc.  Returns 0 on success, -1 if an error occured
int tm_setup_signals(void);
// blocks the signals of interest SIGINT, SIGQUIT, SIGCHLD. Returns 0 on success, -1 if an error occured
int tm_block_signals(void);
// unblocks the signals of interest SIGINT, SIGQUIT, SIGCHLD. Returns 0 on success, -1 if an error occured
int tm_unblock_signals(void);
// test the signals of interest to see if they were set, and if so, calls necessary travelMonitor functions
int tm_test_signals(struct travelMonitor * tm, const char * input_dir_name);