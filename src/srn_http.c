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
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <setjmp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "srn_config.h"
#include "srn_http.h"
#include "srn_response.h"
#include "srn_socket.h"

extern int fdshm_gdown, fdshm_gdownok;
extern jmp_buf	env;
extern struct static_entry static_entries[];
extern struct static_entry dyn_entries[];
extern int    fdlog;
extern pid_t 	pid;
extern char srnlogbuf[2048];


int srn_isgraph(unsigned char c);
unsigned char *decode_url(char *resource);


/*
 * Check requested resource in declared resources, see srn_config.c Resource
 * beginning with "/store/" is handled with a special treatment. Only static
 * resource are looked for here.
 */

static int
match_resource(struct static_entry *entry_res, char *resource)
{
	uint8_t		i = 0;
	char *pchar;
	char guest_down[254 + SRN_PATH_STORE_SIZE];

	if (strncmp(resource, SRN_PATH_STORE, 6) == 0) {
		for (pchar = resource; *pchar && (srn_isgraph((unsigned char)*pchar)); pchar++);
		if (*pchar)
			return 0;

		memset(guest_down, 0,  254 + SRN_PATH_STORE_SIZE);
		strncpy(guest_down, (const char*)resource, 254 + SRN_PATH_STORE_SIZE);
		pwrite(fdshm_gdown, guest_down, strlen(guest_down), 0);

		entry_res->resource = SRN_PATH_STORE;

		return 1;
	}

	do {
		if (strncmp(static_entries[i].resource,
			    resource,
			    RESOURCE_MAX_SIZE) == 0)
			break;
	} while (static_entries[++i].resource != NULL);


	if (static_entries[i].resource == NULL)
		return 0;

	memcpy(entry_res, &static_entries[i], sizeof(struct static_entry));

	return 1;
}

/*
 * Is http request line valid, if so, record method and requested resource
 */

static int
parse_rline(struct request_line *rline, char *buffer)
{
	char		method[5];
	char		resource[RESOURCE_MAX_SIZE];
	char	       *pmethod = method;
	char	       *presource = resource;
	int		i;
	char	       *qmark;

	memset(method, 0, 5);

	for (i = 0; !isspace(*buffer); i++) {
		*pmethod++ = *buffer++;

		if (i > 4)
			longjmp(env, 1);
	}


	if (strcmp(method, "GET") == 0)
		rline->method = HTTP_GET;
	/*
	 * else if (strcmp(method, "HEAD") == 0) rline->method = HTTP_HEAD;
	 */
	else if (strcmp(method, "POST") == 0)
		rline->method = HTTP_POST;
	else
		longjmp(env, 1);

	while (isspace(*++buffer));

	memset(resource, 0, RESOURCE_MAX_SIZE);

	for (i = 0; !isspace(*buffer); i++) {
		*presource++ = *buffer++;

		if (i > RESOURCE_MAX_SIZE - 1)
			longjmp(env, 1);
	}


	if (*resource == '/' && *(resource + 1) == '\0') {
		strcpy(resource, "/Accueil");
	} else if ((qmark = strchr(resource, '?')) != NULL) {
		strcpy(resource, "/RGPD_Refus");
	}
	/* Skip the leading slash for matching */

	if (!match_resource(&rline->entry, resource + 1))
		longjmp(env, 1);

	while (isspace(*buffer))
		buffer++;

	/* Http version ok ? */
	if (strncmp(buffer, "HTTP/1.1", 8) != 0 &&
	    strncmp(buffer, "HTTP/1.0", 8) != 0)
		longjmp(env, 1);


	/* Is hidden data at the end of line */

	buffer += 8;
	while (isspace(*buffer++));

	if (*buffer != '\0')
		longjmp(env, 1);

	return 1;
}


/*
 * Get and parse http request line
 */

static int
get_rline(int suser, struct request_line *rline)
{
	char		buffer[1024];
	uint16_t	i;

	memset(buffer, 0, 1024);

	for (i = 0; recv_data(suser, &buffer[i], 1) > 0;) {
		if (buffer[i] == '\n')
			break;

		if (++i == 1023)
			longjmp(env, 1);
	}

	parse_rline(rline, buffer);

	return 1;
}

