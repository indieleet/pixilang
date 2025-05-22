all:
	gcc -Os -fPIC -m64 -g -c library.cpp
	gcc -dynamiclib -m64 -lstdc++ -o library.dylib library.o

clean:
	rm -f *.o