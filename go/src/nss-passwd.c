#include "nss.h"
#include "_cgo_export.h"
#include <string.h>

enum nss_status _nss_http_setpwent (int stayopen) {
	return setPwEnt();
}

enum nss_status _nss_http_endpwent (void) {
	return endPwEnt();
}

enum nss_status _nss_http_getpwent_r (struct passwd *p, char *buf, size_t len, int *errnop) {
	return getPwEntR(p, buf, len, errnop);
}

enum nss_status _nss_http_getpwnam_r (const char *name, struct passwd *p, char *buf, size_t len, int *errnop) {
	GoString goname = {name, strlen(name) };
	return getPwNamR(goname, p, buf, len, errnop);
}

enum nss_status _nss_http_getpwuid_r (uid_t uid, struct passwd *p, char *buf, size_t len, int *errnop) {
	return getPwUID(uid, p, buf, len, errnop);
}
