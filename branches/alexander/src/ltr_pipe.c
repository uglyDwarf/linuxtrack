/*
 *      Author: Alexander Pravdin <aledin@evpatoria.com.ua>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <libgen.h>
#include <linuxtrack.h>


#define DEFAULT_DST_HOST      "127.0.0.1"
#define DEFAULT_DST_PORT      "6543"
#define DEFAULT_LTR_TIMEOUT   "10"
#define DEFAULT_LTR_PROFILE   "Default"


//static int Recenter  =  0;
static int Terminate =  0;
static int PipeFD    = -1;

static char *Program_name;


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


enum outputs {
	OUTPUT_STDOUT = 0,
	OUTPUT_NETUDP = 1
};


enum formats {
	FORMAT_FGFS  = 0,  // Default, for FlightGear with linux-track.xml
	FORMAT_IL2   = 1,  // For IL-2 Shturmovik with DeviceLink protocol
	FORMAT_EHT   = 2   // Easy-Headtrack compatible format
};


/* Program arguments structure */
struct args
{
	enum outputs  output;
	char          *dst_host;
	char          *dst_port;
	char          *ltr_profile;
	char          *ltr_timeout;
	enum formats  format;
};

static struct args Args = {
	.output       = OUTPUT_STDOUT,
	.dst_host     = DEFAULT_DST_HOST,
	.dst_port     = DEFAULT_DST_PORT,
	.ltr_profile  = NULL, // DEFAULT_LTR_PROFILE
	.ltr_timeout  = DEFAULT_LTR_TIMEOUT,
	.format       = FORMAT_FGFS,
};


/* Program options structure */
static struct option Opts[] = {
	{ "help",         no_argument,       0,                  'h'         },
	{ "version",      no_argument,       0,                  'V'         },
	{ "udp",          no_argument,       0,                  'U'         },
	{ "dst-host",     required_argument, 0,                  'd'         },
	{ "dst-port",     required_argument, 0,                  'p'         },
	{ "ltr-profile",  required_argument, 0,                  'f'         },
	{ "ltr-timeout",  required_argument, 0,                  't'         },
	{ "format-il2",   no_argument,       (int*)&Args.format,  FORMAT_IL2 },
	{ "format-eht",   no_argument,       (int*)&Args.format,  FORMAT_EHT },
	{ 0, 0, 0, 0 }
};

static const char *Opts_str = "hVUd:p:f:t:";


static void help(void)
{
	fprintf(stderr,

"Usage: %s [OPTION...]\n"
"\n"
"  -h, --help                 Give this help list\n"
"  -V, --version              Print program version\n"
"  -U, --udp                  Output data to network using UDP proto\n"
"  -d, --dst-host=HOST        Destination host (default: %s)\n"
"  -p, --dst-port=PORT        Destination port (default: %s)\n"
"  -f, --ltr-profile=NAME     Linux-track profile name (default: %s)\n"
"  -t, --ltr-timeout=SECONDS  Linux-track init timeout (default: %s)\n"
"      --format-il2           Output in IL-2 Shturmovik DeviceLink format\n"
"      --format-eht           Output in Easy-Headtrack compatible format\n"
"\n"
"Mandatory or optional arguments to long options are also mandatory or optional\n"
"for any corresponding short options.\n"
"\n"
"Report bugs to %s\n",

	Program_name,
	DEFAULT_DST_HOST,
	DEFAULT_DST_PORT,
	DEFAULT_LTR_PROFILE,
	DEFAULT_LTR_TIMEOUT,
	PACKAGE_BUGREPORT);
}


static void version(void)
{
	fprintf(stderr, "%s\n", PACKAGE_STRING);
}


static void parse_opt(int argc, char **argv)
{
	int key;
	int idx = 0;

	while ((key = getopt_long(argc, argv, Opts_str, Opts, &idx)) != -1) {
		switch (key) {
		case 'h':
			help();
			exit(EXIT_SUCCESS);
			break;
		case 'V':
			version();
			exit(EXIT_SUCCESS);
			break;
		case 'U':
			Args.output = OUTPUT_NETUDP;
			break;
		case 'd':
			Args.dst_host = optarg;
			break;
		case 'p':
			Args.dst_port = optarg;
			break;
		case 'f':
			Args.ltr_profile = optarg;
			break;
		case 't':
			Args.ltr_timeout = optarg;
			break;
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}
}


/**
 * xerror() - Print error message and optionally terminate program
 * @status:  Terminate with this exit code if it is non-zero.
 * @errnum:  Value of errno.
 * @format:  Printf-like format string for message.
 * @...:     Format optional parameters.
 *
 * This is a re-invention of GNU/Linux error() function from <error.h>
 * just because there is no such header and function available on Mac OS.
 **/
static void xerror(int status, int errnum, const char *format, ...)
{
	va_list params;

	fprintf(stderr, "%s: ", Program_name);

	va_start(params, format);
	vfprintf(stderr, format, params);
	va_end(params);

	if (errnum)
		fprintf(stderr, ": %s\n", strerror(errnum));

	if (status)
		exit(status);
}


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
	(void) sig;

	Recenter = 1;
}
*/

