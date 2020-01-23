#include "nss_http.h"

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

struct MemoryStruct {
  char *data;
  size_t size;
};

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
    DEBUG_LOG;

    struct MemoryStruct *mem = (struct MemoryStruct *)stream;
    size_t realsize = size * nmemb;

    if (realsize < NSS_HTTP_MAX_BUFFER_SIZE) {
        mem->data = realloc(mem->data, mem->size + realsize + 1);
        if (mem->data == NULL) {
            /* out of memory! */
            fprintf(stderr, "not enough memory (realloc returned NULL)\n");
            return 0;
        }
    } else {
        // Request data is too large.
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

    CURL *curl = NULL;
    CURLcode result;
    long code;
    struct curl_slist *headers = NULL;
    struct MemoryStruct chunk = { .data = malloc(1), .size = 0 };
    struct config conf;

    readconfig(&conf);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    headers = curl_slist_append(headers, "User-Agent: NSS-HTTP");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conf.timeout);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    result = curl_easy_perform(curl);
    if(result != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
    } else {
        result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if ((result != CURLE_OK) && (code != 200)) {
            fprintf(stderr, "curl_easy_getinfo() failed: result: %s, return code: %ld\n", curl_easy_strerror(result), code);
        };
    };

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    curl_global_cleanup();

    return chunk.data;
}
