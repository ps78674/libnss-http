package main

//#include <pwd.h>
//#include <errno.h>
import "C"

import (
	"bytes"
	"syscall"
	"unsafe"
)

//export setPwEnt
func setPwEnt() nssStatus {
	// DEBUG
	debugPrint("setPwEnt\n")

	passwdEntryIndex = 0
	return nssStatusSuccess
}

//export endPwEnt
func endPwEnt() nssStatus {
	// DEBUG
	debugPrint("endPwEnt\n")

	passwdEntryIndex = 0
	return nssStatusSuccess
}

//export getPwEntR
func getPwEntR(passwd *C.struct_passwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getPwEntR\n")

	if passwdEntryIndex == len(passwdEntries) {
		return nssStatusNotfound
	}

	ret := setCPasswd(&passwdEntries[passwdEntryIndex], passwd, buf, buflen, errnop)
	if ret != nssStatusSuccess {
		return ret
	}

	passwdEntryIndex++
	return nssStatusSuccess
}

//export getPwNamR
func getPwNamR(name string, pwd *C.struct_passwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getPwNamR\n")

	for _, p := range passwdEntries {
		if p.Username == name {
			return setCPasswd(&p, pwd, buf, buflen, errnop)
		}
	}

	*errnop = C.int(syscall.ENODATA)
	return nssStatusNotfound
}

//export getPwUID
func getPwUID(uid uint, pwd *C.struct_passwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getPwUID\n")

	for _, p := range passwdEntries {
		if p.UID == uid {
			return setCPasswd(&p, pwd, buf, buflen, errnop)
		}
	}

	*errnop = C.int(syscall.ENODATA)
	return nssStatusNotfound
}

// set C struct values
func setCPasswd(p *passwd, pwd *C.struct_passwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("setCPasswd\n")

	if len(p.Username)+len(p.Password)+len(p.Gecos)+len(p.Dir)+len(p.Shell)+5 > int(buflen) {
		*errnop = C.int(syscall.EAGAIN)
		return nssStatusTryagain
	}

	gobuf := C.GoBytes(unsafe.Pointer(buf), C.int(buflen))
	b := bytes.NewBuffer(gobuf)
	b.Reset()

	pwd.pw_name = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Username)
	b.WriteByte(0)

	pwd.pw_passwd = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Password)
	b.WriteByte(0)

	pwd.pw_gecos = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Gecos)
	b.WriteByte(0)

	pwd.pw_dir = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Dir)
	b.WriteByte(0)

	pwd.pw_shell = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Shell)
	b.WriteByte(0)

	pwd.pw_uid = C.uint(p.UID)
	pwd.pw_gid = C.uint(p.GID)

	return nssStatusSuccess
}
