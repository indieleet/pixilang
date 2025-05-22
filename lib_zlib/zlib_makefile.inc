ZLIB_ORIG = $(ZLIB)zlib/

FINAL_CFLAGS += -I$(ZLIB_ORIG)

OBJS += \
    adler32.o \
    compress.o \
    crc32.o \
    deflate.o \
    infback.o \
    inffast.o \
    inflate.o \
    inftrees.o \
    trees.o \
    uncompr.o \
    zutil.o

ZLIB_DEPS = \
    $(ZLIB_ORIG)crc32.h \
    $(ZLIB_ORIG)deflate.h \
    $(ZLIB_ORIG)gzguts.h \
    $(ZLIB_ORIG)inffast.h \
    $(ZLIB_ORIG)inffixed.h \
    $(ZLIB_ORIG)inflate.h \
    $(ZLIB_ORIG)inftrees.h \
    $(ZLIB_ORIG)trees.h \
    $(ZLIB_ORIG)zconf.h \
    $(ZLIB_ORIG)zlib.h \
    $(ZLIB_ORIG)zutil.h

adler32.o: $(ZLIB_ORIG)adler32.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)adler32.c

compress.o: $(ZLIB_ORIG)compress.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)compress.c

crc32.o: $(ZLIB_ORIG)crc32.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)crc32.c

deflate.o: $(ZLIB_ORIG)deflate.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)deflate.c

infback.o: $(ZLIB_ORIG)infback.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)infback.c

inffast.o: $(ZLIB_ORIG)inffast.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)inffast.c

inflate.o: $(ZLIB_ORIG)inflate.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)inflate.c

inftrees.o: $(ZLIB_ORIG)inftrees.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)inftrees.c

trees.o: $(ZLIB_ORIG)trees.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)trees.c

uncompr.o: $(ZLIB_ORIG)uncompr.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)uncompr.c

zutil.o: $(ZLIB_ORIG)zutil.c $(ZLIB_DEPS)
	$(CC) $(FINAL_CFLAGS) -c $(ZLIB_ORIG)zutil.c
