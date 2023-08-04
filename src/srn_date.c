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
char		gmdatelog[33];

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


void
set_gmdatelog(void)
{
	struct tm      *tm;
	time_t		tloc;

	time(&tloc);
	tm = gmtime(&tloc);

	memset(gmdatelog, 0, sizeof(gmdate));
	strftime(gmdatelog, 33, "%a.%d.%b.%Y.%H.%M.%S", tm);

	return;
}
