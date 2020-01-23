#include "nss_http.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <nss.h>
#include <shadow.h>
#include <string.h>

// json-c - path configured with Makefile
// #include <json-c/json.h>
#include <json.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static json_object *ent_json_root = NULL;
static int ent_json_idx = 0;

// -1 Failed to parse
// -2 Buffer too small
static int
pack_shadow_struct(json_object *root, struct spwd *result, char *buffer, size_t buflen)
{
    DEBUG_LOG;

    char *next_buf = buffer;
    size_t bufleft = buflen;

    json_object *j_sp_namp;
    json_object *j_sp_pwdp;
    json_object *j_sp_lstchg;
    json_object *j_sp_min;
    json_object *j_sp_max;
    json_object *j_sp_warn;
    json_object *j_sp_inact;
    json_object *j_sp_expire;
    json_object *j_sp_flag;

    if (json_object_get_type(root) != json_type_object) return -1;

    json_object_object_get_ex(root, "sp_namp", &j_sp_namp);
    json_object_object_get_ex(root, "sp_pwdp", &j_sp_pwdp);
    json_object_object_get_ex(root, "sp_lstchg", &j_sp_lstchg);
    json_object_object_get_ex(root, "sp_min", &j_sp_min);
    json_object_object_get_ex(root, "sp_max", &j_sp_max);
    json_object_object_get_ex(root, "sp_warn", &j_sp_warn);
    json_object_object_get_ex(root, "sp_inact", &j_sp_inact);
    json_object_object_get_ex(root, "sp_expire", &j_sp_expire);
    json_object_object_get_ex(root, "sp_flag", &j_sp_flag);

    if (json_object_get_type(j_sp_namp) != json_type_string) return -1;
    if (json_object_get_type(j_sp_pwdp) != json_type_string) return -1;
    if (json_object_get_type(j_sp_lstchg) != json_type_int) return -1;
    if (json_object_get_type(j_sp_min) != json_type_int) return -1;
    if (json_object_get_type(j_sp_max) != json_type_int) return -1;
    if (json_object_get_type(j_sp_warn) != json_type_int) return -1;
    if (json_object_get_type(j_sp_inact)  != json_type_int && json_object_get_type(j_sp_inact) != json_type_null) return -1;
    if (json_object_get_type(j_sp_expire)  != json_type_int && json_object_get_type(j_sp_expire) != json_type_null) return -1;
    if (json_object_get_type(j_sp_flag)  != json_type_int && json_object_get_type(j_sp_flag) != json_type_null) return -1;

    memset(buffer, '\0', buflen);

    if ((bufleft <= json_object_get_string_len(j_sp_namp)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_sp_namp)) <= 0)) return -2;
    result->sp_namp = next_buf;
    next_buf += strlen(result->sp_namp) + 1;
    bufleft  -= strlen(result->sp_namp) + 1;

    if ((bufleft <= json_object_get_string_len(j_sp_pwdp)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_sp_pwdp)) <= 0)) return -2;
    result->sp_pwdp = next_buf;
    next_buf += strlen(result->sp_pwdp) + 1;
    bufleft  -= strlen(result->sp_pwdp) + 1;

    result->sp_lstchg = json_object_get_int(j_sp_lstchg);
    result->sp_min = json_object_get_int(j_sp_min);
    result->sp_max = json_object_get_int(j_sp_max);
    result->sp_warn = json_object_get_int(j_sp_warn);

    if (json_object_get_type(j_sp_inact) != json_type_null) result->sp_inact = json_object_get_int(j_sp_inact);
    else result->sp_inact = -1;

    if (json_object_get_type(j_sp_expire) != json_type_null) result->sp_expire = json_object_get_int(j_sp_expire);
    else result->sp_expire = -1;

    if (json_object_get_type(j_sp_flag) != json_type_null) result->sp_flag = json_object_get_int(j_sp_flag);
    else result->sp_flag = ~0ul;

    return 0;
}

enum nss_status
_nss_http_setspent_locked(int stayopen)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    genurl(url, "shadow", "");

    char *response = nss_http_request(url);
    if (!response) {
        return NSS_STATUS_UNAVAIL;
    }

    json_root = json_tokener_parse(response);
    if (!json_root) {
        return NSS_STATUS_UNAVAIL;
    }

    if (json_object_get_type(json_root) != json_type_array) {
        json_object_put(json_root);
        return NSS_STATUS_UNAVAIL;
    }

    ent_json_root = json_root;
    ent_json_idx = 0;
    return NSS_STATUS_SUCCESS;
}

// Called to open the shadow file
enum nss_status
_nss_http_setspent(int stayopen)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_setspent_locked(stayopen);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_endspent_locked(void)
{
    DEBUG_LOG;

    if (ent_json_root){
        json_object_put(ent_json_root);
    }

    ent_json_root = NULL;
    ent_json_idx = 0;
    return NSS_STATUS_SUCCESS;
}

// Called to close the shadow file
enum nss_status
_nss_http_endspent(void)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_endspent_locked();
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getspent_r_locked(struct spwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret = NSS_STATUS_SUCCESS;

    if (ent_json_root == NULL) {
        ret = _nss_http_setspent_locked(0);
    }

    if (ret != NSS_STATUS_SUCCESS) return ret;

    int pack_result = pack_shadow_struct(
        json_object_array_get_idx(ent_json_root, ent_json_idx), result, buffer, buflen
    );

    if (pack_result == -1) {
        *errnop = ENOENT;
        return NSS_STATUS_UNAVAIL;
    }

    if (pack_result == -2) {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    // Return notfound when there's nothing else to read.
    if (ent_json_idx >= json_object_array_length(ent_json_root)) {
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    ent_json_idx++;
    return NSS_STATUS_SUCCESS;
}

// Called to look up next entry in shadow file
enum nss_status
_nss_http_getspent_r(struct spwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getspent_r_locked(result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getspnam_r_locked(const char *name, struct spwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    char key[MAX_URLKEY_LEN];
    sprintf(key, "name=%s", name);
    genurl(url, "shadow", key);

    char *response = nss_http_request(url);
    if (!response) {
        *errnop = ENOENT;
        return NSS_STATUS_UNAVAIL;
    }

    json_root = json_tokener_parse(response);
    if (!json_root) {
        *errnop = ENOENT;
        return NSS_STATUS_UNAVAIL;
    }

    int pack_result = pack_shadow_struct(json_root, result, buffer, buflen);
    if (pack_result == -1) {
        json_object_put(json_root);
        *errnop = ENOENT;
        return NSS_STATUS_UNAVAIL;
    }

    if (pack_result == -2) {
        json_object_put(json_root);
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    json_object_put(json_root);
    return NSS_STATUS_SUCCESS;
}

// Find a shadow by name
enum nss_status
_nss_http_getspnam_r(const char *name, struct spwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getspnam_r_locked(name, result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}
