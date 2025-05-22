all:
	gcc -Os -fPIC -m32 -g -c library.cpp
	gcc -shared -Wl,-soname,library.so -m32 -lstdc++ -o library.so library.o

clean:
	rm -f *.o