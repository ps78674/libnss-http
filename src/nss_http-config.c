#include "nss_http.h"

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

void readconfig(struct config *configstruct)
{
    memset(configstruct->httpserver, '\0', sizeof(configstruct->httpserver));
    memset(configstruct->debug, '\0', sizeof(configstruct->debug));

    FILE *file = fopen (CONFIG_FILE, "r");
    if (file != NULL)
    {
        char line[MAX_CONFLINE_LEN];
        while(fgets(line, sizeof(line), file) != NULL)
        {
            if(line[0] == '#') {
                continue;
            }

            if ((sscanf(line, "HTTPSERVER=%s\n", configstruct->httpserver) != 0)
                || (sscanf(line, "DEBUG=%s\n", configstruct->debug) != 0)
                || (sscanf(line, "TIMEOUT=%ld\n", &configstruct->timeout) != 0))
            {
                continue;
            }
        }

        fclose(file);
    }
}

void getmyhostname(char *hostname)
{
    memset(hostname, '\0', sizeof(hostname));
    gethostname(hostname, MAX_HOSTNAME_LEN);

    struct addrinfo hints={ .ai_family=AF_UNSPEC, .ai_flags=AI_CANONNAME };
    struct addrinfo *res=NULL;

    if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
        snprintf(hostname, MAX_HOSTNAME_LEN, "%s", res->ai_canonname);
        freeaddrinfo(res);
    }
}

void genurl(char* url, const char *type, const char *key)
{
    struct config conf;
    char hostname[MAX_HOSTNAME_LEN];

    readconfig(&conf);
    getmyhostname(hostname);
    
    if (strlen(key) == 0){
        snprintf(url, MAX_URL_LEN, "%s/%s?hostname=%s", conf.httpserver, type, hostname);
    }
    else if ( strlen(key) != 0){
        snprintf(url, MAX_URL_LEN, "%s/%s?%s&hostname=%s", conf.httpserver, type, key, hostname);
    }
}

void debug_print(const char *func)
{
    struct config conf;
    readconfig(&conf);
    if (strcmp("true", conf.debug) == 0)
        fprintf(stderr, "NSS DEBUG: Called %s \n", func);
}
