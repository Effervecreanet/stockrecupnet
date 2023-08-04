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

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include "srn_config.h"
#include "srn_socket.h"
#include "srn_http.h"
#include "srn_response.h"

extern jmp_buf	env;

/*
 * Set value value to a string corresponding to the size of filename
 * filename. This function is used prior to send static web site content.
 */

int static_send_reply(int suser, struct response *resp);
int static_send_header(int suser, struct hdr_nv hdrnv[MAX_HEADERS]);
int send_status_line(int suser, struct status_line *sline);

static		off_t
set_content_length(char *value, const char *content_filename)
{
	struct stat	sb;

	memset(&sb, 0, sizeof(struct stat));
	if (stat(content_filename, &sb) < 0) {
		fprintf(stderr, "Error in file %s at line %d with stat (%s (%s)).\n",
			__FILE__,
			__LINE__,
			strerror(errno),
			content_filename);
		return -1;
	}
	memset(value, 0, HEADER_VALUE_SIZE);
	sprintf(value, "%lu", sb.st_size);

	return sb.st_size;
}

/*
 * Send the http header status line.
 */

int
send_status_line(int suser, struct status_line *sline)
{
	if (send_data(suser, "HTTP/1.1", 8) < 0)
		return -1;

	if (send_data(suser, " ", 1) < 0)
		return -1;

	if (send_data(suser, (char *)sline->code, 3) < 0)
		return -1;

	if (send_data(suser, " ", 1) < 0)
		return -1;

	if (send_data(suser, (char *)sline->mess, strlen(sline->mess)) < 0)
		return -1;

	if (send_data(suser, "\r\n", 2) < 0)
		return -1;

	return 1;
}


/*
 * Send the server response http headers name/value pairs.
 */

int
static_send_header(int suser, struct hdr_nv hdrnv[MAX_HEADERS])
{
	struct hdr_nv  *pnv;

	for (pnv = hdrnv; pnv->pname != NULL; pnv++) {
		if (send_data(suser, pnv->pname, strlen(pnv->pname)) < 0)
			return -1;

		if (send_data(suser, ": ", 2) < 0)
			return -1;

		if (pnv->pvalue != NULL) {
			if (send_data(suser, pnv->pvalue, strlen(pnv->pvalue)) < 0)
				return -1;
		} else {
			if (send_data(suser, pnv->value, strlen(pnv->value)) < 0)
				return -1;
		}

		if (send_data(suser, "\r\n", 2) < 0)
			return -1;
	}

	if (send_data(suser, "\r\n", 2) < 0)
		return -1;


	return 1;
}

/*
 * Send the static content http body response
 */
int
static_send_reply(int suser, struct response *resp)
{
	char	       *phdr_clen;
	char	       *pfile;
	char		buffer[1024];
	ssize_t		readb;
	int		fd;

	phdr_clen = hdr_nv_value(resp->hdrnv, "Content-Length");
	if (!phdr_clen)
		return -1;

	if (strcmp(resp->entry.type, "text/html") == 0) {
		set_content_length(phdr_clen, resp->local_entry);
		pfile = (char *)resp->local_entry;
	} else {
		set_content_length(phdr_clen, resp->entry.resource);
		pfile = resp->entry.resource;
	}

	send_status_line(suser, &resp->statusline);
	static_send_header(suser, resp->hdrnv);

	fd = open(pfile, O_RDONLY);
	if (fd < 0) {
		warn("Unable to open file %s (%s)",
		     resp->local_entry,
		     strerror(errno));
		return -1;
	}
	do {
		readb = read(fd, buffer, 1024);
		if (readb <= 0)
			break;
	} while (send_data(suser, buffer, (size_t)readb) > 0);


	close(fd);

	return 1;
}
