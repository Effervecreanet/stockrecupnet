
extern char            guest_download[254 + SRN_PATH_STORE_SIZE];
extern char            guest_download_ok[254 + SRN_PATH_STORE_SIZE];


void *srn_receive_file(void *arg);
void *send_file(void *argsf);
struct res_tolog *srn_handle(int suser, struct request_line *rline,
			struct hdr_nv hdrnv[MAX_HEADERS],
			char *usraddr);
