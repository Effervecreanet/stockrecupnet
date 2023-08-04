#include <stdio.h>
#include <stdint.h>

#define HTML_DIRECTORY_ALL      	"./"
#define HTML_DIRECTORY_DESKTOP  	"./html/"
#define HTML_DIRECTORY_MOBILE  		"./html_mobile/"

#define SRN_PATH_STORE			"store/"
#define SRN_PATH_STORE_SIZE		sizeof(SRN_PATH_STORE)

#define SRN_HTTP_ENDPOINT_STORE         "stockage"
#define SRN_HTTP_ENDPOINT_RETRIEVE      "recuperation"

struct static_entry {
	uint64_t allow_method;
	char *resource;
	char *type;
	char *type_opt;
};

extern struct static_entry static_entries[];
extern struct static_entry dyn_entries[];
