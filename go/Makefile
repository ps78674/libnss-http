VERSION =$(shell cat VERSION | cut -d- -f1)
LD_FLAGS='-s -w'
CGO_CFLAGS='-g -O2'

LIBRARY=libnss_http.so.$(VERSION)

PREFIX=/usr
LIBARCHDIR=/lib
LIBDIR=$(DESTDIR)$(PREFIX)$(LIBARCHDIR)
ETCDIR=$(DESTDIR)/etc
BUILDDIR=./build

default: build

clean: 
	rm -rf $(BUILDDIR)
build: 
	mkdir $(BUILDDIR)
	CGO_CFLAGS=$(CGO_CFLAGS) go build -ldflags=$(LD_FLAGS) --buildmode=c-shared -o $(BUILDDIR)/$(LIBRARY) ./src
install: build
	[ -d $(LIBDIR) ] || install -d $(LIBDIR)
	[ -d $(ETCDIR) ] || install -d $(ETCDIR)
	install $(BUILDDIR)/$(LIBRARY) $(LIBDIR)
	ln -sf $(LIBRARY) $(LIBDIR)/libnss_http.so.2
	ln -sf libnss_http.so.2 $(LIBDIR)/libnss_http.so
	[ -f $(ETCDIR)/nss_http.conf ] || install -m 0644 conf/nss_http.conf $(ETCDIR)

.DEFAULT_GOAL = build
