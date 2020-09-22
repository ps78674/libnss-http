INC_LIBJSON_PATH=json-c

CC=gcc
CFLAGS=-Wall -O2 -s -fPIC -std=gnu99 -I/usr/include/$(INC_LIBJSON_PATH)

LD_SONAME=-Wl,-soname,libnss_http.so.2
LIBRARY=libnss_http.so.2.0
LINKS=libnss_http.so.2 libnss_http.so

PREFIX=/usr
LIBDIR=$(DESTDIR)$(PREFIX)/lib
BUILD=build

default: build
build: nss_http

nss_http_build_dir:
	[ -d $(BUILD) ] || mkdir $(BUILD)

nss_http-passwd:
	$(CC) $(CFLAGS) -c src/nss_http-passwd.c -o $(BUILD)/nss_http-passwd.o

nss_http-group:
	$(CC) $(CFLAGS) -c src/nss_http-group.c -o $(BUILD)/nss_http-group.o

nss_http-shadow:
	$(CC) $(CFLAGS) -c src/nss_http-shadow.c -o $(BUILD)/nss_http-shadow.o

nss_http_services: nss_http-passwd nss_http-group nss_http-shadow

nss_http: nss_http_build_dir nss_http_services
	$(CC) $(CFLAGS) -c src/nss_http.c -o $(BUILD)/nss_http.o

	$(CC) -shared $(LD_SONAME) \
        -o $(BUILD)/$(LIBRARY) \
		$(BUILD)/nss_http.o \
		$(BUILD)/nss_http-passwd.o \
		$(BUILD)/nss_http-group.o \
		$(BUILD)/nss_http-shadow.o \
		-l$(INC_LIBJSON_PATH) \
		-lcurl

	strip $(BUILD)/$(LIBRARY)

clean:
	rm -rf $(BUILD)

install:
	[ -d $(LIBDIR) ] || install -d $(LIBDIR)
	install $(BUILD)/$(LIBRARY) $(LIBDIR)
	cd $(LIBDIR); for link in $(LINKS); do ln -sf $(LIBRARY) $$link ; done

.PHONY: clean install nss_http_build_dir nss_http
