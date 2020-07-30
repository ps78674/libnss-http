#include "nss.h"
#include "_cgo_export.h"
#include <string.h>
#include <grp.h>

enum nss_status _nss_http_setgrent (int stayopen) {
	return setGrEnt(stayopen);
}

enum nss_status _nss_http_endgrent (void) {
	return endGrEnt();
}

enum nss_status _nss_http_getgrent_r (struct group *result, char *buffer, size_t buflen, int *errnop) {
	return getGrEntR(result, buffer, buflen, errnop);
}

enum nss_status _nss_http_getgrnam_r (const char *name, struct group *grp, char *buffer, size_t buflen, int *errnop) {
	GoString goname = {name, strlen(name) };
	return getGrNamR(goname, grp, buffer, buflen, errnop);
}

enum nss_status _nss_http_getgrgid_r (gid_t gid, struct group *grp, char *buffer, size_t buflen, int *errnop) {
	return getGrGidR(gid, grp, buffer, buflen, errnop);
}
