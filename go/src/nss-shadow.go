package main

//#include <shadow.h>
//#include <errno.h>
import "C"

import (
	"bytes"
	"syscall"
	"unsafe"
)

//export setSpEnt
func setSpEnt() nssStatus {
	// DEBUG
	debugPrint("setSpEnt\n")

	shadowEntryIndex = 0
	return nssStatusSuccess
}

//export endSpEnt
func endSpEnt() nssStatus {
	// DEBUG
	debugPrint("endSpEnt\n")

	shadowEntryIndex = 0
	return nssStatusSuccess
}

//export getSpEntR
func getSpEntR(spwd *C.struct_spwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getSpEntR\n")

	if shadowEntryIndex == len(shadowEntries) {
		return nssStatusNotfound
	}

	ret := setCShadow(&shadowEntries[shadowEntryIndex], spwd, buf, buflen, errnop)
	if ret != nssStatusSuccess {
		return ret
	}

	shadowEntryIndex++
	return nssStatusSuccess
}

//export getSpNamR
func getSpNamR(name string, spwd *C.struct_spwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getSpNamR\n")

	for _, s := range shadowEntries {
		if s.Username == name {
			return setCShadow(&s, spwd, buf, buflen, errnop)
		}

	}

	*errnop = C.int(syscall.ENODATA)
	return nssStatusNotfound
}

// set C struct values
func setCShadow(p *shadow, spwd *C.struct_spwd, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("setCShadow\n")

	if len(p.Username)+len(p.Password)+7 > int(buflen) {
		*errnop = C.int(syscall.EAGAIN)
		return nssStatusTryagain
	}

	gobuf := C.GoBytes(unsafe.Pointer(buf), C.int(buflen))
	b := bytes.NewBuffer(gobuf)
	b.Reset()

	spwd.sp_namp = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Username)
	b.WriteByte(0)

	spwd.sp_lstchg = C.long(p.LastChange)
	spwd.sp_min = C.long(p.MinChange)
	spwd.sp_max = C.long(p.MaxChange)
	spwd.sp_warn = C.long(p.PasswordWarn)
	if p.InactiveLockout != nil {
		spwd.sp_inact = C.long(p.InactiveLockout.(int))
	}
	if p.ExpirationDate != nil {
		spwd.sp_expire = C.long(p.ExpirationDate.(int))
	}
	if p.Reserved != nil {
		spwd.sp_flag = C.ulong(p.Reserved.(uint))
	}

	spwd.sp_pwdp = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Password)
	b.WriteByte(0)

	return nssStatusSuccess
}
