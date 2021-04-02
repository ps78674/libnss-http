#include "nss_http.h"

#include <unistd.h>
#include <netdb.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

struct config
{
   char httpserver[64];
   char debug[5];
   long timeout;
};

struct MemoryStruct {
  char *data;
  size_t size;
};

struct config conf;

void readconfig(struct config *configstruct)
{
    FILE *f;
    if ((f = fopen (CONFIG_FILE, "r")) == NULL)
    {
        fprintf(stderr, "NSS-HTTP: error opening configuration file '%s'\n", CONFIG_FILE);
        exit(1);
    }

    char line[MAX_CONFLINE_LEN];
    while(fgets(line, sizeof(line), f) != NULL)
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

    fclose(f);
}

void debug_func_name(const char *func)
{
    memset(&conf, '\0', sizeof(conf));
    readconfig(&conf);

    if (strcmp("true", conf.debug) == 0)
        fprintf(stderr, "NSS-HTTP: called function %s \n", func);
}

void getmyhostname(char *hostname)
{
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
    memset(&conf, '\0', sizeof(conf));
    readconfig(&conf);

    char hostname[MAX_HOSTNAME_LEN];
    getmyhostname(hostname);

    if (strlen(key) == 0){
        snprintf(url, MAX_URL_LEN, "%s/%s?hostname=%s", conf.httpserver, type, hostname);
    }
    else if ( strlen(key) != 0){
        snprintf(url, MAX_URL_LEN, "%s/%s?%s&hostname=%s", conf.httpserver, type, key, hostname);
    }
}

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
    DEBUG_LOG;

    struct MemoryStruct *mem = (struct MemoryStruct *)stream;
    size_t realsize = size * nmemb;

    if (realsize < NSS_HTTP_MAX_BUFFER_SIZE) {
        mem->data = realloc(mem->data, mem->size + realsize + 1);
        if (mem->data == NULL) {
            // out of memory
            fprintf(stderr, "NSS-HTTP: not enough memory (realloc returned NULL)\n");
            return 0;
        }
    } else {
        // request data is too large
        fprintf(stderr, "request data is too large\n");
        return 0;
    }

    memcpy(&(mem->data[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

char *nss_http_request(const char *url)
{
    DEBUG_LOG;

    memset(&conf, '\0', sizeof(conf));
    readconfig(&conf);

    CURL *curl = NULL;
    CURLcode result;

    long code;
    struct curl_slist *headers = NULL;
    struct MemoryStruct chunk = { .data = malloc(1), .size = 0 };

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    headers = curl_slist_append(headers, "User-Agent: NSS-HTTP");    

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, conf.timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conf.timeout);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    result = curl_easy_perform(curl);
    if(result != CURLE_OK) {
        fprintf(stderr, "NSS-HTTP: curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
    } else {
        result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if ((result != CURLE_OK) && (code != 200)) {
            fprintf(stderr, "NSS-HTTP: curl_easy_getinfo() failed: result: %s, return code: %ld\n", curl_easy_strerror(result), code);
        };
    };

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();

    return chunk.data;
}
