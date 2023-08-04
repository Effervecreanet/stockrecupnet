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

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>


#include "srn_config.h"
#include "srn_http.h"
#include "srn_response.h"
#include "srn_send.h"
#include "srn_pin.h"
#include "srn_socket.h"
#include "srn_down.h"
#include "srn_block.h"
#include "srn_handler.h"


extern int fdshm_gdown, fdshm_gdownok;
extern jmp_buf env;
extern int    fdlog;
extern char	gmdate[33];
extern pid_t pid;
extern char strlog[2048];
extern char nakedhostname[NI_MAXHOST];
extern char wwwhostname[NI_MAXHOST];

/*
 * This code should be commented.
 */

struct pthrdarg {
	int		suser;
	struct hdr_nv	hdrnv[MAX_HEADERS];
};

void	       *
srn_receive_file(void *arg)
{
	char		filename[254];
	char		boundary[255];
	char	       *clength;
	char	       *ctype;
	off_t		off_clength = 0;
	int		r;
	int		suser;
	struct hdr_nv	hdrnv[MAX_HEADERS];
	char            strlogthrdbuff[2048];
	char   		*strlogthrd = strlogthrdbuff;

	memset(strlogthrd, 0, 2048);
	strncpy(strlogthrd, strlog, 2048); 

	suser = ((struct pthrdarg *)arg)->suser;
	memcpy(hdrnv, &((struct pthrdarg *)arg)->hdrnv, sizeof(struct hdr_nv) * MAX_HEADERS);

	clength = hdr_nv_value(hdrnv, "Content-Length");
	if (clength == NULL || *clength == '\0')
		goto err;

	if (http_parse_length(clength, &off_clength) < 0)
		goto err;

	ctype = hdr_nv_value(hdrnv, "Content-Type");
	if (ctype == NULL || *ctype == '\0')
		goto err;

	if (rec_boundary(boundary, ctype) < 0)
		goto err;

	memset(filename, 0, 254);
	r = get_form_data(suser, filename, off_clength, boundary);
	if (r == -1) {
		struct response	resp;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &dyn_entries[4], sizeof(struct static_entry));

		if (create_response(&resp, hdrnv) < 0)
			goto err;

		static_send_reply(suser, &resp);

		strcat(strlogthrd, "invalid_input_data_or_conn_abort");
		
		goto err;
	} else if (r == 0) {
		struct response	resp;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &dyn_entries[1], sizeof(struct static_entry));

		if (create_response(&resp, hdrnv) < 0)
			goto err;

		static_send_reply(suser, &resp);

		strcat(strlogthrd, "input_file_exist");

		goto err;
	} else if (r == 1) {
#define PIN_SIZE	6
		struct response	resp;
		char	       *pcontent;
		char	       *clen;
		char		buffer[8192];
		unsigned long	bufferlen = 0;
		unsigned char	pin[PIN_SIZE + 1];
		char		filenamedup[254];
		int		fnamelen;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &dyn_entries[0],
		       sizeof(struct static_entry));
		memset(filenamedup, 0, 254);

		gen_pin(pin, filenamedup);

		fnamelen = strlen(filename);
		if (fnamelen > 22) {
			memset(filenamedup, 0, 254);
			strncpy(filenamedup, filename, 12);
			filenamedup[12] = '(';
			filenamedup[13] = '.';
			filenamedup[14] = '.';
			filenamedup[15] = '.';
			filenamedup[16] = ')';
			filenamedup[17] = '0';
			strncpy(&filenamedup[17], &filename[fnamelen - 4], 4);
		} else {
			strcpy(filenamedup, filename);
		}

		pcontent = create_dynamic_response(&resp, hdrnv,
						   filename,
						   filenamedup,
						   pin);
		if (!pcontent)
			goto err;

		memset(buffer, 0, 8192);
		memcpy(buffer, pcontent, 8192);
		free(pcontent);


		clen = hdr_nv_value(resp.hdrnv, "Content-Length");

		bufferlen = strlen(buffer);
		sprintf(clen, "%lu", bufferlen);

		if (send_status_line(suser, &resp.statusline) < 0)
			goto err;
		if (static_send_header(suser, resp.hdrnv) < 0)
			goto err;

		if (send_data(suser, buffer, bufferlen) < 0)
			goto err;

		rec_pin(pin, filename);
		strcat(strlogthrd, "file:");
		strcat(strlogthrd, filename);
		strcat(strlogthrd, ":stored_with_pin:");
		strcat(strlogthrd, (const char*)&pin[0]);
	}
