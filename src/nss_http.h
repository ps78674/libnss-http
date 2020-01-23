#ifndef NSS_HTTP_H
#define NSS_HTTP_H

#define _GNU_SOURCE

#define NSS_HTTP_MAX_BUFFER_SIZE (2 * 1024 * 1024) // 2 Mb

#define MAX_CONFLINE_LEN 128
#define MAX_HOSTNAME_LEN 64
#define MAX_URL_LEN 256
#define MAX_URLKEY_LEN 64
#define MAX_PPIDNAME_LEN 8
#define MAX_PPIDCOMM_LEN 20

//debug mode
#define DEBUG_LOG debug_print(__FUNCTION__)

//config file location
#define CONFIG_FILE "/etc/nss_http.conf"

// define nss_http.conf key&value model
struct config
{
   char httpserver[64];
   char debug[5];
   long timeout;
};

extern char *nss_http_request(const char *);

extern void readconfig(struct config *conf);
extern void debug_print(const char *func);
extern void genurl(char* url, const char *type, const char *key);

#endif /* NSS_HTTP_H */
