/*
 * Copyright (c) 2020 Franck Lesage <francksys@free.fr>
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

#include "srn_config.h"
#include "srn_http.h"


/*
 * Static web site resources are declared here
 */

struct static_entry static_entries[] = {
	{
		HTTP_ALLOW_GET,
		"Accueil", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"RGPD_Ok", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"RGPD_Refus", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"mentions-legales", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"aide", "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"sitemap_2022_06_16.xml", "application/xml", NULL,
	},
	{
		HTTP_ALLOW_GET | HTTP_ALLOW_POST,
		SRN_HTTP_ENDPOINT_STORE, "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET | HTTP_ALLOW_POST,
		SRN_HTTP_ENDPOINT_RETRIEVE, "text/html", "; charset=ISO-8859-1",
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_ACCUEIL.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MENU_ACCUEIL.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MENU_STOCKAGE.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MENU_STOCKAGE.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MENU_RECUPERATION.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MENU_AIDE.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_STOCKAGE.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_STOCKAGE_FRAMES.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_RECUPERATION.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_AIDE.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_DESKTOP_BG_MENTIONS_LEGALES.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MOBILE_ILLUSTRATION1.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MOBILE_ILLUSTRATION2.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MOBILE_ILLUSTRATION3.png", "image/png", NULL,
	},
	{
		HTTP_ALLOW_GET,
                "images/STOCKRECUP_MOBILE_BG.jpg", "image/jpg", NULL,
        },
	{
		HTTP_ALLOW_GET,
		"images/STOCKRECUP_MOBILE_BG_FRAMES.jpg", "image/jpeg", NULL,
	},
	{
		HTTP_ALLOW_GET,
		"favicon.ico", "image/png", NULL,
	},
  {
    HTTP_ALLOW_GET,
    "robots.txt", "text/plain", NULL,
  },
	{
		0,
	},
};

/*
 * Dynamic web site resource, ones which need to be filled with dynamic data
 * or which change according to user input. They are retourned only if errors
 * occur or success happen. They aren't indexable and used in a context where
 * the user use the store/retrieve service, therefore not for read only use
 * of the service.
 */

struct static_entry dyn_entries[] = {
	{
		0,
		"dyn_envoie_succes", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_fichier_existe", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_code_faux", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_code_bon", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
		"dyn_err_nom_fichier_inv", "text/html", "; charset=ISO-8859-1",
	},
	{
		0,
	},
};
