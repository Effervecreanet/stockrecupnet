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
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <netdb.h>
#include <stdbool.h>

#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>
#include <err.h>
#include <regex.h>
#include <fcntl.h>
#include <locale.h>

#include "srn_config.h"
#include "srn_log.h"
#include "srn_socket.h"
#include "srn_http.h"
#include "srn_response.h"
#include "srn_handler.h"
#include "srn_date.h"
#include "srn_moved.h"
#include "srn_pin.h"
#include "srn_main.h"


extern char	gmdate[33];
extern char	gmdatelog[33];
char  nakedhostname[NI_MAXHOST];
char  wwwhostname[NI_MAXHOST];
int fdshm_gdown, fdshm_gdownok, fdshm_suspend;
int 	fdlog;
char   strlog[2048];
jmp_buf  env;

struct hostserv {
	char		hostname[NI_MAXHOST];
	char		servname[NI_MAXSERV];
	uint8_t		canonname;
};

/*
 * Print a very short explanation of this program.
 */

void
usage(const char *progname)
{
  fprintf(stderr, "usage: %s [-d] -a listen_addr -h http_hostname -n"
                  " naked_domain_name -p listen_port\n", progname);

  return;
}


int
main(int argc, char **argv)
{
	struct sockaddr_in sain_addr, usersin;
	char		addrstr[INET_ADDRSTRLEN];
	int		sserv;
	int		suser = 0;
	socklen_t	slen;
	struct request_line rline;
	struct hdr_nv	hdrnv[MAX_HEADERS];
	struct response	resp;
	char	       *phdr_host;
	regex_t		reg1, reg2;
	pid_t pid = 1, pid0 = 1;
	char guest_down[254 + SRN_PATH_STORE_SIZE];
	bool daemonize = false;
	char ch, srnsuspend[2];

	/* Without arguments run the daemon otherwise print usage */
	if (argc < 4) {
		usage(argv[0]);
		exit(2);
	}


       while ((ch = getopt(argc, argv, "da:h:n:p:")) != -1) {
         switch (ch) {
           case 'd':
             daemonize = true;
             break;
           case 'a':
             inet_aton(optarg, &sain_addr.sin_addr);
             strcpy(addrstr, optarg);
             break;
           case 'h':
             strncpy(wwwhostname, optarg, NI_MAXHOST - 1);
             break;
           case 'n':
             strncpy(nakedhostname, optarg, NI_MAXHOST - 1);
             break;
           case 'p':
             sain_addr.sin_port = htons(atoi(optarg));
             break;
           default:
             usage(argv[0]);
             exit(1);
           }
        }

        fdshm_gdown = shm_open("/srnsharedmem", O_CREAT | O_RDWR, 0600);
	if (fdshm_gdown < 0) {
		perror("shm_open");
		return 1;
	}

        fdshm_gdownok = shm_open("/srnsharedmema", O_CREAT | O_RDWR, 0600);
	if (fdshm_gdown < 0) {
		perror("shm_open");
		return 1;
	}

        fdshm_suspend = shm_open("/srnsharedmemb", O_CREAT | O_RDWR, 0600);
	if (fdshm_gdown < 0) {
		perror("shm_open");
		return 1;
	}

	if (ftruncate(fdshm_gdown,  254 + SRN_PATH_STORE_SIZE) < 0) {
		perror("ftruncate");
		return 1;
	}

	if (ftruncate(fdshm_gdownok,  254 + SRN_PATH_STORE_SIZE) < 0) {
		perror("ftruncate");
		return 1;
	}

	if (ftruncate(fdshm_suspend, 2) < 0) {
		perror("ftruncate");
		return 1;
	}

	memset(guest_down, 0,  254 + SRN_PATH_STORE_SIZE);
	pwrite(fdshm_gdown, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

	guest_down[0] = 'A';
	pwrite(fdshm_gdownok, guest_down, 254 + SRN_PATH_STORE_SIZE, 0);

	srnsuspend[0] = srnsuspend[1] = 0;
	pwrite(fdshm_suspend, srnsuspend, 2, 0);

	if (create_socket(&sserv) < 0)
		exit(1);

	open_log(addrstr);

	write(fdlog, STRLOG_STARTING,
	             strlen(STRLOG_STARTING));
	write(fdlog, wwwhostname,
		     strlen(wwwhostname));
	write(fdlog, "\n", 1);

	write(fdlog, STRLOG_STARTING_ADDR,
	             strlen(STRLOG_STARTING_ADDR));
	write(fdlog, addrstr, strlen(addrstr));
	write(fdlog, "\n", 1);

	createdb();

	sain_addr.sin_family = AF_INET;
	bind_socket(&sserv, &sain_addr);

	listen(sserv, 24);

	printf("Accepting connections on %s\n", addrstr);

	if (daemonize)
	  daemon(1, 0);

	set_gmdate();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	memset(&reg1, 0, sizeof(regex_t));
	memset(&reg2, 0, sizeof(regex_t));

	/* TLD we will accept for incoming connections */

	if (regcomp(&reg1, "(...)",
		    REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
		printf("regcomp() error\n");
		return -1;
	}

	/* TLD we will reject for incoming connections */

	if (regcomp(&reg2, "(...)",
		    REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
		printf("regcomp() error\n");
		return -1;
	}
	
  srand48(666663);

	for (;;) {
		if (pid == 0 || pid0 == 0) {
			if (strlog[0]) {
				while(lockf(fdlog, F_LOCK, 0) < 0)
					;
				write(fdlog, strlog, strlen(strlog));
				write(fdlog, "\n", 1);
				lockf(fdlog, F_ULOCK, 0);
			}
			exit(0);
		}

		if (strlog[0]) {
			while(lockf(fdlog, F_LOCK, 0) < 0)
				;
			write(fdlog, strlog, strlen(strlog));
			write(fdlog, "\n", 1);
			lockf(fdlog, F_ULOCK, 0);
		}

		pread(fdshm_suspend, srnsuspend, 2, 0);
		if (srnsuspend[0] == '\x01') {
			sleep(60);
			srnsuspend[0] = srnsuspend[1] = 0;
			pwrite(fdshm_suspend, srnsuspend, 2, 0);
			system("sed -e '10,$p' -n blocked/srn > blocked/srntmp && mv blocked/srntmp blocked/srn ;");
		}

		slen = sizeof(struct sockaddr_in);
		suser = accept(sserv, (struct sockaddr *)&usersin, &slen);

		if (suser < 0) {
                        if (errno != EAGAIN) {
				warn("At line %d in file %s: ",
				     __LINE__,
				     __FILE__);
				perror("accept");
                        }
			continue;
		} else if ((pid0 = fork()) != 0) {
			close(suser);
			continue;
    }
		

		set_gmdate();
		set_gmdatelog();

		/* printf("Oncoming connection from %s:%s\n", hs.hostname,
 * hs.servname); */

		memset(strlog, 0, 2048);
		/*
		sprintf(strlog, "%s:oncoming:%s:%s:", gmdatelog,
					              hs.hostname,
						      hs.servname);
		*/
	
		memset(&rline, 0, sizeof(struct request_line));
		memset(hdrnv, 0, sizeof(struct hdr_nv) * MAX_HEADERS);
		memset(&resp, 0, sizeof(struct response));

		switch(setjmp(env)) {
		case 2:
			pwrite(fdshm_suspend, "\x01", 2, 0);
		case 1:
			strcat(strlog, "exception_error");
			shutdown(suser, SHUT_RDWR);
			close(suser);
			continue;
			break;
		case 0:
		default:
			break;
		}

		get_http_request(suser, &rline, hdrnv);
		if (rline.method == HTTP_GET) {
			pid = fork();
			if (pid != 0) {
				close(suser);
				memset(strlog, 0, 2048);
				continue;
			}
			shutdown(suser, SHUT_RD);
		}

		phdr_host = hdr_nv_value(hdrnv, "Host");

		if (phdr_host == NULL || *phdr_host == '\0') {
			strcat(strlog, "null_host");
			goto closeconn; 
		}

		if (strncasecmp(phdr_host, nakedhostname,
        strlen(nakedhostname)) == 0) {
			send_moved_perm(suser);
			strcat(strlog, "root_redirection");
			goto closeconn; 
		} else if (strncasecmp(phdr_host, wwwhostname,
						  strlen(wwwhostname)) != 0) {
			strcat(strlog, "bad_host:blocked_request");
			goto closeconn; 
		}

		if (srn_handle(suser, &rline, hdrnv, "srn") == 2) {
			strcat(strlog, "cont");
			/* continue; */
		}

closeconn:
		shutdown(suser, SHUT_RDWR);
		close(suser);
		srand48(usersin.sin_port & 0xA7C7);

		strcat(strlog, rline.entry.resource);
		strcat(strlog, ":sent");
	}

}
