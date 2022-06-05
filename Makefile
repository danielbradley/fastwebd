
all: _dir _obj _bin

_dir:
	mkdir -p bin obj

_obj:
	gcc   -o obj/libbase.o       -Isource/include -c source/src/libbase/libbase.c
	gcc   -o obj/libadt.o        -Isource/include -c source/src/libadt/libadt.c
	gcc   -o obj/libhttp.o       -Isource/include -c source/src/libhttp/libhttp.c
	gcc   -o obj/libhttpserver.o -Isource/include -c source/src/libhttpserver/libhttpserver.c
	gcc   -o obj/main.o          -Isource/include -c source/src/main.c

_bin:
	gcc   -o bin/fastwebd obj/{libbase,libadt,libhttp,libhttpserver,main}.o

run:
	cd share/www; ../../bin/fastwebd

clean:
	rm -rf bin obj
