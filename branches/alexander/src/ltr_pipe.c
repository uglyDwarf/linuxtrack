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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <libgen.h>
#include <linuxtrack.h>

#ifdef LINUX
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#endif


#define DEFAULT_DST_HOST      "127.0.0.1"
#define DEFAULT_DST_PORT      "6543"
#define DEFAULT_LTR_TIMEOUT   "10"
#define DEFAULT_LTR_PROFILE   "Default"


/* Global flags */
static int Recenter  =  0;
static int Terminate =  0;


/* File descriptor */
static int PipeFD    = -1;


static char *Program_name;


/* Run states: RUNNING -> SUSPEND -> STOPPED -> WAKEUP -> RUNNING */
enum run_states {
	RST_RUNNING = 0,
	RST_SUSPEND = 1,
	RST_STOPPED = 2,
	RST_WAKEUP  = 3
};

enum run_states Run_state = RST_RUNNING;


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


/* Output destinations */
enum outputs {
	OUTPUT_NONE     = 0,
	OUTPUT_STDOUT   = 1,
	OUTPUT_FILE     = 2,
	OUTPUT_NET_UDP  = 3,
	OUTPUT_NET_TCP  = 4,
};


/* Data output formats */
enum formats {
	FORMAT_DEFAULT      = 0, // Default
	FORMAT_FLIGHTGEAR   = 1, // For FlightGear with linuxtrack.xml
	FORMAT_IL2          = 2, // For IL-2 Shturmovik with DeviceLink protocol
	FORMAT_HEADTRACK    = 3, // EasyHeadTrack compatible format
	FORMAT_SILENTWINGS  = 4, // Silent Wings format
	FORMAT_MOUSE        = 5, // ImPS/2 Mouse format
#ifdef LINUX
	FORMAT_UINPUT_REL   = 6, // uinput relative position (like a mouse)
	FORMAT_UINPUT_ABS   = 7, // uinput absolute position (like a joystick)
#endif
};


/* Program arguments structure */
struct args
{
	enum outputs  output;
	char          *file;
	char          *dst_host;
	char          *dst_port;
	char          *ltr_profile;
	char          *ltr_timeout;
	enum formats  format;
};

static struct args Args = {
	.output       = OUTPUT_NONE,
	.file         = NULL,
	.dst_host     = DEFAULT_DST_HOST,
	.dst_port     = DEFAULT_DST_PORT,
	.ltr_profile  = NULL, // DEFAULT_LTR_PROFILE
	.ltr_timeout  = DEFAULT_LTR_TIMEOUT,
	.format       = FORMAT_DEFAULT,
};


enum option_codes {
	/* Don't assign code 0x3f (symbol '?') */
	OPT_HELP                   = 'h',  // 0x68
	OPT_VERSION                = 'V',  // 0x56
	OPT_OUTPUT_STDOUT          = 0x01,
	OPT_OUTPUT_FILE            = 0x02,
	OPT_OUTPUT_NET_UDP         = 0x03,
	OPT_OUTPUT_NET_TCP         = 0x04,
	OPT_DST_HOST               = 0x05,
	OPT_DST_PORT               = 0x06,
	OPT_LTR_PROFILE            = 0x07,
	OPT_LTR_TIMEOUT            = 0x08,
	OPT_FORMAT_DEFAULT         = 0x09,
	OPT_FORMAT_FLIGHTGEAR      = 0x0a,
	OPT_FORMAT_IL2             = 0x0b,
	OPT_FORMAT_HEADTRACK       = 0x0c,
	OPT_FORMAT_SILENTWINGS     = 0x0d,
	OPT_FORMAT_MOUSE           = 0x0e,
#ifdef LINUX
	OPT_FORMAT_UINPUT_REL      = 0x0f,
	OPT_FORMAT_UINPUT_ABS      = 0x10,
#endif
};


