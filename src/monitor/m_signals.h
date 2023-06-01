/* file : m_signals.h */
#pragma once
#include "m_helper.h"

// initializes signal handlers, masks, setups signals etc.  Returns 0 on success, -1 if an error occured
int m_setup_signals(void);
// blocks the signals of interest SIGINT, SIGQUIT, SIGUSR1. Returns 0 on success, -1 if an error occured
int m_block_signals(void);
// unblocks the signals of interest SIGINT, SIGQUIT, SIGUSR1. Returns 0 on success, -1 if an error occured
int m_unblock_signals(void);
// test the signals of interest to see if they were set, and if so, calls necessary monitor functions
int m_test_signals(struct Monitor * monitor, char * subdir);