#define HTTP_HEADER_ALLOW             "Allow"
#define HTTP_HEADER_CONTENT_LANGUAGE  "Content-Language"
#define HTTP_HEADER_CONTENT_LENGTH    "Content-Length"
#define HTTP_HEADER_DATE              "Date"
#define HTTP_HEADER_HOST              "Host"
#define HTTP_HEADER_EXPIRES           "Expires"
#define HTTP_HEADER_LAST_MODIFIED     "Last-Modified"
#define HTTP_HEADER_SERVER            "Server"
#define HTTP_HEADER_CONTENT_TYPE      "Content-Type"
#define HTTP_HEADER_CONNECTION        "Connection"
#define HTTP_HEADER_IF_MODIFIED_SINCE "If-Modified-Since"
#define HTTP_HEADER_CACHE_CONTROL     "Cache-Control"
#define HTTP_HEADER_VARY              "Vary"
#define HTTP_HEADER_ACCEPT_RANGES     "Accept-Ranges"
#define HTTP_HEADER_CONTENT_RANGE     "Content-Range"
#define HTTP_HEADER_RANGE             "Range"
#define HTTP_HEADER_SET_COOKIE        "Set-Cookie"

#define HTTP_HEADER_CACHE_CONTROL_VALUE  "public, max-age=0"
#define HTTP_HEADER_CACHE_CONTROL_VALUE2 "private"
#define HTTP_HEADER_SERVER_VALUE         "nginx"
#define HTTP_HEADER_ALLOW_VALUE          "GET, HEAD"
#define HTTP_HEADER_CONNECTION_VALUE     "keep-alive"
#define HTTP_HEADER_VARY_VALUE           "User-Agent"
#define HTTP_HEADER_ACCEPT_RANGES_VALUE  "bytes"
#define HTTP_HEADER_CONTENT_LANGUAGE_VALUE "fr-FR"

struct status_line {
        char            *code;
        char            *mess;
        char            *vers;
};

struct response {
        struct status_line      statusline;
        struct hdr_nv           hdrnv[MAX_HEADERS];
        struct static_entry     entry;
        char			local_entry[RESOURCE_MAX_SIZE +
						sizeof("./html_mobile/")];
	char			pad[3];
};

int create_response(struct response *resp, struct hdr_nv hdrnv[MAX_HEADERS]);
char* create_dynamic_response(struct response *resp,
			       struct hdr_nv hdrnv[MAX_HEADERS],
			       char *filename,
			       ...);
void create_header(struct hdr_nv hdrnv[MAX_HEADERS],
		   uint64_t allow,
		   char *type,
                   char *cookie,
                   unsigned short cache);

void create_status_line(struct status_line *sline);
void set_expires(char value[33]);
