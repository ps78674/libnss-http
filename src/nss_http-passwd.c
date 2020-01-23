#include "nss_http.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <nss.h>
#include <pwd.h>
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
pack_passwd_struct(json_object *root, struct passwd *result, char *buffer, size_t buflen)
{
    DEBUG_LOG;

    char *next_buf = buffer;
    size_t bufleft = buflen;

    json_object *j_pw_name;
    json_object *j_pw_passwd;
    json_object *j_pw_uid;
    json_object *j_pw_gid;
    json_object *j_pw_gecos;
    json_object *j_pw_dir;
    json_object *j_pw_shell;

    if (json_object_get_type(root) != json_type_object) return -1;

    json_object_object_get_ex(root, "pw_name", &j_pw_name);
    json_object_object_get_ex(root, "pw_passwd", &j_pw_passwd);
    json_object_object_get_ex(root, "pw_uid", &j_pw_uid);
    json_object_object_get_ex(root, "pw_gid", &j_pw_gid);
    json_object_object_get_ex(root, "pw_gecos", &j_pw_gecos);
    json_object_object_get_ex(root, "pw_dir", &j_pw_dir);
    json_object_object_get_ex(root, "pw_shell", &j_pw_shell);

    if (json_object_get_type(j_pw_name) != json_type_string) return -1;
    if (json_object_get_type(j_pw_passwd) != json_type_string) return -1;
    if (json_object_get_type(j_pw_uid) != json_type_int) return -1;
    if (json_object_get_type(j_pw_gid) != json_type_int) return -1;
    if (json_object_get_type(j_pw_gecos) != json_type_string && json_object_get_type(j_pw_gecos) != json_type_null) return -1;
    if (json_object_get_type(j_pw_dir) != json_type_string) return -1;
    if (json_object_get_type(j_pw_shell) != json_type_string) return -1;

    memset(buffer, '\0', buflen);

    if ((bufleft <= json_object_get_string_len(j_pw_name)) || 
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_pw_name)) <= 0)) return -2;
    result->pw_name = next_buf;
    next_buf += strlen(result->pw_name) + 1;
    bufleft  -= strlen(result->pw_name) + 1;

    if ((bufleft <= json_object_get_string_len(j_pw_passwd)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_pw_passwd)) <= 0)) return -2;
    result->pw_passwd = next_buf;
    next_buf += strlen(result->pw_passwd) + 1;
    bufleft  -= strlen(result->pw_passwd) + 1;

    result->pw_uid = json_object_get_int(j_pw_uid);
    result->pw_gid = json_object_get_int(j_pw_gid);

    if (json_object_get_type(j_pw_gecos) == json_type_null)
    {
        if (bufleft <= 1) return -2;
        result->pw_gecos = "\0";
        next_buf += 1;
        bufleft -= 1;
    } else {
        if ((bufleft <= json_object_get_string_len(j_pw_gecos)) ||
           (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_pw_gecos)) <= 0)) return -2;
        result->pw_gecos = next_buf;
        next_buf += strlen(result->pw_gecos) + 1;
        bufleft  -= strlen(result->pw_gecos) + 1;
    }

    if ((bufleft <= json_object_get_string_len(j_pw_dir)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_pw_dir)) <= 0)) return -2;
    result->pw_dir = next_buf;
    next_buf += strlen(result->pw_dir) + 1;
    bufleft  -= strlen(result->pw_dir) + 1;

    if ((bufleft <= json_object_get_string_len(j_pw_shell)) ||
       (snprintf(next_buf, bufleft, "%s", json_object_get_string(j_pw_shell)) <= 0)) return -2;
    result->pw_shell = next_buf;
    next_buf += strlen(result->pw_shell) + 1;
    bufleft  -= strlen(result->pw_shell) + 1;

    return 0;
}

enum nss_status
_nss_http_setpwent_locked(int stayopen)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    genurl(url, "passwd", "");

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

// Called to open the passwd file
enum nss_status
_nss_http_setpwent(int stayopen)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_setpwent_locked(stayopen);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_endpwent_locked(void)
{
    DEBUG_LOG;

    if (ent_json_root){
        json_object_put(ent_json_root);
    }

    ent_json_root = NULL;
    ent_json_idx = 0;
    return NSS_STATUS_SUCCESS;
}

// Called to close the passwd file
enum nss_status
_nss_http_endpwent(void)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_endpwent_locked();
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getpwent_r_locked(struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret = NSS_STATUS_SUCCESS;

    if (ent_json_root == NULL) {
        ret = _nss_http_setpwent_locked(0);
    }

    if (ret != NSS_STATUS_SUCCESS) return ret;

    int pack_result = pack_passwd_struct(
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

// Called to look up next entry in passwd file
enum nss_status
_nss_http_getpwent_r(struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getpwent_r_locked(result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}

// Find a passwd by uid
enum nss_status
_nss_http_getpwuid_r_locked(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    char key[MAX_URLKEY_LEN];
    sprintf(key, "uid=%d", uid);
    genurl(url, "passwd", key);

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

    int pack_result = pack_passwd_struct(json_root, result, buffer, buflen);

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
_nss_http_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getpwuid_r_locked(uid, result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}

enum nss_status
_nss_http_getpwnam_r_locked(const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    char url[MAX_URL_LEN];
    json_object *json_root;

    char key[MAX_URLKEY_LEN];
    sprintf(key, "name=%s", name);
    genurl(url, "passwd", key);

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

    int pack_result = pack_passwd_struct(json_root, result, buffer, buflen);
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

// Find a passwd by name
enum nss_status
_nss_http_getpwnam_r(const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    DEBUG_LOG;

    enum nss_status ret;
    pthread_mutex_lock(&mutex);
    ret = _nss_http_getpwnam_r_locked(name, result, buffer, buflen, errnop);
    pthread_mutex_unlock(&mutex);
    return ret;
}
