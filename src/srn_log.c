/*
 * Copyright (c) 2020 Franck Lesage <effervecreanet@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <netinet/in.h>

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>


#include "srn_date.h"
#include "srn_log.h"


extern char	gmdate[33];
extern int	fdlog_http;
extern int	fdlog_srn;
char	httplogbuf[2048];
char	srnlogbuf[2048];


/*
 * Log filename is the inet addr concatenated with `.log', try to open or
 * create it, if an error occur exit. The directory holding the log files is
 * PATH_LOG.
 */

int
open_http_log(const char *ipstr)
{
	char		logpath[sizeof(PATH_LOG) - 1 + INET_ADDRSTRLEN - 1 + sizeof(".log")];
	char		buffer[1024];

	if (access(PATH_LOG, X_OK | W_OK) < 0)
		if (mkdir(PATH_LOG, 0700) < 0)
			errx(1,
			     "Can't create the log directory with write and execute perms");

	memset(logpath, 0, sizeof(logpath));
	sprintf(logpath, "%shttp_%s.log", PATH_LOG, ipstr);

	fdlog_http = open(logpath, O_WRONLY | O_APPEND | O_CREAT);
	if (fdlog_http < 0)
		errx(1, "Unable to open or create log file %s (%s) exiting ..",
		     logpath,
		     strerror(errno));

	set_gmdate();

	memset(buffer, 0, 1024);
	sprintf(buffer, "\n\nLog file %s opened at %s\n",
		logpath,
		gmdate);

	write(fdlog_http, buffer, strlen(buffer));


	return 1;
}

void init_intro_http_log(char *wwwhostname, char *addrstr) {
	write(fdlog_http, HTTP_STRLOG_STARTING,
                     strlen(HTTP_STRLOG_STARTING));
        write(fdlog_http, wwwhostname,
                     strlen(wwwhostname));
        write(fdlog_http, "\n", 1);

        write(fdlog_http, HTTP_STRLOG_STARTING_ADDR,
                     strlen(HTTP_STRLOG_STARTING_ADDR));
        write(fdlog_http, addrstr, strlen(addrstr));
        write(fdlog_http, "\n", 1);

	return;
}

int open_srn_log(void) {
	char            logpath[sizeof(PATH_LOG) - 1 + INET_ADDRSTRLEN - 1 + sizeof(".log")];
	char		buffer[1024];

	memset(logpath, 0, sizeof(logpath));
	sprintf(logpath, "%ssrn.log", PATH_LOG);


	fdlog_srn = open(logpath, O_WRONLY | O_APPEND | O_CREAT);
	if (fdlog_srn < 0)
		errx(1, "Unable to open or create log file %s (%s) exiting ..",
		     logpath,
		     strerror(errno));

	set_gmdate();
	memset(buffer, 0, 1024);
	sprintf(buffer, "\n\nSRN log file %s opened at %s\n",
			logpath,
			gmdate);

	write(fdlog_srn, buffer, strlen(buffer));
}

void init_intro_srn_log(void) {
        write(fdlog_srn, "\n\n", 2);
	write(fdlog_srn, SRN_STRLOG_STARTING, strlen(SRN_STRLOG_STARTING));
        write(fdlog_srn, "\n", 2);

	return;
}
