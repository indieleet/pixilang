FINAL_CFLAGS += -I$(GIF)
GIF_CFLAGS = 

OBJS += \
    dgif_lib.o \
    egif_lib.o \
    gif_err.o \
    gif_hash.o \
    gifalloc.o

GIF_DEPS = \
    $(GIF)gif_hash.h \
    $(GIF)gif_lib.h \
    $(GIF)gif_lib_private.h

dgif_lib.o: $(GIF)dgif_lib.c $(GIF_DEPS)
	$(CC) $(FINAL_CFLAGS) $(GIF_CFLAGS) -c $(GIF)dgif_lib.c

egif_lib.o: $(GIF)egif_lib.c $(GIF_DEPS)
	$(CC) $(FINAL_CFLAGS) $(GIF_CFLAGS) -c $(GIF)egif_lib.c

gif_err.o: $(GIF)gif_err.c $(GIF_DEPS)
	$(CC) $(FINAL_CFLAGS) $(GIF_CFLAGS) -c $(GIF)gif_err.c

gif_hash.o: $(GIF)gif_hash.c $(GIF_DEPS)
	$(CC) $(FINAL_CFLAGS) $(GIF_CFLAGS) -c $(GIF)gif_hash.c

gifalloc.o: $(GIF)gifalloc.c $(GIF_DEPS)
	$(CC) $(FINAL_CFLAGS) $(GIF_CFLAGS) -c $(GIF)gifalloc.c
