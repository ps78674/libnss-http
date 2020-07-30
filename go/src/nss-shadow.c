#include "nss.h"
#include "_cgo_export.h"
#include <string.h>

enum nss_status _nss_http_setspent (void){
	return setSpEnt();
}

enum nss_status _nss_http_endspent (void){
	return endSpEnt();
}

enum nss_status _nss_http_getspent_r (struct spwd *result, char *buffer, size_t buflen, int *errnop){
	return getSpEntR(result, buffer, buflen, errnop);
}

enum nss_status _nss_http_getspnam_r (const char *name, struct spwd *result, char *buffer, size_t buflen, int *errnop){
	GoString goname = {name, strlen(name) };
	return getSpNamR(goname, result, buffer, buflen, errnop);
}
