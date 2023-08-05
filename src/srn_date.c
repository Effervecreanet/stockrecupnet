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

#include <time.h>
#include <stdio.h>
#include <string.h>

#include "srn_date.h"

char		gmdate[33];
char		log_date_now[33];
struct		tm *tmv;

/*
 * Empty global variable gmdate and set it to GMT date format for now
 */

void
set_gmdate(void)
{
	struct tm      *tm;
	time_t		tloc;

	time(&tloc);
	tm = gmtime(&tloc);

	memset(gmdate, 0, sizeof(gmdate));
	strftime(gmdate, 33, "%a, %d %b %Y %H:%M:%S GMT", tm);

	return;
}


/* Set log_date_now to now in a short wsite log format still human readable */

void set_log_date_now(void) {
  time_t now;

  memset(&now, 0, sizeof(time_t));
  time(&now);
  tmv = gmtime(&now);

  memset(log_date_now, 0, LOG_DATE_NOW_SIZE);
  strftime(log_date_now, LOG_DATE_NOW_SIZE, "%d/%b/%Y:%T -0600", tmv);

  return;
}
