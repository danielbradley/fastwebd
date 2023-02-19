arch := $(shell uname)
cpu  := $(shell uname -m)

all: dir obj bin

dir:
	mkdir -p _bin/$(arch)-$(cpu) _obj/$(arch)-$(cpu)

obj:
	gcc -g -o _obj/$(arch)-$(cpu)/IO_sendFile.o    -Isource/include -c source/src/libbase/$(arch)/IO_sendFile.c
	gcc -g -o _obj/$(arch)-$(cpu)/libbase.o        -Isource/include -c source/src/libbase/libbase.c
	gcc -g -o _obj/$(arch)-$(cpu)/libhttp.o        -Isource/include -c source/src/libhttp/libhttp.c
	gcc -g -o _obj/$(arch)-$(cpu)/libhttpserver.o  -Isource/include -c source/src/libhttpserver/libhttpserver.c
	gcc -g -o _obj/$(arch)-$(cpu)/main.o           -Isource/include -c source/src/main.c

bin:
	gcc -g -o _bin/$(arch)-$(cpu)/fastwebd _obj/$(arch)-$(cpu)/*.o

run:
	cd share/www; ../../_bin/$(arch)-$(cpu)/fastwebd

clean:
	rm -rf _bin _obj
