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

#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#include "srn_config.h"
#include "srn_pin.h"


/*
 * Generate a pin code that doesn't already exist in the database. So, 1)
 * Generate a pin 2) Check if it already exist 3) Regenerate one until this
 * one doesn't exist 4) Return this one If the pin already exist, the
 * filename relative to the pin is copied into filename. This last sentence
 * pinpoint a little lost of time in the global execution.
 */

#define GEN_PIN(PPIN, C, ARV)   ARV = lrand48();                                        \
				do {							\
					C = ((ARV & 0x0000000F) > 9 ? 			\
					     (ARV & 0x00000007) : (ARV & 0x0000000F));	\
					*PPIN++ = C + '0';				\
					ARV >>= 4;					\
				} while(*PPIN)

int
gen_pin(unsigned char pin[PIN_SIZE + 1], char filename[254])
{
	long int	arv;
	unsigned char   c;
        unsigned char *ppin;

	do {
		memset(pin, 2, PIN_SIZE);
		pin[PIN_SIZE] = '\0';
		ppin = pin;
		GEN_PIN(ppin, c, arv);
	} while (pin_exist(pin, filename));

	return 1;
}

/*
 * Return 1 if pin exist in the database with the relative filename copied
 * into filename Return 0 if walk of database does'nt show any presence of
 * pin in the database
 */

int 
pin_exist(unsigned char pin[PIN_SIZE + 1], char filename[254])
{
	FILE	       *fp;
	char		line[254 + PIN_SIZE + 10];

	fp = fopen(SRN_DB, "r");
	if (fp == NULL) {
		errx(1, "Unable to fopen %s (%s)",
		     SRN_DB,
		     strerror(errno));
	}
	do {
		memset(line, 0, 254 + PIN_SIZE + 10);
		fgets(line, 254 + PIN_SIZE + 10, fp);
		if (strncmp((const char*)pin, line, 6) == 0) {
			unsigned long linelen;

			memset(filename, 0, 254);

			linelen = strlen(line);
			line[linelen - 1] = '\0';
			strncpy(filename, &line[7], 254);

			fclose(fp);

			return 1;
		}
	} while (!feof(fp));

	fclose(fp);

	return 0;
}

/*
 * Record generated pin with its associated file in the database SRN_DB.
 * Install an exclusive lock in order to not let forked process attempt to
 * simultaneously write in the database.
 */

int
rec_pin(unsigned char pin[PIN_SIZE + 1], char *filename)
{
	int		fd;
	char		line[254 + PIN_SIZE + 10];
	int		cntopentry = 0;

	do {
		fd = open(SRN_DB, O_WRONLY | O_APPEND | F_EXLCK);
		if (fd > -1)
			break;
		usleep(777);
	} while (++cntopentry < 10);

	if (cntopentry == 10)
		errx(1, "At line %d in file %s unable to open file %s",
		     __LINE__,
		     __FILE__,
		     SRN_DB);

	memset(line, 0, 254 + PIN_SIZE + 10);
	sprintf(line, "%s:%s\n", pin, filename);
	write(fd, line, strlen(line));

	close(fd);

	return 1;
}

/*
 */

int
rem_pin(long fposmatch, unsigned short linelen)
{
	int		fd;
	int		cntopentry = 0;
	char		c = '-';
	char		lf = '\n';
	do {
		fd = open(SRN_DB, O_WRONLY | F_EXLCK);
		if (fd > -1)
			break;
		usleep(777);
	} while (++cntopentry < 10);

	if (cntopentry == 10)
		errx(1, "At line %d in file %s unable to open file %s",
		     __LINE__,
		     __FILE__,
		     SRN_DB);

	lseek(fd, (off_t)fposmatch, SEEK_SET);

	while(--linelen > 0)
		write(fd, &c, 1);

	write(fd, &lf, 1);

	close(fd);

	return 1;
}

/* Create store directory, create db directory, create database text file */

void
createdb(void)
{
	int fd;

	if (access(SRN_PATH_STORE, X_OK | R_OK | W_OK) != 0) {
		char path[SRN_PATH_STORE_SIZE];

		strcpy(path, SRN_PATH_STORE);
		path[strlen(path) - 1] = '\0';

		if (mkdir(path, 0755) != 0) {
			errx(3, "Unable to create store (%s): %s\n", "./store",
								     strerror(errno));
		}
	}

	if (access("./db", X_OK | R_OK | W_OK) != 0) {
		if (mkdir("./db", 00755) != 0) {
			errx(3, "Unable to create database (%s): %s\n", "./db",
								     strerror(errno));
		}
	}

	if (access(SRN_DB, R_OK | W_OK) != 0) {
		fd = open(SRN_DB, O_CREAT, 0644);
		if (fd < 0) 
			errx(3, "Unable to create database (%s): %s\n",
				SRN_DB,
				strerror(errno));
		close(fd);
	}

	return;
}
