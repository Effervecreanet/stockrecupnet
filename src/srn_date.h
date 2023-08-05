#define LOG_DATE_NOW_SIZE sizeof("[18/Feb/2000:13:33:37 -0600]")
extern char     gmdate[33];
extern char     gmdatelog[33];

void set_gmdate(void);
void set_log_date_now(void);
