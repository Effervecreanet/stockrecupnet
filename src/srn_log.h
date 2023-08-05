#define PATH_LOG "./logs/"
#define HTTP_STRLOG_STARTING      "Starting StockRecupNET with following domain name: "
#define HTTP_STRLOG_STARTING_ADDR "This file logs network and http events relative to address: "
#define SRN_STRLOG_STARTING	  "Starting StockrecupNET, this file logs srn events like wrong pin, successful retrieve and so on"
extern FILE     *fp_log;

int open_http_log(const char *ipstr);
int open_srn_log(void);
void init_intro_http_log(char *wwwhostname, char *addrstr);
void init_intro_srn_log(void);
