all:
	x86_64-w64-mingw32-gcc -m64 -Os -g -c library.cpp
	x86_64-w64-mingw32-gcc -shared -Wl,--kill-at -o library.dll library.o

clean:
	rm -f *.o