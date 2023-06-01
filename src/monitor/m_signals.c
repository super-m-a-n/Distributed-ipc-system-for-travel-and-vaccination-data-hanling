/* file : m_signals.c */
/* signal handling for the Monitor child process is developed here */
/* for the code I developed here I used MARC J.ROCHKIND's book as a guide*/
#include <signal.h>
#include <string.h>
#include "m_helper.h"

/* to make global variables usage safe inside signal handler, we declare them as volatile sig_atomic_t */
static volatile sig_atomic_t got_SIGINT;		// a flag for SIGINT, if 1, a SIGINT was received, if 0 a SIGINT was not received
static volatile sig_atomic_t got_SIGQUIT;		// a flag for SIGQUIT, if 1, a SIGQUIT was received, if 0 a SIGQUIT was not received
static volatile sig_atomic_t got_SIGUSR1;		// a flag for SIGUSR1, if 1, a SIGUSR1 was received, if 0 a SIGUSR1 was not received


// a pretty simple signal handler
//if it receives any of the SIGINT, SIGQUIT, SIGUSR1, it just sets the corresponding flag to 1
static void sig_handler(int signum)
{
	if (signum == SIGINT)
		got_SIGINT = 1;
	else if (signum == SIGQUIT)
		got_SIGQUIT = 1;
	else if (signum == SIGUSR1)
		got_SIGUSR1 = 1;
}

static sigset_t blocked_set;		// this will be the set of signals of interest (SIGINT, SIGQUIT, SIGUSR1) that we want to be blocked
									// while commands are being processed by the Monitor


// initializes signal handlers, masks, setups signals etc.  Returns 0 on success, -1 if an error occured
int m_setup_signals(void)
{
	sigset_t set;
	struct sigaction act;

	if (sigfillset(&set) < 0)
		return -1;
	if (sigprocmask(SIG_SETMASK, &set, NULL) < 0)		// block all signals, we do not want to be interrupted here during initialization
		return -1;
	memset(&act, 0, sizeof(act));
	if (sigfillset(&act.sa_mask) < 0)					// block all signals while handling either SIGINT, SIGQUIT or SIGUSR1
		return -1;
	act.sa_handler = sig_handler;						// set the signal handling function
	
	got_SIGINT = 0;
	if (sigaction(SIGINT, &act, NULL) < 0)				// handle SIGINT
		return -1;
	
	got_SIGQUIT = 0;
	if (sigaction(SIGQUIT, &act, NULL) < 0)				// handle SIGQUIT
		return -1;

	got_SIGUSR1 = 0;
	if (sigaction(SIGUSR1, &act, NULL) < 0)				// handle SIGUSR1
		return -1;

	if (sigemptyset(&blocked_set) < 0)					// initialize the blocked set, to include the signals of interest SIGINT, SIGQUIT, SIGUSR1
		return -1;
	if (sigaddset(&blocked_set, SIGINT) < 0)
		return -1;
	if (sigaddset(&blocked_set, SIGQUIT) < 0)
		return -1;
	if (sigaddset(&blocked_set, SIGUSR1) < 0)
		return -1;

	if (sigemptyset(&set) < 0)
		return -1;
	if (sigprocmask(SIG_SETMASK, &set, NULL) < 0)		// now unblock all signals and resume execution
		return -1;

	return 0;
}

// blocks the signals of interest SIGINT, SIGQUIT, SIGUSR1. Returns true on success, false if an error occured
int m_block_signals(void)
{
	if (sigprocmask(SIG_SETMASK, &blocked_set, NULL) < 0)		// sets the mask of blocked signals to be the signals of interest SIGINT, SIGQUIT, SIGUSR1
		return -1;
	return 0;
}

// unblocks the signals of interest SIGINT, SIGQUIT, SIGUSR1. Returns true on success, false if an error occured
int m_unblock_signals(void)
{
	if (sigprocmask(SIG_UNBLOCK, &blocked_set, NULL) < 0)		// removes the signals of interest SIGINT, SIGQUIT, SIGUSR1 from the mask of blocked signals
		return -1;
	return 0;
}

// tests the signals of interest to see if they are set. 
// If they are set, the function calls the corresponding Monitor functions for each case
int m_test_signals(struct Monitor * monitor, char * subdir)
{
	int status = 0;
	if (got_SIGINT || got_SIGQUIT)
		m_log_file_print(monitor);
	if (got_SIGUSR1)
		status = read_subdir_updates(monitor, subdir);

	// reset the signal flags
	got_SIGINT = 0;
	got_SIGQUIT = 0;
	got_SIGUSR1 = 0;

	return status; 

	// NOTE : we assume that the parent ONLY sends a SIGUSR1  to the MONITOR process through /addVaccinationRecords command
}