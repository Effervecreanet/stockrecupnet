#include <netinet/in.h>

ssize_t send_data(int suser, char *data, size_t length);
ssize_t recv_data(int suser, char *data, size_t length);
int bind_socket(int *s, struct sockaddr_in *sin);
int create_socket(int *s);