static void catch_sigterm(int sig)
{
	(void) sig;

	Terminate = 1;
}

static void catch_sigint(int sig)
{
	(void) sig;

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


/**
 * setup_fd() - Set up file descriptor for data traversal
 **/
static void setup_fd(void)
{
	if (Args.output == OUTPUT_STDOUT) {
		PipeFD = STDOUT_FILENO;
		return;
	}

	int r;
	struct addrinfo hints;
	struct addrinfo *res, *rp;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family    = AF_UNSPEC;
	hints.ai_socktype  = SOCK_DGRAM;
	hints.ai_flags     = 0;
	hints.ai_protocol  = IPPROTO_UDP;

	r = getaddrinfo(Args.dst_host, Args.dst_port, &hints, &res);
	if (r != 0)
		xerror(1, 0, "Bad host address: %s", gai_strerror(r));

	for (rp = res; rp != NULL; rp = rp->ai_next) {

		PipeFD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (PipeFD == -1)
			continue;

		r = connect(PipeFD, rp->ai_addr, rp->ai_addrlen);
		if (r != -1)
			break;

		close(PipeFD);
	}

	if (rp == NULL)
		xerror(1, 0, "Socket setup failed");

	freeaddrinfo(res);
}


static void at_exit(void)
{
	fprintf(stderr, "Exiting...\n");

	if (PipeFD >= 0)
		close(PipeFD);

	ltr_shutdown();

	fprintf(stderr, "Bye.\n");
}


static void init(void)
{
	if (ltr_init(Args.ltr_profile) != 0)
		xerror(1, 0, "Linux-track initialization failed");

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
		xerror(1, 0, "Linux-track initialization timeout");

	if (atexit(at_exit) != 0)
		xerror(1, 0, "Exit function setup failed");

	setup_signals();

	setup_fd();

	fprintf(stderr, "Started successfuly\n");
}


/**
 * pipe_write() - Write data to PideFD
 * @buf:          Data buffer
 * @bsz:          Data size
 **/
static void pipe_write(const char *buf, size_t bsz)
{
	int r = write(PipeFD, (void *) buf, bsz);

	if (r == -1 && errno == EINTR)
		return;

	if (r == -1 && errno == ECONNREFUSED)
		return;

	if (r == -1)
		xerror(1, errno, "write()");
}


/**
 * il2_send() - Send data to IL-2 Shturmovik
 * @d:        Data to send.
 **/
static void il2_send(const struct ltr_data *d)
{
	int r;

	char buf[64];

	r = snprintf(buf, sizeof(buf),
			"R/11\\%f\\%f\\%f",
			d->heading * -1,
			d->pitch   * -1,
			d->roll);

	pipe_write(buf, r);
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

	pipe_write(buf, r);
}


/**
 * eht_send() - Send data to FlightGear using Easy-Headtrack format
 * @d:        Data to send.
 **/
static void eht_send(const struct ltr_data *d)
{
	const size_t bsz = 1 + 6 * sizeof(uint32_t);
	char buf[bsz];
	uint32_t tmp;
	size_t offset = 0;

	buf[offset++] = 1;

	tmp = htonl(*(uint32_t *) &d->heading);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	tmp = htonl(*(uint32_t *) &d->roll);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	tmp = htonl(*(uint32_t *) &d->pitch);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	tmp = htonl(*(uint32_t *) &d->tx);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	tmp = htonl(*(uint32_t *) &d->ty);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	tmp = htonl(*(uint32_t *) &d->tz);
	memcpy(&buf[offset], &tmp, sizeof(uint32_t));

	pipe_write(buf, bsz);
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

		FD_SET(PipeFD, &wfds);
		nfds = max(nfds, PipeFD);

		r = select(nfds + 1, NULL, &wfds, NULL, &tv);

		if (r == -1 && errno == EINTR)
			continue;

		if (r == -1)
			xerror(1, errno, "select()");

		if (FD_ISSET(PipeFD, &wfds)) {
			switch (Args.format) {
			case FORMAT_IL2:
				il2_send(&d);
				break;
			case FORMAT_EHT:
				eht_send(&d);
				break;
			case FORMAT_FGFS:
			default:
				fg_send(&d);
				break;
			}
		}
	}

	fprintf(stderr, "Got termination signal\n");
}


int main(int argc, char **argv)
{
	Program_name = basename(argv[0]);

	parse_opt(argc, argv);

	init();

	run_loop();

	return EXIT_SUCCESS;
}
