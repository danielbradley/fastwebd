arch    := $(shell uname)
cpu     := $(shell uname -m)
version := "1.5

all: dir obj bin

dir:
	mkdir -p _bin/$(arch)-$(cpu) _obj/$(arch)-$(cpu)

obj:
	gcc -g -o _obj/$(arch)-$(cpu)/IO_sendFile.o            $(includes) -c source/src/libbase/$(arch)/IO_sendFile.c
	gcc -g -o _obj/$(arch)-$(cpu)/syslog.o                 $(includes) -c source/src/libbase/SysLog.c
	gcc -g -o _obj/$(arch)-$(cpu)/libbase.o                $(includes) -c source/src/libbase/libbase.c
	gcc -g -o _obj/$(arch)-$(cpu)/libhttp.o                $(includes) -c source/src/libhttp/libhttp.c
	gcc -g -o _obj/$(arch)-$(cpu)/libhttpserver.o          $(includes) -c source/src/libhttpserver/libhttpserver.c
	gcc -g -o _obj/$(arch)-$(cpu)/main.o                   $(includes) -c source/src/main.c

bin:
	gcc -g -o _bin/$(arch)-$(cpu)/fastwebd _obj/$(arch)-$(cpu)/*.o

tar:
	mkdir -p _tar
	cd ..; tar --disable-copyfile --no-xattrs --exclude=".*" --exclude="_*" -jcvf fastwebd/_tar/fastwebd-$(version).tar.bz2 fastwebd/{Makefile,source}

run:
	cd share/www; ../../_bin/$(arch)-$(cpu)/fastwebd

clean:
	rm -rf _bin _obj _tar