static int
parse_headernv(struct hdr_nv *nv, char *buffer)
{
	uint16_t	i;
	char	       *pbuffer = buffer;

	for (i = 0; *buffer != ':'; i++) {
		if (i == HEADER_NAME_SIZE)
			longjmp(env, 1);
		if (!isalpha(*buffer) && *buffer != '.' && *buffer != '-')
			longjmp(env, 1);
		buffer++;
	}

	*buffer = '\0';
	strncpy(nv->name, pbuffer, HEADER_NAME_SIZE);

	while (isspace(*++buffer));

	pbuffer = buffer;

	for (i = 0; *buffer != '\r' && *buffer != '\n'; i++) {
		if (i == HEADER_VALUE_SIZE)
			longjmp(env, 1);
		buffer++;
	}

	*buffer = '\0';

	strncpy(nv->value, pbuffer, HEADER_VALUE_SIZE);

	return 1;
}

/*
 * Receive and parse http header name/value pairs
 */

static int
get_headernv(int suser, struct hdr_nv nv[MAX_HEADERS])
{
	char		buffer[HEADER_SIZE];
	uint16_t	i, j;
	struct hdr_nv  *pnv = nv;


	for (j = 0; j < MAX_HEADERS; j++) {
		memset(buffer, 0, HEADER_SIZE);

		for (i = 0; recv_data(suser, &buffer[i], 1) > 0;) {
			if (buffer[i] == '\n')
				break;

			if (++i == HEADER_SIZE)
				longjmp(env, 1);
		}

		if (i < 2)
			break;

		parse_headernv(pnv++, buffer);
	}


	/*
	 * for (i = 0; i< MAX_HEADERS && i <9 ; i++) { printf("header_name:
	 * %s\nheader_value: %s\n", nv[i].name, nv[i].value); }
	 */



	return 1;
}

/*
 * Return the http header value corresponding to http header name name
 */

char	       *
hdr_nv_value(struct hdr_nv nv[MAX_HEADERS], const char *name)
{
	struct hdr_nv  *pnv;

	for (pnv = nv; pnv->pname != NULL ||
	     pnv->name[0] != '\0' ||
	     pnv->pvalue != NULL ||
	     pnv->value[0] != '\0'; pnv++) {
		if (pnv->pname == NULL) {
			if (strncmp(pnv->name, name, HEADER_NAME_SIZE) == 0) {
				if (pnv->pvalue == NULL)
					return pnv->value;
				else
					return pnv->pvalue;
			}
		} else {
			if (strncmp(pnv->pname, name, HEADER_NAME_SIZE) == 0) {
				if (pnv->pvalue == NULL)
					return pnv->value;
				else
					return pnv->pvalue;
			}
		}
	}

	/* NOT REACHED */

	return NULL;
}

/* Extract Content-Type form boundary and store it */

int
rec_boundary(char boundary[255], char *pctype)
{
	char	       *pboundary;

	memset(boundary, 0, 255);

	if ((pboundary = strstr(pctype, "boundary=")) == NULL)
		return -1;

	pboundary += sizeof("boundary=") - 1;
	strncpy(boundary, pboundary, 254);

	return 1;
}

/* Receive MIME header and return its length */

static int
http_recvMIME(int s, char buffer[4096])
{
	int		i = 0;
	int		crnlcrnl = 0;

	memset(buffer, 0, 4096);


	/*
	 * If "\r\n\r\n" received, crnlcrnl = 2, http header successfully
	 * received isn't too long
	 */

	while (crnlcrnl < 2 && i < 4096 && recv(s, &buffer[i], 1, 0) == 1) {
		if (buffer[i] == '\r') {
			if (recv(s, &buffer[++i], 1, 0) == 1) {
				if (buffer[i] == '\n')
					crnlcrnl++;
				else
					crnlcrnl = 0;
			} else
				break;
		} else if (buffer[i] == '\n') {
			crnlcrnl++;
			if (recv(s, &buffer[++i], 1, 0) == 1) {
				if (buffer[i] == '\n')
					crnlcrnl++;
				else
					crnlcrnl = 0;
			} else
				break;
		} else
			crnlcrnl = 0;
		i++;
	}

	if (crnlcrnl != 2 || i >= 4096)
		return -1;
	else
		return i;
}

int
srn_isgraph(unsigned char c)
{
	if (!isgraph(c) || c == ' ' || c == '%')
		return 0;

	return 1;
}

/*
 * Extract and save user input filename if right. Accented letters and space
 * character are invalid. A filename containing them will be rejected.
 */

