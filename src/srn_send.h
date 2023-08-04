int send_status_line(int suser, struct status_line *sline);
int static_send_header(int suser, struct hdr_nv hdrnv[MAX_HEADERS]);
int static_send_reply(int suser, struct response *resp);
int dynamic_send_reply(int suser, struct response *resp, char *content);
