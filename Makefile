
all: _dir _obj _bin

_dir:
	mkdir -p bin obj

_obj:
	gcc -g -o obj/libbase.o        -Isource/include -c source/src/libbase/libbase.c
	gcc -g -o obj/libhttp.o        -Isource/include -c source/src/libhttp/libhttp.c
	gcc -g -o obj/libhttpserver.o  -Isource/include -c source/src/libhttpserver/libhttpserver.c
	gcc -g -o obj/main.o           -Isource/include -c source/src/main.c

_bin:
	gcc -g -o bin/fastwebd obj/{libbase,libhttp,libhttpserver,main}.o

run:
	cd share/www; ../../bin/fastwebd

clean:
	rm -rf bin obj
