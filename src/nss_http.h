#ifndef NSS_HTTP_H
#define NSS_HTTP_H

#define _GNU_SOURCE

#define NSS_HTTP_MAX_BUFFER_SIZE (10 * 1024 * 1024) // 10 Mb

#define MAX_CONFLINE_LEN 128
#define MAX_HOSTNAME_LEN 128
#define MAX_URL_LEN 256
#define MAX_URLKEY_LEN 128

//debug mode
#define DEBUG_LOG debug_func_name(__FUNCTION__)

//config file location
#define CONFIG_FILE "/etc/nss_http.conf"

extern char *nss_http_request(const char *);

extern void debug_func_name(const char *func);
extern void genurl(char* url, const char *type, const char *key);

#endif /* NSS_HTTP_H */