/* Program options structure */
static struct option Opts[] = {
	{
		"help",
		no_argument,
		0,
		OPT_HELP
	},
	{
		"version",
		no_argument,
		0,
		OPT_VERSION
	},
	{
		"output-stdout",
		no_argument,
		0,
		OPT_OUTPUT_STDOUT
	},
	{
		"output-file",
		required_argument,
		0,
		OPT_OUTPUT_FILE
	},
	{
		"output-net-udp",
		no_argument,
		0,
		OPT_OUTPUT_NET_UDP
	},
	{
		"output-net-tcp",
		no_argument,
		0,
		OPT_OUTPUT_NET_TCP
	},
	{
		"dst-host",
		required_argument,
		0,
		OPT_DST_HOST
	},
	{
		"dst-port",
		required_argument,
		0,
		OPT_DST_PORT
	},
	{
		"ltr-profile",
		required_argument,
		0,
		OPT_LTR_PROFILE
	},
	{
		"ltr-timeout",
		required_argument,
		0,
		OPT_LTR_TIMEOUT
	},
	{
		"format-default",
		no_argument,
		0,
		OPT_FORMAT_DEFAULT
	},
	{
		"format-flightgear",
		no_argument,
		0,
		OPT_FORMAT_FLIGHTGEAR
	},
	{
		"format-il2",
		no_argument,
		0,
		OPT_FORMAT_IL2
	},
	{
		"format-headtrack",
		no_argument,
		0,
		OPT_FORMAT_HEADTRACK
	},
	{
		"format-silentwings",
		no_argument,
		0,
		OPT_FORMAT_SILENTWINGS
	},
	{
		"format-mouse",
		no_argument,
		0,
		OPT_FORMAT_MOUSE
	},
#ifdef LINUX
	{
		"format-uinput-rel",
		no_argument,
		0,
		OPT_FORMAT_UINPUT_REL
	},
	{
		"format-uinput-abs",
		no_argument,
		0,
		OPT_FORMAT_UINPUT_ABS
	},
#endif
	{ 0, 0, 0, 0 }
};

static const char *Opts_str = "hV";


static void xerror(int, int, const char *, ...);
static void xwrite(const void *, size_t);


static void help(void)
{
	fprintf(stderr,

"Usage: %s [OPTION...]\n"
"\n"
"Generic options:\n"
"\n"
"  -h, --help                 Give this help list\n"
"  -V, --version              Print program version\n"
"\n"
"Output options:\n"
"\n"
"  --output-stdout            Write data to STDOUT\n"
"  --output-file=FILENAME     Write data to a file\n"
"  --output-net-udp           Write data to network using UDP protocol\n"
"  --output-net-tcp           Write data to network using TCP protocol\n"
"\n"
"Network options:\n"
"\n"
"  --dst-host=HOST            Destination host (default: %s)\n"
"  --dst-port=PORT            Destination port (default: %s)\n"
"\n"
"LinuxTrack options:\n"
"\n"
"  --ltr-profile=PROFILE      LinuxTrack profile name (default: %s)\n"
"  --ltr-timeout=SECONDS      LinuxTrack init timeout (default: %s)\n"
"\n"
"Output data format options:\n"
"\n"
"  --format-default           Write all LinuxTrack values\n"
"  --format-flightgear        FlightGear format\n"
"  --format-il2               IL-2 Shturmovik DeviceLink format\n"
"  --format-headtrack         EasyHeadTrack compatible format\n"
"  --format-silentwings       Silent Wings remote control format\n"
"  --format-mouse             ImPS/2 mouse format\n"
#ifdef LINUX
"  --format-uinput-rel        uinput relative position (like a mouse)\n"
"  --format-uinput-abs        uinput absolute position (like a joystick)\n"
#endif
"\n",

	Program_name,
	DEFAULT_DST_HOST,
	DEFAULT_DST_PORT,
	DEFAULT_LTR_PROFILE,
	DEFAULT_LTR_TIMEOUT);
}


static void version(void)
{
	fprintf(stderr, "%s\n", PACKAGE_STRING);
}


