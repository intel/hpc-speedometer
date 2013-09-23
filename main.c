/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <err.h>
#include <sys/wait.h>
#include <signal.h>
#include <getopt.h>

#include "metrics.h"

struct gbl gbl;

#define VERSION "1.0 " __DATE__



/*          
 * Wallclock time in microseconds
 */

static uint64_t
usec(void) 
{           
    struct timeval tv;
    uint64_t t, s; 
        
    gettimeofday(&tv, 0);
    t = tv.tv_sec;
    t *= 1000000;
    s = t;  
    s += tv.tv_usec;
    return s;
}       


/*
 * Return scale factor to convert RDTSC ticks to seconds.
 */
static void
calibrate()
{
    uint64_t startus, endus;
    uint64_t start, end;
    uint64_t scale;
    int interval = 1000;

    // advance at least one clock tick
    endus = usec();
    for (startus = usec(); startus == endus; startus = usec())
        ;

    // use increasing advance times until we get a reasonable relative error
    start = _rdtsc();
    for (endus = usec(); endus < startus + interval; endus = usec())
        ;
    end = _rdtsc();
    scale = ((end - start) * 1000000) / (endus - startus);
    startus = endus;
    //printf("scale = %lld\n", scale);
    while (interval < 500000)
    {
        uint64_t newscale;
        uint64_t resolution;
        uint64_t diff;

        // increase the interval
        interval *= 2;

        // advance at least one clock tick
        endus = usec();
        for (startus = usec(); startus == endus; startus = usec())
            ;

        start = _rdtsc();
        for (endus = usec(); endus < startus + interval; endus = usec())
            ;
        end = _rdtsc();

        newscale = ((end - start) * 1000000) / (endus - startus);
        if (newscale < scale)
            diff = scale - newscale;
        else
            diff = newscale - scale;
        // Try for microsecond resolution
        resolution = ((1000000 * diff) / newscale);
        //printf("resolution = %lld\n", resolution);
        if (resolution == 0)
            break;
        scale = newscale;
    }

    gbl.hz = scale;
}

static char *
findpath(const char *s)
{
    char *path = strdup(getenv("PATH"));
    char *q;
    if (strchr(s, '/'))
    {
        q = strdup(s);
        goto ret;
    }
    for (char *p = strtok(path, ":"); p != NULL; p = strtok(NULL, ":"))
    {
        struct stat buf;
        q = malloc(strlen(p) + strlen(s) + 2);
        if (q == 0)
            err(1, NULL);
        strcpy(q, p);
        strcat(q, "/");
        strcat(q, s);
        if (!stat(q, &buf))
            if (S_ISREG(buf.st_mode))
                goto ret;
        free(q);
    }
    q = 0;
ret:
    free(path);
    return q;
}

static char *myname;
#define NOARG 0
#define HASARG 1
#define OPTARG 2
struct option long_options[] = {
    {"server", NOARG, 0, 's'},
    {"out", HASARG, 0, 'o'},
    {"port", HASARG, 0, 'p'},
    {"help", NOARG, 0, 'h'},
    {"verbose", NOARG, 0, 'v'},
    {0}
};

#define OPTSTR "+so:p:hv"

#define HELPMSG \
"-s,--server          Go into server mode\n" \
"-o,--out file        Output file for full sample listing\n" \
"-p,--port port       Port for server mode\n" \
"-h,--help            This message\n" \
"-v,--verbose         Version, etc\n"


static void
usage()
{
    fprintf(stderr, "Usage: %s [--server] prog [args]\n", myname);
    fprintf(stderr, HELPMSG);
    exit(1);
}

static int
process_options(int argc, char **argv)
{
    int c;

    myname = argv[0];
    for (;;)
    {
	int this_option_optind = optind ? optind : 1;
	int option_index = 0;

	c = getopt_long(argc, argv, OPTSTR, long_options, &option_index);
	if (c == -1)
	    break;

	switch (c)
	{
	case 's':
            gbl.server = 1;
            break;

        case 'h':
            usage();
            exit(0);

        case 'o':
            gbl.outfile = optarg;
            break;
        case 'p':
            gbl.port = atoi(optarg);
            break;

        case 'v':
            gbl.verbose = 1;
            printf("%s Version %s\n", argv[0], VERSION);
            printf("Frequency = %g\n", gbl.hz);
            break;


	case '?':
	    usage();
            return -1;

	default:
            usage();
            return -1;
	}
    }
    if (!gbl.server && optind >= argc)
    {
        usage();
        return -1;
    }
    return optind;
}

int
main(int argc, char **argv)
{
    int pid = 0;
    int status;
    int ret;
    int ready[2], go[2];
    char buf;
    char *f;
    int offs;

    calibrate();
    offs = process_options(argc, argv);
    if (offs < 0)
        exit(1);

    if (!gbl.server)
    {
        f = findpath(argv[offs]);
        if (f == NULL)
            errx(1, "File not found on path: %s", argv[offs]);
        /* re-export LD_LIBRARY_PATH */
        char *p = getenv("_TMP_INTEL_LD_LIBRARY_PATH");
        if (p != 0)
        {
            size_t len = strlen(p) + strlen("LD_LIBRARY_PATH") + 1;
            char *q = malloc(len+1);
            if (q == 0)
                err(1, NULL);
            strcpy(q, "LD_LIBRARY_PATH=");
            strcat(q, p);
            putenv(q);
        }

        /* Create child and pipe for synchronization so we can set up events
         * and get child pid and start monitoring before it execs the real code.
         */
        ret = pipe(ready);
        if (ret)
            err(1, "cannot create pipe ready");
        ret = pipe(go);
        if (ret)
            err(1, "cannot create pipe go");
        pid = fork();
        if (pid == -1)
            err(1, "cannot fork process");
        if (pid == 0)
        {
            if (setuid(getuid()) != 0)
                err(1, "Couldn't revert to my uid for child\n");

            /* Child */
            close(ready[0]);
            close(go[1]);

            close(ready[1]);        // signal the parent
            if (read(go[0], &buf, 1)  == -1)        // wait for ack
                err(1, "unable to read go pipe");
            execv(f, &argv[offs]);
            err(1, "%s", f);
        }
        free(f);
        close(ready[1]);
        close(go[0]);
        if (read(ready[0], &buf, 1) == -1)
            err(1, "unable to read child ready pipe");
        close(ready[0]);
    }

    setup(pid);

    struct timeval forktime, jointime;
    gettimeofday(&forktime, 0);
    if (!gbl.server)
        close(go[1]);       // let the child go

    status = monitor(pid, &jointime);
    printsummary(pid, &forktime, &jointime);
    jointhreads();
    closefiles();
    if (!gbl.server)
        if (WIFEXITED(status))
            exit(WEXITSTATUS(status));
    exit(-1);
}

