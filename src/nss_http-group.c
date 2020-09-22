#include "nss_http.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <nss.h>
#include <grp.h>
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
pack_group_struct(json_object *root, struct group *result, char *buffer, size_t buflen)
{
    DEBUG_LOG;

    char *next_buf = buffer;
    size_t bufleft = buflen;

    json_object *j_member;
    json_object *j_gr_name;
    json_object *j_gr_passwd;
    json_object *j_gr_gid;
    json_object *j_gr_mem;

    if (json_object_get_type(root) != json_type_object) return -1;

    json_object_object_get_ex(root, "gr_name", &j_gr_name);
    json_object_object_get_ex(root, "gr_passwd", &j_gr_passwd);
    json_object_object_get_ex(root, "gr_gid", &j_gr_gid);
    json_object_object_get_ex(root, "gr_mem", &j_gr_mem);

    if (json_object_get_type(j_gr_name) != json_type_string) return -1;
    if (json_object_get_type(j_gr_passwd) != json_type_string) return -1;
    if (json_object_get_type(j_gr_gid) != json_type_int) return -1;
    if (json_object_get_type(j_gr_mem) != json_type_array) return -1;

    memset(buffer, '\0', buflen);

    if ((int)bufleft <= json_object_get_string_len(j_gr_name)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_gr_name)) <= 0)) return -2;
    result->gr_name = next_buf;
    next_buf += strlen(result->gr_name) + 1;
    bufleft  -= strlen(result->gr_name) + 1;

    if ((int)bufleft <= json_object_get_string_len(j_gr_passwd)) || 
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_gr_passwd)) <= 0)) return -2;
    result->gr_passwd = next_buf;
    next_buf += strlen(result->gr_passwd) + 1;
    bufleft  -= strlen(result->gr_passwd) + 1;

    result->gr_gid = json_object_get_int(j_gr_gid);

    // Carve off some space for array of members.
    result->gr_mem = (char **)next_buf;
    next_buf += (json_object_array_length(j_gr_mem) + 1) * sizeof(char *);
    bufleft  -= (json_object_array_length(j_gr_mem) + 1) * sizeof(char *);

    for(int i = 0; i < json_object_array_length(j_gr_mem); i++)
    {
        j_member = json_object_array_get_idx(j_gr_mem, i);
        if (json_object_get_type(j_member) != json_type_string) return -1;

        if ((int)bufleft < json_object_get_string_len(j_member)) || 
           (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_member)) <= 0)) return -2;
        result->gr_mem[i] = next_buf;
        next_buf += strlen(result->gr_mem[i]) + 1;
        bufleft  -= strlen(result->gr_mem[i]) + 1;
    }

    return 0;
}

enum nss_status
_nss_http_setgrent_locked(int stayopen)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    genurl(url, "group", "");

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

// Called to open the group file
enum nss_status
_nss_http_setgrent(int stayopen)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_setgrent_locked(stayopen);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_endgrent_locked(void)
{
    DEBUG_LOG;

    if (ent_json_root){
        json_object_put(ent_json_root);
    }

    ent_json_root = NULL;
    ent_json_idx = 0;
    return NSS_STATUS_SUCCESS;
}

// Called to close the group file
enum nss_status
_nss_http_endgrent(void)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_endgrent_locked();
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getgrent_r_locked(struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret = NSS_STATUS_SUCCESS;

    if (ent_json_root == NULL) {
        ret = _nss_http_setgrent_locked(0);
    }

    if (ret != NSS_STATUS_SUCCESS) return ret;

    int pack_result = pack_group_struct(
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

// Called to look up next entry in group file
enum nss_status
_nss_http_getgrent_r(struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getgrent_r_locked(result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}

// Find a group by gid
enum nss_status
_nss_http_getgrgid_r_locked(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    char key[MAX_URLKEY_LEN];
    sprintf(key, "gid=%d", gid);
    genurl(url, "group", key);


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

    int pack_result = pack_group_struct(json_root, result, buffer, buflen);
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

enum nss_status
_nss_http_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getgrgid_r_locked(gid, result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getgrnam_r_locked(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    char key[MAX_URLKEY_LEN];
    sprintf(key, "name=%s", name);
    genurl(url, "group", key);

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

    int pack_result = pack_group_struct(json_root, result, buffer, buflen);

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

// Find a group by name
enum nss_status
_nss_http_getgrnam_r(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getgrnam_r_locked(name, result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}