static int
rec_filename(char buffer[4096], char *filename)
{
	char	       *pfilename;
	char	       *pdquote;
	char	       *pchar;

	pfilename = strstr(buffer, "filename=\"");
	if (pfilename == NULL)
		return -1;

	pfilename += sizeof("filename=\"") - 1;

	pdquote = strchr(pfilename, '"');
	if (pdquote == NULL)
		return -1;

	*pdquote = '\0';
	if (strlen(pfilename) > 252) {
		strcat(srnlogbuf, "filename_too_long");
		return -1;
	} else if (*pfilename == '\0') {
		strcat(srnlogbuf, "no_filename_supplied");
		return -1;
	}
	/*
	 * Escape malicious user that would write file in arbitrary directory
	 */
	if (strstr(pfilename, "..") != NULL ||
	    strchr(pfilename, '/') != NULL ||
	    strchr(pfilename, '%') != NULL) {
		strcat(srnlogbuf, "invalid_characters_in_filename_(\"..\"_or_'/')");
		return -1;
	}
	for (pchar = pfilename; *pchar && (srn_isgraph((unsigned char)*pchar)); pchar++);

	if (*pchar) {
		strcat(srnlogbuf, "invalid_characters_in_filename");
		return -1;
	}
	strcpy(filename, pfilename);

	*pdquote = '"';

	return 1;
}

/*
 * Handle HTTP POST method for transfering a file from user device especially
 * a mobile to local harddrive.
 */
int
receive_form_data(int s, char *filename, off_t clen, char boundary[255])
{
	static char	buffer[4096];
	size_t		torecv = 1024;
	ssize_t		ret;
	int		MIMElen = 0;
	int		fd;
	unsigned long	boundarylen = 0;
	char		path[254 + SRN_PATH_STORE_SIZE];

	MIMElen = http_recvMIME(s, buffer);
	if (MIMElen < 0) {
		return -1;
	}

	/* Sub MIME header length to clen */

	clen -= MIMElen;

	boundarylen = strlen((const char *)boundary);

	ret = rec_filename(buffer, filename);
	if (ret < 0) {
		while ((clen -= recv_data(s, buffer, 1024)) > 0)
			;
		return -1;
	}

	memset(path, 0, 254 + SRN_PATH_STORE_SIZE);

	strcpy(path, SRN_PATH_STORE);
	strcat(path, filename);

	if (!access(path, F_OK)) {
		while ((clen -= recv_data(s, buffer, 1024)) > 0)
			;
		return 0;
	}
	fd = open(path, O_TRUNC | O_CREAT | O_WRONLY | F_EXLCK, 0774);
	if (fd < 0) {
		warn("Unable to open %s for writing (%s)", path, strerror(errno));
		close(s);
		return -1;
	}

	/* printf("Receiving %s...\n", path); */

	if (clen <= 1024) {
		ret = recv(s, buffer, torecv, 0);
		if (ret <= 0) {
			close(fd);
			unlink(path);
			return -1;
		}

		write(fd, buffer, (size_t)(ret) - boundarylen - 8);

		goto smallfile;
	}

	while (1) {
		ret = recv(s, buffer, torecv, 0);
		if (ret <= 0) {
			close(fd);
			unlink(path);
			return -1;
		}
		clen -= ret;

		/* End of rx ? */
		if (clen <= 0) {
			/* Ignore trailling `CR LF boundary-- CR LF' string */
			write(fd, buffer, (size_t)(ret) - boundarylen - 8);
			break;
		} else if ((unsigned long)clen < (1024 + boundarylen + 8) && clen > 1024) {
			/* Data overlap */
			torecv = 777;
		}

		write(fd, buffer, (size_t)ret);
	}

smallfile:
	close(fd);

	return 1;
}

/*
 * * Parse a content-length header
 */
int
http_parse_length(const char *p, off_t * length)
{
	off_t		len;
	for (len = 0; *p && isdigit((unsigned char)*p); ++p)
		len = len * 10 + (*p - '0');
	if (*p)
		return (-1);

	*length = len;
	return (0);
}

int
get_form_data(int suser, char *filename, off_t clen, char boundary[255])
{
	return receive_form_data(suser, filename, clen, boundary);
}

int
get_http_request(int suser, struct request_line *rline,
		 struct hdr_nv hdrnv[MAX_HEADERS])
{
	get_rline(suser, rline);
	get_headernv(suser, hdrnv);

	return 1;
}
