all:
	i686-w64-mingw32-gcc -Os -g -c library.cpp
	i686-w64-mingw32-gcc -shared -Wl,--kill-at -o library.dll library.o

clean:
	rm -f *.o