#include <stdint.h>
#include <stdio.h>


#define HTTP_GET  	0x1
#define HTTP_HEAD 	0x2
#define HTTP_POST 	0x3


#define HTTP_ALLOW_GET  0x1
#define HTTP_ALLOW_HEAD 0X2
#define HTTP_ALLOW_POST 0x4

#define RESOURCE_MAX_SIZE 254

#define HEADER_NAME_SIZE	32
#define HEADER_VALUE_SIZE	600
#define HEADER_SIZE		HEADER_NAME_SIZE + HEADER_VALUE_SIZE + 2
#define MAX_HEADERS		30

#define FORM_NAME_SIZE	48	
#define FORM_VALUE_SIZE	224

struct http_allow {
	const unsigned short  httpallow_code;
	const char           *httpallow_str;
	const char           *httpmetho_str;
};


struct request_line {
	uint64_t 	method;
	struct static_entry entry;
	char		*version;
};

struct hdr_nv {
	char	name[HEADER_NAME_SIZE];
	char	value[HEADER_VALUE_SIZE];
	char 	*pname;		/* For server reply only */
	char	*pvalue;	/* For server reply only */
};

struct form_nv {
	char name[FORM_NAME_SIZE];
	char value[FORM_VALUE_SIZE];
};

char *hdr_nv_value(struct hdr_nv nv[MAX_HEADERS], const char *name);
int receive_form_data(int s, char *filename, off_t clen, char boundary[255]);
int get_form_data(int suser, char *filename, off_t clen, char boundary[255]);
int get_www_form(int suser, struct form_nv **formnv, size_t clength);
int get_http_request(int suser, struct request_line *rline,
				struct hdr_nv hdrnv[MAX_HEADERS]);

int rec_boundary(char boundary[255], char *pctype);
int http_parse_length(const char *p, off_t *length);