err:
	while(lockf(fdlog, F_LOCK, 0) < 0)
		;
	write(fdlog, strlogthrd, strlen(strlogthrd));
	write(fdlog, "\n", 1);

	lockf(fdlog, F_ULOCK, 0);
	
	shutdown(suser, SHUT_WR);
	while(close(suser) < 0);

	return NULL;
}

struct pthrdsfarg {
	int		suser;
	int		fd;
	int		delete;
	long		fposmatch;
	char		guest_download_ok[254 + SRN_PATH_STORE_SIZE];
	int		buffer2len;
};

void	       *
send_file(void *argsf)
{
	char		buffer[1024];
	ssize_t		readb;
	int		fd;
	int		suser;
	int		delete;
	int		len2;
	long		fposmatch = 0;
	char   		strlogthrdbuff[2048];
	char   		*strlogthrd = strlogthrdbuff;

	memset(strlogthrd, 0, 2048);
	strncpy(strlogthrd, strlog, 2048); 


	fd = ((struct pthrdsfarg *)argsf)->fd;
	suser = ((struct pthrdsfarg *)argsf)->suser;
	delete = ((struct pthrdsfarg *)argsf)->delete;
	fposmatch = ((struct pthrdsfarg *)argsf)->fposmatch;
	len2 = ((struct pthrdsfarg *)argsf)->buffer2len;

	do {
		readb = read(fd, buffer, 1024);
		if (readb <= 0)
			break;
	} while (send_data(suser, buffer, (size_t) readb) > 0);

	close(fd);

	if (delete == 1 && readb <= 0) {
		rem_pin(fposmatch, (unsigned short)len2);
		remove(((struct pthrdsfarg *)argsf)->guest_download_ok);
	}

	strcat(strlogthrd, ((struct pthrdsfarg *)argsf)->guest_download_ok);

	if (readb <= 0)
		delete > 0 ? strcat(strlogthrd, ":sent_received_deleted\n") :
			     strcat(strlogthrd, ":sent_received\n");
	else
	     strcat(strlogthrd, ":conn_abort\n");

        while(lockf(fdlog, F_LOCK, 0) < 0)
		;

	write(fdlog, strlogthrd, strlen(strlogthrd));

	lockf(fdlog, F_ULOCK, 0);

	shutdown(suser, SHUT_WR);

	while(close(suser) < 0);

	return NULL;
}

