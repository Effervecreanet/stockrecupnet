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

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <netdb.h>

#include "srn_config.h"
#include "srn_http.h"
#include "srn_date.h"
#include "srn_response.h"
#include "srn_moved.h"
#include "srn_socket.h"
#include "srn_send.h"


extern jmp_buf	env;
extern char	gmdate[33];
extern char  wwwhostname[NI_MAXHOST];

/*
   Create a http status line with Moved permanently http status
 */

static void
create_status_line_movedperm(struct status_line *sline)
{
	sline->code = "301";
	sline->mess = "Moved Permanently";
	sline->vers = "HTTP/1.1";
	return;
}

/*
   Create a http response with Moved permanently response
 */

static void
create_response_movedperm(struct response *resp)
{
	struct hdr_nv  *phnv;
	char	       *clength;

	create_status_line_movedperm(&resp->statusline);
	create_header(resp->hdrnv, 0, "text/plain", NULL, 0);

	clength = hdr_nv_value(resp->hdrnv, "Content-Length");
	sprintf(clength, "%lu", strlen(wwwhostname) -1);

	for (phnv = (struct hdr_nv *)&resp->hdrnv; phnv->pname != NULL; phnv++);

	phnv->pname = "Location";
	sprintf(phnv->value, "http://%s/Accueil", wwwhostname);

	return;
}

/*
   Send http Moved Permanetly response
*/

int
send_moved_perm(int suser)
{
	struct response resp;

	memset(&resp, 0, sizeof(struct response));
	create_response_movedperm(&resp);

	send_status_line(suser, &resp.statusline);
	static_send_header(suser, resp.hdrnv);
 	send_data(suser, wwwhostname, strlen(wwwhostname));	

	return 1;
}
