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
#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <err.h>


ssize_t send_data(int suser, char *data, size_t length);
ssize_t recv_data(int suser, char *data, size_t length);
int bind_socket(int *s, struct sockaddr_in *sin);
int create_socket(int *s);

ssize_t
send_data(int suser, char *data, size_t length)
{
	return send(suser, data, length, 0);
}


ssize_t
recv_data(int suser, char *data, size_t length)
{
	return recv(suser, data, length, 0);
}

/* Try to bind server socket to resolved inet addr */

int
bind_socket(int *s, struct sockaddr_in *sin)
{
	int		r;

	r = bind(*s, (struct sockaddr *)sin, sizeof(struct sockaddr));
	if (r < 0)
		errx(1, "Unable to bind server socket to inet addr (%s)\n",
		     strerror(errno));


	return *s;
}

/* Create the server socket that will accept TCP connections */

int
create_socket(int *s)
{
	struct timeval tval;

	*s = 0;

	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*s < 0) {
		fprintf(stderr, "Unable to create socket at line %d in file %s\n",
			__LINE__,
			__FILE__);
		fprintf(stderr, "Error (%s) exiting ..\n", strerror(errno));
		exit(1);
	}

	memset(&tval, 0, sizeof(struct timeval));
	tval.tv_sec = 7;
	setsockopt(*s, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(struct timeval));
	setsockopt(*s, SOL_SOCKET, SO_SNDTIMEO, &tval, sizeof(struct timeval));

	return 1;
}
