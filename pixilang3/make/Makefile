APPNAME = pixilang

BUILD_TYPE = Release
MAKE_WITH_SSE_VER = sse3
#MAKE_WITH_OBJDUMP = true

##
## Used libraries
##

ROOT = ../../
SUNDOG = $(ROOT)lib_sundog/
JPEG = $(ROOT)lib_jpeg/
PNG = $(ROOT)lib_png/
GIF = $(ROOT)lib_gif/
ZLIB = $(ROOT)lib_zlib/
DSP = $(ROOT)lib_dsp/
SUNVOX = $(ROOT)lib_sunvox/
VORBIS = $(ROOT)lib_vorbis/
MP3 = $(ROOT)lib_mp3/
FLAC = $(ROOT)lib_flac/
PIXILANG = $(ROOT)lib_pixilang/

include $(SUNDOG)sundog_makefile.inc
include $(JPEG)jpeg_makefile.inc
include $(PNG)png_makefile.inc
include $(GIF)gif_makefile.inc
include $(ZLIB)zlib_makefile.inc
include $(DSP)dsp_makefile.inc
include $(SUNVOX)sunvox_makefile.inc
include $(VORBIS)vorbis_makefile.inc
include $(MP3)mp3_makefile.inc
include $(FLAC)flac_makefile.inc
include $(PIXILANG)pixilang_makefile.inc

##
## Target configuration
##

ifneq (,$(findstring linux,$(TARGET_OS)))
ifeq ($(TARGET_OS),$(filter $(TARGET_OS),linux_sdl))
    LIBS = -lX11
    SUNDOG_DEFINES = \
	-D$(COLOR_TYPE) \
	-DDIRECTDRAW \
	-DNOVCAP \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../bin/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengl)
    LIBS = -lX11 -lGL -lXext
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../bin/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengles)
    LIBS = -lX11 -lEGL -lGLESv2
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DOPENGLES \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../bin/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_x11_opengles_raspberry_pi)
    LIBS = -lbcm_host -lvcos -lvchiq_arm -lX11 -lbrcmGLESv2 -lbrcmEGL
    SUNDOG_DEFINES = \
	-DX11 \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE) \
	-DJACK_AUDIO \
	-DJACK_INPUT
install:
	mv -f $(APPNAME) ../bin/linux_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX)
endif
ifeq ($(TARGET_OS),linux_android16)
    LIBS = -lEGL -lGLESv2
    SUNDOG_DEFINES = \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-D$(COLOR_TYPE) \
	-DFREE_VERSION \
	-DPKGNAME="\"nightradio/$(APPNAME)\""
dump:
	$(OBJDUMP) -S $(APPNAME).so >$(APPNAME).asmdump
install:
	mv -f $(APPNAME).so ./android_project/app/src/main/jniLibs/$(TARGET_ABI)/libsundog.so
endif #android
endif #linux
ifeq ($(TARGET_OS),windows_opengl)
    LIBS = -lgdi32 -lopengl32
    SUNDOG_DEFINES = \
	-DOPENGL \
	-DGLNORETAIN \
	-DGLZBUF \
	-DNOVCAP \
	-D$(COLOR_TYPE)
install:
	mv -f $(APPNAME).exe ../bin/windows_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX).exe
endif
ifeq ($(TARGET_OS),windows)
    LIBS = -lgdi32
    SUNDOG_DEFINES = \
	-DGDI \
	-DNOVCAP \
	-D$(COLOR_TYPE)
install:
	mv -f $(APPNAME).exe ../bin/windows_$(TARGET_ARCH)$(ARCH_SUFFIX)/$(APPNAME)$(APPNAME_SUFFIX).exe
endif
ifeq ($(TARGET_OS),windowsce)
    LIBS =
    SUNDOG_DEFINES = \
	-D$(COLOR_TYPE) \
	-DDIRECTDRAW \
	-DGDI \
	-DNOVCAP \
	-DSHOW_PIXILANG_MENU
install:
	mv -f $(APPNAME).exe ../bin/wince
endif
SUNDOG_DEFINES += -D$(STYPE) -DSUNVOX_LIB

##
## Object files
##

OBJS += app.o app_info.o

##
## Rules
##

app.o: ../main/app.cpp $(SD_DEPS)
	$(CXX) $(FINAL_CFLAGS) -c ../main/app.cpp

app_info.o: ../main/app_info.cpp $(SD_DEPS)
	$(CXX) $(FINAL_CFLAGS) -c ../main/app_info.cpp
