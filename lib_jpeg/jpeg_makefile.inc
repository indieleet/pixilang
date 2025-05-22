FINAL_CFLAGS += -I$(JPEG)

OBJS += \
    jpgd.o \
    jpge.o

jpgd.o: $(JPEG)jpgd.cpp $(JPEG)jpgd.h $(SD_DEPS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpgd.cpp

jpge.o: $(JPEG)jpge.cpp $(JPEG)jpge.h $(SD_DEPS)
	$(CXX) $(FINAL_CFLAGS) -c $(JPEG)jpge.cpp