int
srn_handle(int suser, struct request_line *rline,
	   struct hdr_nv hdrnv[MAX_HEADERS],
	   char *usraddr)
{
	int8_t		delete = 0;
	char		buffdel[sizeof("&supprimer=0")];

	if (rline->method == HTTP_POST && strcmp(rline->entry.resource, SRN_HTTP_ENDPOINT_STORE) == 0) {
		struct pthrdarg	arg;

		memset(&arg, 0, sizeof(struct pthrdarg));
		arg.suser = suser;
		memcpy(&arg.hdrnv, hdrnv, sizeof(struct hdr_nv) * MAX_HEADERS);

		srn_receive_file(&arg);
		return 2;
	} else if (rline->method == HTTP_POST && strcmp(rline->entry.resource, SRN_HTTP_ENDPOINT_RETRIEVE) == 0) {
		char	       *clength = NULL;
		unsigned char	pin[PIN_SIZE + 1];
		char		filename[254];

		clength = hdr_nv_value(hdrnv, "Content-Length");
		if (clength == NULL || *clength == '\0')
			longjmp(env, 1);

		if (strcmp(clength, "10") == 0 || strcmp(clength, "22") == 0) {
			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, (char *)pin, 4) != 4)
				longjmp(env, 1);

			if (strcmp((const char *)pin, "pin=") != 0)
				longjmp(env, 1);

			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, (char *)pin, PIN_SIZE) != PIN_SIZE)
				longjmp(env, 1);

		} else {
			goto clen_ne_pinsize;
		}

		if (strcmp(clength, "22") == 0) {
			memset(buffdel, 0, sizeof("&supprimer=0"));
			if (recv_data(suser, buffdel, sizeof("&supprimer=0") - 1) !=
			    sizeof("&supprimer=0") - 1)
				longjmp(env, 1);

			if (strcmp(buffdel, "&supprimer=0") == 0)
				delete = 0;
			else if (strcmp(buffdel, "&supprimer=1") == 0)
				delete = 1;
			else
				longjmp(env, 1);
		}

		strcat(strlog, "retrieve_store:");
		if (isblocked(usraddr))
			longjmp(env, 2);

		if (!pin_exist(pin, filename)) {
			struct response	resp;
			char	       *pua;
clen_ne_pinsize:
			memset(&resp, 0, sizeof(struct response));
			memcpy(&resp.entry, &dyn_entries[2], sizeof(struct static_entry));
			create_response(&resp, hdrnv);
			static_send_reply(suser, &resp);


			pua = hdr_nv_value(hdrnv, "User-Agent");
			if (pua == NULL || *pua == '\0')
				longjmp(env, 1);

			strcat(strlog, "wrong_pin:");
			strcat(strlog, (const char*)&pin[0]);

			addblocked(usraddr, pua);
			longjmp(env, 1);
		} else {
			struct response	resp;
			char	       *gdown;
			char guest_downok[254 + SRN_PATH_STORE_SIZE];

			create_guest_down(&resp, filename);
			gdown = hdr_nv_value(resp.hdrnv, "Location");

			if (send_status_line(suser, &resp.statusline) < 0)
				longjmp(env, 1);

			if (static_send_header(suser, resp.hdrnv) < 0)
				longjmp(env, 1);

			memset(guest_downok, 0,  254 + SRN_PATH_STORE_SIZE);

			gdown += sizeof("https://");
			gdown += strlen(wwwhostname);
			strncpy(guest_downok, gdown, 254 + SRN_PATH_STORE_SIZE);

			pwrite(fdshm_gdownok, guest_downok, strlen(guest_downok), 0);

			strcat(strlog, "true_pin:");
			strcat(strlog, (const char*)&pin[0]);
		}
	} else if ((rline->method == HTTP_GET || rline->method == HTTP_POST) &&
		   strcmp(rline->entry.resource, SRN_PATH_STORE) == 0) {
		struct response	resp;
		char	       *phdr_clen = NULL;
		struct stat	sb;
		int		fd;
		char		buffer[1024];
		char		buffer2[1024];
		FILE	       *fp;
		char		pin[PIN_SIZE + 1];
		char	       *clength = NULL;
		long		fposmatch = 0;
		struct hdr_nv  *phnv;
		struct pthrdsfarg pthrdargsf;
		char guest_down[254 + SRN_PATH_STORE_SIZE];
		char guest_downok[254 + SRN_PATH_STORE_SIZE];

		memset(guest_down, 0, 254 + SRN_PATH_STORE_SIZE);
		memset(guest_downok, 0, 254 + SRN_PATH_STORE_SIZE);

		pread(fdshm_gdown, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);
		pread(fdshm_gdownok, guest_downok, 254 + SRN_PATH_STORE_SIZE, 0);

		if (strcmp(guest_downok, guest_down) != 0)
			longjmp(env, 1);

		clength = hdr_nv_value(hdrnv, "Content-Length");
		if (clength == NULL || *clength == '\0')
			longjmp(env, 1);

		if (strcmp(clength, "10") == 0 || strcmp(clength, "22") == 0) {
			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, pin, 4) != 4)
				longjmp(env, 1);

			if (strcmp(pin, "pin=") != 0)
				longjmp(env, 1);

			memset(pin, 0, PIN_SIZE + 1);
			if (recv_data(suser, pin, PIN_SIZE) != PIN_SIZE)
				longjmp(env, 1);
		} else {
			longjmp(env, 1);
		}

		if (strcmp(clength, "22") == 0) {
			memset(buffdel, 0, sizeof("&supprimer=0"));
			if (recv_data(suser, buffdel, sizeof("&supprimer=0") - 1) !=
			    sizeof("&supprimer=0") - 1)
				longjmp(env, 1);

			if (strcmp(buffdel, "&supprimer=0") == 0)
				delete = 0;
			else if (strcmp(buffdel, "&supprimer=1") == 0)
				delete = 1;
			else
				longjmp(env, 1);

		}

		sprintf(buffer, "%s:%s\n", pin, &guest_down[6]);

		if (isblocked(usraddr))
			longjmp(env, 1);

		if ((fp = fopen(SRN_DB, "r")) == NULL) {
			perror("fopen");
			errx(1, "Unable to open %s (%s)", SRN_DB, strerror(errno));
		}
		do {
			memset(buffer2, 0, 1024);
			fgets(buffer2, 1024, fp);
			if (strcmp(buffer, buffer2) == 0) {
				fposmatch = ftell(fp) - (long)strlen(buffer2);
				goto match;
			}
		} while (!feof(fp));

		/* printf("1:%s\n2:%s\n", buffer, buffer2); */

		if (feof(fp)) {
			char	       *pua;
			char guest_down[254 + SRN_PATH_STORE_SIZE];

			memset(guest_down, 0,  254 + SRN_PATH_STORE_SIZE);
			pwrite(fdshm_gdown, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

			guest_down[0] = 'A';
			pwrite(fdshm_gdownok, guest_down, 1, 0);

			fclose(fp);
			pua = hdr_nv_value(hdrnv, "User-Agent");
			if (pua == NULL || *pua == '\0')
				longjmp(env, 1);

			addblocked(usraddr, pua);
			longjmp(env, 1);
		}
match:

		fclose(fp);

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line(&resp.statusline);
		create_header(resp.hdrnv, HTTP_ALLOW_POST,
			      "application/octet-stream",
			      NULL,
			      1);

		send_status_line(suser, &resp.statusline);

		memset(&sb, 0, sizeof(struct stat));
		stat(guest_downok, &sb);

		phdr_clen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(phdr_clen, "%lu", sb.st_size);

		for (phnv = (struct hdr_nv *)&resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Content-Disposition";
		strcpy(phnv->value, "attachment");

		if (static_send_header(suser, resp.hdrnv) < 0)
			longjmp(env, 1);

		memset(guest_down, 0, 254 + SRN_PATH_STORE_SIZE);
		pwrite(fdshm_gdown, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

		fd = open(guest_downok, O_RDONLY);
		if (fd < 0) {
			warn("Unable to open %s (%s)", guest_downok, strerror(errno));
			longjmp(env, 1);
		}

		pthrdargsf.suser = suser;
		pthrdargsf.fd = fd;
		pthrdargsf.delete = delete;
		pthrdargsf.fposmatch = fposmatch;
		memcpy(pthrdargsf.guest_download_ok, guest_downok, 254 + SRN_PATH_STORE_SIZE);
		pthrdargsf.buffer2len = (int)strlen(buffer2);

		send_file(&pthrdargsf);

		memset(guest_down, 0,  254 + SRN_PATH_STORE_SIZE);
		pwrite(fdshm_gdown, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

		guest_down[0] = 'A';
		pwrite(fdshm_gdownok, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

		return 2;
	} else if (rline->method == HTTP_GET &&
		   strcmp(rline->entry.resource, "RGPD_Ok") == 0) {
		struct response	resp;
		char		cookie[2048];
		char		expires[33];
		struct hdr_nv  *phnv;
		char	       *preferer;
		char	       *pclen;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line_tempred(&resp.statusline);

		memset(cookie, 0, 2048);

		snprintf(cookie, 2048, "RGPD=1; Domain=.%s; Expires=", nakedhostname);

		memset(expires, 0, 33);
		set_expires(expires);
		strcat(cookie, expires);

		create_header(resp.hdrnv, 0, "text/html", cookie, 0);

		for (phnv = resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Location";

		if ((preferer = hdr_nv_value(hdrnv, "Referer")) != NULL && *preferer != '\0')
			strncpy(phnv->value, preferer, HEADER_VALUE_SIZE - 1);
		else
			sprintf(phnv->value, "https://%s/Accueil", wwwhostname);

		pclen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(pclen, "%lu", strlen(wwwhostname) - 1);

		send_status_line(suser, &resp.statusline);
		static_send_header(suser, resp.hdrnv);

		send_data(suser, wwwhostname, strlen(wwwhostname) - 1);

	} else if (rline->method == HTTP_GET &&
		   strcmp(rline->entry.resource, "RGPD_Refus") == 0) {
		struct response	resp;
		char		cookie[2048];
		char		expires[33];
		struct hdr_nv  *phnv;
		char	       *preferer;
		char	       *pclen;

		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));

		create_status_line_tempred(&resp.statusline);

		memset(cookie, 0, 2048);

		snprintf(cookie, 2048, "RGPD=0; Domain=.%s; Expires=", nakedhostname);

		memset(expires, 0, 33);
		set_expires(expires);
		strcat(cookie, expires);

		create_header(resp.hdrnv, 0, "text/html", cookie, 0);

		for (phnv = resp.hdrnv; phnv->pname != NULL; phnv++);

		phnv->pname = "Location";

		if ((preferer = hdr_nv_value(hdrnv, "Referer")) != NULL &&
		    *preferer != '\0' && strstr(preferer, wwwhostname) != NULL)
			strncpy(phnv->value, preferer, HEADER_VALUE_SIZE - 1);
		else
			sprintf(phnv->value, "https://%s/Accueil", wwwhostname);

		pclen = hdr_nv_value(resp.hdrnv, "Content-Length");
		sprintf(pclen, "%lu", strlen(wwwhostname) - 1);

		send_status_line(suser, &resp.statusline);
		static_send_header(suser, resp.hdrnv);

		send_data(suser, wwwhostname, strlen(wwwhostname) - 1);
	} else {
		struct response	resp;
		memset(&resp, 0, sizeof(struct response));
		memcpy(&resp.entry, &rline->entry, sizeof(struct static_entry));
		create_response(&resp, hdrnv);
		static_send_reply(suser, &resp);
	}
	return 1;
}
