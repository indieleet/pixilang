NDK = /home/alex/soft/android-ndk
SYSROOT = $(NDK)/platforms/android-9/arch-arm
CXX = $(NDK)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc $(CFLAGS) -Wno-multichar --sysroot=$(SYSROOT)

all:
	$(CXX) -I$(NDK)/platforms/android-9/arch-arm/usr/include -I$(NDK)/sources/android/native_app_glue -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -Os -fPIC -g -c library.cpp
	$(CXX) -shared -Wl,-soname,library.so -llog -landroid -o library.so library.o

clean:
	rm -f *.o