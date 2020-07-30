package main

//#include <grp.h>
//#include <errno.h>
import "C"

import (
	"bytes"
	"syscall"
	"unsafe"
)

//export setGrEnt
func setGrEnt(stayopen C.int) int32 {
	// DEBUG
	debugPrint("setGrEnt\n")

	groupEntryIndex = 0
	return nssStatusSuccess
}

//export endGrEnt
func endGrEnt() nssStatus {
	// DEBUG
	debugPrint("endGrEnt\n")

	groupEntryIndex = 0
	return nssStatusSuccess
}

//export getGrEntR
func getGrEntR(grp *C.struct_group, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getGrEntR\n")

	if groupEntryIndex == len(groupEntries) {
		return nssStatusNotfound
	}

	ret := setCGroup(&groupEntries[groupEntryIndex], grp, buf, buflen, errnop)
	if ret != nssStatusSuccess {
		return ret
	}

	groupEntryIndex++
	return nssStatusSuccess
}

//export getGrNamR
func getGrNamR(name string, grp *C.struct_group, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getGrNamR\n")

	for _, g := range groupEntries {
		if g.Groupname == name {
			return setCGroup(&g, grp, buf, buflen, errnop)
		}
	}

	*errnop = C.int(syscall.ENODATA)
	return nssStatusNotfound
}

//export getGrGidR
func getGrGidR(gid uint, grp *C.struct_group, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("getGrGidR\n")

	for _, g := range groupEntries {
		if g.GID == gid {
			return setCGroup(&g, grp, buf, buflen, errnop)
		}

	}

	*errnop = C.int(syscall.ENODATA)
	return nssStatusNotfound
}

// set C struct values
func setCGroup(p *group, grp *C.struct_group, buf *C.char, buflen C.size_t, errnop *C.int) nssStatus {
	// DEBUG
	debugPrint("setCGroup\n")

	if len(p.Groupname)+len(p.Password)+5 > int(buflen) {
		*errnop = C.int(syscall.EAGAIN)
		return nssStatusTryagain
	}

	gobuf := C.GoBytes(unsafe.Pointer(buf), C.int(buflen))
	b := bytes.NewBuffer(gobuf)
	b.Reset()

	grp.gr_name = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString(p.Groupname)
	b.WriteByte(0)

	grp.gr_passwd = (*C.char)(unsafe.Pointer(&gobuf[b.Len()]))
	b.WriteString("x")
	b.WriteByte(0)

	grp.gr_gid = C.uint(p.GID)

	cArr := make([]*C.char, len(p.Members)+1)
	for i, name := range p.Members {
		cArr[i] = C.CString(name)
	}

	grp.gr_mem = &cArr[0]

	return nssStatusSuccess
}
