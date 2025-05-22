all:
	gcc -Os -fPIC -m64 -g -c library.cpp
	gcc -shared -Wl,-soname,library.so -m64 -lstdc++ -o library.so library.o

clean:
	rm -f *.o