static void parse_opts(int argc, char **argv)
{
	int key;
	int idx = 0;

	while ((key = getopt_long(argc, argv, Opts_str, Opts, &idx)) != -1) {
		switch (key) {
		case OPT_HELP:
			help();
			exit(EXIT_SUCCESS);
			break;
		case OPT_VERSION:
			version();
			exit(EXIT_SUCCESS);
			break;
		case OPT_OUTPUT_STDOUT:
			Args.output = OUTPUT_STDOUT;
			break;
		case OPT_OUTPUT_FILE:
			Args.output = OUTPUT_FILE;
			Args.file = optarg;
			break;
		case OPT_OUTPUT_NET_UDP:
			Args.output = OUTPUT_NET_UDP;
			break;
		case OPT_OUTPUT_NET_TCP:
			Args.output = OUTPUT_NET_TCP;
			break;
		case OPT_DST_HOST:
			Args.dst_host = optarg;
			break;
		case OPT_DST_PORT:
			Args.dst_port = optarg;
			break;
		case OPT_LTR_PROFILE:
			Args.ltr_profile = optarg;
			break;
		case OPT_LTR_TIMEOUT:
			Args.ltr_timeout = optarg;
			break;
		case OPT_FORMAT_DEFAULT:
			Args.format = FORMAT_DEFAULT;
			break;
		case OPT_FORMAT_FLIGHTGEAR:
			Args.format = FORMAT_FLIGHTGEAR;
			break;
		case OPT_FORMAT_IL2:
			Args.format = FORMAT_IL2;
			break;
		case OPT_FORMAT_HEADTRACK:
			Args.format = FORMAT_HEADTRACK;
			break;
		case OPT_FORMAT_SILENTWINGS:
			Args.format = FORMAT_SILENTWINGS;
			break;
		case OPT_FORMAT_MOUSE:
			Args.format = FORMAT_MOUSE;
			break;
#ifdef LINUX
		case OPT_FORMAT_UINPUT_REL:
			Args.format = FORMAT_UINPUT_REL;
			break;
		case OPT_FORMAT_UINPUT_ABS:
			Args.format = FORMAT_UINPUT_ABS;
			break;
#endif
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}
}


static void check_opts(void)
{
	if (Args.output == OUTPUT_NONE) {
		fprintf(stderr, "Output channel unspecified, using STDOUT\n");
		Args.output = OUTPUT_STDOUT;
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
		fprintf(stderr, ": %s", strerror(errnum));

	fprintf(stderr, "\n");

	if (status)
		exit(status);
}


static inline int max(int a, int b)
{
	return (a > b) ? a : b;
}


static void catch_sigusr1(int sig)
{
	(void) sig;

	switch (Run_state) {
	case RST_RUNNING:
		Run_state = RST_SUSPEND;
		break;
	case RST_STOPPED:
		Run_state = RST_WAKEUP;
		break;
	default:
		break;
	}
}

/*
static void catch_sigusr2(int sig)
{
}
*/

static void catch_sighup(int sig)
{
	(void) sig;

	Recenter = 1;
}

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

static void catch_sigpipe(int sig)
{
	(void) sig;

	Terminate = 1;
}

static void setup_signals(void)
{
	signal(SIGUSR1, catch_sigusr1);
	//signal(SIGUSR2, catch_sigusr2);
	signal(SIGHUP,  catch_sighup);
	signal(SIGTERM, catch_sigterm);
	signal(SIGINT,  catch_sigint);
	signal(SIGPIPE, catch_sigpipe);
}


/**
 * setup_ltr() - Prepare LinuxTrack library.
 **/
static void setup_ltr(void)
{
	if (ltr_init(Args.ltr_profile) != 0)
		xerror(1, 0, "LinuxTrack initialization failed");

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
		xerror(1, 0, "LinuxTrack initialization timeout");
}


static void prepare_sock(void)
{
	int r;
	struct addrinfo hints;
	struct addrinfo *res, *rp;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family    = AF_UNSPEC;
	hints.ai_flags     = 0;

	switch (Args.output) {
	case OUTPUT_NET_UDP:
		hints.ai_socktype  = SOCK_DGRAM;
		hints.ai_protocol  = IPPROTO_UDP;
		break;
	case OUTPUT_NET_TCP:
		hints.ai_socktype  = SOCK_STREAM;
		hints.ai_protocol  = IPPROTO_TCP;
		break;
	default:
		break;
	}

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
		xerror(1, errno, "Socket setup failed");

	freeaddrinfo(res);
}


#ifdef LINUX
static void xioctl(int request, ...)
{
	va_list params;
	void    *arg;

	va_start(params, request);
	arg = va_arg(params, void *);
	va_end(params);

	int r = ioctl(PipeFD, request, arg);

	if (r == -1)
		xerror(1, errno, "ioctl()");
}
#endif /* LINUX */


static void prepare_file(void)
{
	const int o_opts = O_CREAT | O_APPEND | O_WRONLY | O_NONBLOCK;
	const int s_opts = S_IRUSR | S_IWUSR  | S_IRGRP  | S_IROTH;

	PipeFD = open(Args.file, o_opts, s_opts);
	if (PipeFD == -1)
		xerror(1, errno, "Failed to open file %s", Args.file);

#ifdef LINUX

	if (Args.format != FORMAT_UINPUT_REL)
		if (Args.format != FORMAT_UINPUT_ABS)
			return;

	struct uinput_user_dev ud;

	memset(&ud, 0, sizeof(ud));

	ud.id.bustype = BUS_VIRTUAL;
	ud.id.vendor  = 0;
	ud.id.version = 1;

	switch (Args.format) {
	case FORMAT_UINPUT_REL:
		snprintf(ud.name, UINPUT_MAX_NAME_SIZE,
				"LinuxTrack uinput-rel");
		ud.id.product = 1;
		xioctl(UI_SET_EVBIT, EV_REL);
		xioctl(UI_SET_RELBIT, REL_X);
		xioctl(UI_SET_RELBIT, REL_Y);
		//xioctl(UI_SET_RELBIT, REL_RX);
		//xioctl(UI_SET_RELBIT, REL_RY);
		//xioctl(UI_SET_RELBIT, REL_RZ);
		break;
	case FORMAT_UINPUT_ABS:
		snprintf(ud.name, UINPUT_MAX_NAME_SIZE,
				"LinuxTrack uinput-abs");
		ud.id.product = 2;
		xioctl(UI_SET_EVBIT, EV_ABS);
		xioctl(UI_SET_ABSBIT, ABS_X);
		xioctl(UI_SET_ABSBIT, ABS_Y);
		xioctl(UI_SET_ABSBIT, ABS_Z);
		xioctl(UI_SET_ABSBIT, ABS_RX);
		xioctl(UI_SET_ABSBIT, ABS_RY);
		xioctl(UI_SET_ABSBIT, ABS_RZ);

		ud.absmin[ABS_X] = ud.absmin[ABS_RX] = -180;
		ud.absmin[ABS_Y] = ud.absmin[ABS_RY] = -180;
		ud.absmin[ABS_Z] = ud.absmin[ABS_RZ] = -180;

		ud.absmax[ABS_X] = ud.absmax[ABS_RX] =  180;
		ud.absmax[ABS_Y] = ud.absmax[ABS_RY] =  180;
		ud.absmax[ABS_Z] = ud.absmax[ABS_RZ] =  180;
		break;
	default:
		break;
	}

	xwrite(&ud, sizeof(ud));
	xioctl(UI_DEV_CREATE);

#endif /* LINUX */
}


/**
 * setup_fd() - Set up file descriptor for data traversal
 **/
static void setup_fd(void)
{
	switch (Args.output) {
	case OUTPUT_STDOUT:
		PipeFD = STDOUT_FILENO;
		break;
	case OUTPUT_FILE:
		prepare_file();
		break;
	case OUTPUT_NET_UDP:
	case OUTPUT_NET_TCP:
		prepare_sock();
		break;
	default:
		break;
	}
}


static void at_exit(void)
{
	fprintf(stderr, "Exiting...\n");

	if (PipeFD >= 0) {

#ifdef LINUX
		if (Args.format == FORMAT_UINPUT_REL ||
		    Args.format == FORMAT_UINPUT_ABS)
			ioctl(PipeFD, UI_DEV_DESTROY);
#endif

		close(PipeFD);
	}

	ltr_shutdown();

	fprintf(stderr, "Bye.\n");
}


static void init(void)
{
	setup_ltr();

	if (atexit(at_exit) != 0)
		xerror(1, 0, "Exit function setup failed");

	setup_signals();

	setup_fd();

	fprintf(stderr, "Started successfuly\n");
}


/**
 * xwrite() - Write data to PipeFD
 * @buf:    Data buffer
 * @bsz:    Data size
 **/
static void xwrite(const void *buf, size_t bsz)
{
	int r = write(PipeFD, buf, bsz);

	if (r == -1 && errno == EINTR)
		return;

	if (r == -1 && errno == ECONNREFUSED)
		return;

	if (r == -1)
		xerror(1, errno, "write()");
}


/**
 * write_data_default() - Write data with all LinuxTrack values
 * @d:                  Data to write.
 **/
static void write_data_default(const struct ltr_data *d)
{
	char buf[256];

	int r = snprintf(buf, sizeof(buf),
			"heading(%f) "
			"pitch(%f) "
			"roll(%f) "
			"tx(%f) "
			"ty(%f) "
			"tz(%f) "
			"counter(%u)\n",
			d->heading,
			d->pitch,
			d->roll,
			d->tx,
			d->ty,
			d->tz,
			d->cnt);

	xwrite(buf, r);
}


/**
 * write_data_flightgear() - Write data in FlightGear format
 * @d:                     Data to write.
 **/
static void write_data_flightgear(const struct ltr_data *d)
{
	char buf[128];

	int r = snprintf(buf, sizeof(buf),
			"%f\t%f\t%f\t%f\t%f\t%f\n",
			d->heading,
			d->pitch,
			d->roll,
			d->tx,
			d->ty,
			d->tz);

	xwrite(buf, r);
}


/**
 * write_data_il2() - Write data in IL-2 Shturmovik DeviceLink format
 * @d:              Data to write.
 **/
static void write_data_il2(const struct ltr_data *d)
{
	int r;

	char buf[64];

	r = snprintf(buf, sizeof(buf),
			"R/11\\%f\\%f\\%f",
			d->heading * -1,
			d->pitch   * -1,
			d->roll);

	xwrite(buf, r);
}


/**
 * write_data_headtrack() - Write data in EasyHeadTrack format
 * @d:                    Data to write.
 **/
static void write_data_headtrack(const struct ltr_data *d)
{
	const size_t bsz = 1 + 6 * sizeof(uint32_t);
	int8_t buf[bsz];
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

	xwrite(buf, bsz);
}


/**
 * write_data_silentwings() - Write data in Silent Wings format
 * @d:                      Data to write.
 **/
static void write_data_silentwings(const struct ltr_data *d)
{
	char buf[64];

	int r = snprintf(buf, sizeof(buf),
			"PANH %f\nPANV %f\n",
			d->heading,
			d->pitch);

	xwrite(buf, r);
}


/**
 * write_data_mouse() - Write data in ImPS/2 mouse format
 * @d:                Data to write.
 **/
static void write_data_mouse(const struct ltr_data *d)
{
        int8_t x  = (int8_t) d->heading;
        int8_t y  = (int8_t) d->pitch;
        int8_t zx = 0;
        int8_t zy = 0;

        int8_t bttns = 0;

        int8_t buf[4];

        buf[0]  = 0x08;
        buf[0] |= (bttns & 0x07);
        buf[0] |= ((x < 0) ? 0x10 : 0);
        buf[0] |= ((y < 0) ? 0x20 : 0);

        buf[1] = x;
        buf[2] = y;
        buf[3] = 0x00;

        if (zx)
                buf[3] |= ((zx < 0) ? 2 : -2);
        if (zy)
                buf[3] |= ((zy < 0) ? 1 : -1);

        xwrite(buf, sizeof(buf));
}


#ifdef LINUX
static inline void set_input_event_code(struct input_event *e,
		uint16_t code_rel, uint16_t code_abs)
{
	switch (Args.format) {
	case FORMAT_UINPUT_REL:
		e->type = EV_REL;
		e->code = code_rel;
		break;
	case FORMAT_UINPUT_ABS:
		e->type = EV_ABS;
		e->code = code_abs;
		break;
	default:
		/* Shouldn't happen */
		xerror(1, 0, "Program error");
		break;
	}
}
#endif


/**
 * write_data_uinput() - Write data in uinput format
 * @d:                 Data to write.
 **/
#ifdef LINUX
static void write_data_uinput(const struct ltr_data *d)
{
	struct input_event ie;

	memset(&ie, 0, sizeof(ie));
	gettimeofday(&ie.time, NULL);

	/* heading */
	set_input_event_code(&ie, REL_X, ABS_X);
	ie.value = (int32_t) d->heading;
	xwrite(&ie, sizeof(ie));

	/* pitch */
	set_input_event_code(&ie, REL_Y, ABS_Y);
	ie.value = (int32_t) d->pitch;
	xwrite(&ie, sizeof(ie));

	/* roll */
	set_input_event_code(&ie, REL_Z, ABS_Z);
	ie.value = (int32_t) d->roll;
	xwrite(&ie, sizeof(ie));

	if (Args.format == FORMAT_UINPUT_ABS) {
		/* tx */
		set_input_event_code(&ie, REL_RX, ABS_RX);
		ie.value = (int32_t) d->tx;
		xwrite(&ie, sizeof(ie));

		/* ty */
		set_input_event_code(&ie, REL_RY, ABS_RY);
		ie.value = (int32_t) d->tx;
		xwrite(&ie, sizeof(ie));

		/* tz */
		set_input_event_code(&ie, REL_RZ, ABS_RZ);
		ie.value = (int32_t) d->tx;
		xwrite(&ie, sizeof(ie));
	}

	/* sync */
	ie.type = EV_SYN;
	ie.code = SYN_REPORT;
	ie.value = 0;
	xwrite(&ie, sizeof(ie));
}
#endif /* LINUX */


static void write_data(const struct ltr_data *d)
{
	switch (Args.format) {
	case FORMAT_FLIGHTGEAR:
		write_data_flightgear(d);
		break;
	case FORMAT_IL2:
		write_data_il2(d);
		break;
	case FORMAT_HEADTRACK:
		write_data_headtrack(d);
		break;
	case FORMAT_SILENTWINGS:
		write_data_silentwings(d);
		break;
	case FORMAT_MOUSE:
		write_data_mouse(d);
		break;

#ifdef LINUX
	case FORMAT_UINPUT_REL:
	case FORMAT_UINPUT_ABS:
		write_data_uinput(d);
		break;
#endif

	case FORMAT_DEFAULT:
	default:
		write_data_default(d);
		break;
	}
}


static int sleepping(void)
{
	switch (Run_state) {
	case RST_RUNNING:
		return 0;
	case RST_SUSPEND:
		fprintf(stderr, "Suspend\n");
		ltr_suspend();
		Run_state = RST_STOPPED;
		return 1;
	case RST_STOPPED:
		return 1;
	case RST_WAKEUP:
		fprintf(stderr, "Wake-up\n");
		ltr_wakeup();
		Run_state = RST_RUNNING;
		return 0;
	default:
		break;
	}

	return 0;
}


static void run_loop(void)
{
	int r;
	fd_set wfds;
	struct timeval tv;
	int nfds;

	struct ltr_data d;

	unsigned int cnt = 0;

	while (!Terminate) {

		usleep(10000);

		if (sleepping()) {
			usleep(100000);
			continue;
		}

		if (Recenter) {
			Recenter = 0;
			fprintf(stderr, "Recenter\n");
			ltr_recenter();
		}

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

		if (d.cnt == cnt)
			continue;
		cnt = d.cnt;

#ifdef LINUX
		/* Don't use select() for uinput writing */
		if (Args.output == OUTPUT_FILE) {
			if (Args.format == FORMAT_UINPUT_REL ||
			    Args.format == FORMAT_UINPUT_ABS) {
				write_data_uinput(&d);
			}
		}
#endif

		FD_ZERO(&wfds);
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		nfds = 0;

		FD_SET(PipeFD, &wfds);
		nfds = max(nfds, PipeFD);

		r = select(nfds + 1, NULL, &wfds, NULL, &tv);

		if (r == -1 && errno == EINTR)
			continue;

		if (r == -1)
			xerror(1, errno, "select()");

		if (FD_ISSET(PipeFD, &wfds))
			write_data(&d);
	}

	fprintf(stderr, "Got termination signal\n");
}


int main(int argc, char **argv)
{
	Program_name = basename(argv[0]);

	parse_opts(argc, argv);

	check_opts();

	init();

	run_loop();

	return EXIT_SUCCESS;
}
