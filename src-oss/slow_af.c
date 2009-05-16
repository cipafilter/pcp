#include <pcp/pmapi.h>
#include <pcp/impl.h>

static struct timeval	start;

#ifdef IS_MINGW
void pause(void) { SleepEx(INFINITE, TRUE); }
#endif

static void
printstamp(struct timeval *tp)
{
    static struct tm    *tmp;
    time_t		tt =  (time_t)tp->tv_sec;

    tmp = localtime(&tt);
    fprintf(stderr, "%02d:%02d:%02d.%03d", tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)(tp->tv_usec/1000));
}

void
onevent(int afid, void *data)
{
    struct timeval	now;
    static int		delay = -3;
    int			evnum;
    double		elapsed;
    struct timeval	sec = { 0, 0 };

    gettimeofday(&now, NULL);

    if (pmDebug & DBG_TRACE_AF) {
	fprintf(stderr, "onevent(0x%x, 0x%lx) called: ", afid, (unsigned long)data);
	printstamp(&now);
	fputc('\n', stderr);
    }

    elapsed = now.tv_sec - start.tv_sec + (double)(now.tv_usec - start.tv_usec) / 1000000.0;

    if (afid == 0x8003)
	printf("event 0x%x callback\n", afid);
    else {
	if (afid == 0x8001)
	    evnum = (int)(elapsed / 2.5);
	else
	    evnum = (int)(elapsed / 1.5);
	/* evnum not reliable for small elapsed intervals */
	if (evnum >= 3)
	    printf("event 0x%x callback #%d\n", afid, evnum);
	else
	    printf("event 0x%x callback #?\n", afid);
    }

    if (delay > 6) exit(0);
    if (delay > 0) {
	/*
	 * was sginap(delay * CLK_TCK) ... usleep() for
	 * delay*CLK_TCK*10^6/CLK_TCK microseconds so "delay" sec
	 */
	sec.tv_sec = delay;
	__pmtimevalSleep(sec);
    }
    delay++;

    if (pmDebug & DBG_TRACE_AF) {
	gettimeofday(&now, NULL);
	fprintf(stderr, "onevent done: ");
	printstamp(&now);
	fputc('\n', stderr);
    }

    fflush(stderr);
    fflush(stdout);
}

int
main(int argc, char **argv)
{
    int		c;
    int		sts;
    int		errflag = 0;
    struct timeval	delta = { 2, 500000 };
    struct timeval	now;

    __pmSetProgname(argv[0]);

    while ((c = getopt(argc, argv, "D:?")) != EOF) {
	switch (c) {

	case 'D':	/* debug flag */
	    sts = __pmParseDebug(optarg);
	    if (sts < 0) {
		fprintf(stderr, "%s: unrecognized debug flag specification (%s)\n",
		    pmProgname, optarg);
		errflag++;
	    }
	    else
		pmDebug |= sts;
	    break;

	case '?':
	default:
	    errflag++;
	    break;
	}
    }

    if (errflag) {
	fprintf(stderr,
"Usage: %s options ...\n\
\n\
Options\n\
  -D debug	standard PCP debug flag\n",
		pmProgname);
	exit(1);
    }

    __pmAFblock();
    gettimeofday(&start, NULL);
    __pmAFregister(&delta, NULL, onevent);
    delta.tv_sec = 1;
    __pmAFregister(&delta, NULL, onevent);
    delta.tv_sec = 0;
    delta.tv_usec = 0;
    __pmAFregister(&delta, NULL, onevent);
    __pmAFunblock();

    for ( ; ; ) {
	fflush(stderr);
	pause();
	if (pmDebug & DBG_TRACE_AF) {
	    gettimeofday(&now, NULL);
	    fprintf(stderr, "returned from pause(): ");
	    printstamp(&now);
	    fputc('\n', stderr);
	}
    }
}
