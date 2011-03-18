/*
 *      Author: Alexander Pravdin <aledin@evpatoria.com.ua>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <linuxtrack.h>


#define DEFAULT_FG_HOST       "localhost"
#define DEFAULT_FG_PORT       "6543"
#define DEFAULT_LTR_TIMEOUT   "10"
#define DEFAULT_LTR_PROFILE   "Default"


int Terminate =  0;
int Recenter  =  0;
int Socket    = -1;

const char *argp_program_version      = PACKAGE_STRING;
const char *argp_program_bug_address  = PACKAGE_BUGREPORT;


/* ltr_get_camera_update() arguments put together in one structure */
struct ltr_data {
	float heading;
	float pitch;
	float roll;
	float tx;
	float ty;
	float tz;
	unsigned int cnt;
};


/* Program arguments structure */
struct args
{
	char *fg_host;
	char *fg_port;
	char *ltr_profile;
	char *ltr_timeout;

};

static struct args Args = {
	.fg_host      = DEFAULT_FG_HOST,
	.fg_port      = DEFAULT_FG_PORT,
	.ltr_profile  = NULL, // DEFAULT_LTR_PROFILE
	.ltr_timeout  = DEFAULT_LTR_TIMEOUT
};


/* Program options structure */
static struct argp_option Opts[] = {
	{
		"fg-host",
		'h',
		"HOST",
		0,
		"FlightGear host (default: " DEFAULT_FG_HOST ")",
		1
	},
	{
		"fg-port",
		'p',
		"PORT",
		0,
		"FlightGear port (default: " DEFAULT_FG_PORT ")",
		2
	},
	{
		"ltr-profile",
		'f',
		"NAME",
		0,
		"Linux-track profile name (default: " DEFAULT_LTR_PROFILE ")",
		3
	},
	{
		"ltr-timeout",
		't',
		"SECONDS",
		0,
		"Linux-track initialization timeout (default: " DEFAULT_LTR_TIMEOUT ")",
		4
	},
	{ 0, 0, 0, 0, 0, 0 }
};


static error_t parse_opt(int key, char *arg, struct argp_state *st)
{
	struct args *a = st->input;

	switch (key) {
	case 'h':
		a->fg_host = arg;
		break;
	case 'p':
		a->fg_port = arg;
		break;
	case 'f':
		a->ltr_profile = arg;
		break;
	case 't':
		a->ltr_timeout = arg;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}


/* Argp parser */
static struct argp Argp = { Opts, parse_opt, 0, 0, 0, 0, 0 };



static inline int max(int a, int b)
{
	return (a > b) ? a : b;
}


/*
static void catch_sigusr1(int sig)
{
}

static void catch_sigusr2(int sig)
{
}

static void catch_sighup(int sig)
{
	Recenter = 1;
}
*/

static void catch_sigterm(int sig)
{
	Terminate = 1;
}

static void catch_sigint(int sig)
{
	Terminate = 1;
}

static void setup_signals(void)
{
	//signal(SIGUSR1, catch_sigusr1);
	//signal(SIGUSR2, catch_sigusr2);
	//signal(SIGHUP,  catch_sighup);
	signal(SIGTERM, catch_sigterm);
	signal(SIGINT,  catch_sigint);
}


static void setup_socket(void)
{
	int r;
	struct addrinfo hints;
	struct addrinfo *res, *rp;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family    = AF_UNSPEC;
	hints.ai_socktype  = SOCK_DGRAM;
	hints.ai_flags     = 0;
	hints.ai_protocol  = 0;

	r = getaddrinfo(Args.fg_host, Args.fg_port, &hints, &res);
	if (r != 0)
		error(1, 0, "Bad host address: %s", gai_strerror(r));

	for (rp = res; rp != NULL; rp = rp->ai_next) {

		Socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (Socket == -1)
			continue;

		r = connect(Socket, rp->ai_addr, rp->ai_addrlen);
		if (r != -1)
			break;

		close(Socket);
	}

	if (rp == NULL)
		error(1, 0, "Socket setup failed");

	freeaddrinfo(res);
}


static void at_exit(void)
{
	fprintf(stderr, "Exiting...\n");

	if (Socket >= 0)
		close(Socket);

	ltr_shutdown();

	fprintf(stderr, "Bye.\n");
}


static void init(void)
{
	if (ltr_init(Args.ltr_profile) != 0)
		error(1, 0, "Linux-track initialization failed");

	int timeout = atoi(Args.ltr_timeout);

	ltr_state_type st;

	while (timeout > 0) {
		st = ltr_get_tracking_state();
		if ((st == DOWN) || (st == STOPPED))
			sleep(1);
		else
			break;
		timeout--;
	}

	if (ltr_get_tracking_state() != RUNNING)
		error(1, 0, "Linux-track initialization timeout");

	if (atexit(at_exit) != 0)
		error(1, 0, "Exit function setup failed");

	setup_signals();

	setup_socket();
}


/**
 * fg_send() - Send data to FlightGear
 * @d:        Data to send.
 **/
static void fg_send(const struct ltr_data *d)
{
	int r;

	char buf[128];

	r = snprintf(buf, sizeof(buf),
			"%f\t%f\t%f\t%f\t%f\t%f\n",
			d->heading,
			d->pitch,
			d->roll,
			d->tx,
			d->ty,
			d->tz);

	r = send(Socket, (void *) buf, r, 0);

	if (r == -1 && errno == EINTR)
		return;

	if (r == -1 && errno == ECONNREFUSED)
		return;

	if (r == -1)
		error(1, errno, "send()");
}


static void run_loop(void)
{
	int r;
	fd_set wfds;
	struct timeval tv;
	int nfds;

	struct ltr_data d;

	while (!Terminate) {

		usleep(10000);

		r = ltr_get_camera_update(
				&d.heading,
				&d.pitch,
				&d.roll,
				&d.tx,
				&d.ty,
				&d.tz,
				&d.cnt);

		if (r != 0)
			continue;

		FD_ZERO(&wfds);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		nfds = 0;

		FD_SET(Socket, &wfds);
		nfds = max(nfds, Socket);

		r = select(nfds + 1, NULL, &wfds, NULL, &tv);

		if (r == -1 && errno == EINTR)
			continue;

		if (r == -1)
			error(1, errno, "select()");

		if (FD_ISSET(Socket, &wfds))
			fg_send(&d);
	}

	fprintf(stderr, "Got termination signal\n");
}


int main(int argc, char **argv)
{
	argp_parse(&Argp, argc, argv, 0, 0, &Args);

	fprintf(stderr, "Will send  data  to:  %s:%s\n"
			"Linux-track profile:  %s\n",
			Args.fg_host, Args.fg_port,
			((Args.ltr_profile)
					? Args.ltr_profile
					: DEFAULT_LTR_PROFILE));

	init();

	fprintf(stderr, "Started successfuly\n");

	run_loop();

	return EXIT_SUCCESS;
}
