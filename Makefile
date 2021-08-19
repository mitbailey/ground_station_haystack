CXX = g++
CC = gcc
CPPOBJS = src/main.o src/gs_haystack.o network/network.o
COBJS = modem/src/libuio.o modem/src/libiio.o modem/src/adidma.o modem/src/rxmodem.o modem/src/txmodem.o adf4355/adf4355.o spibus/spibus.o gpiodev/gpiodev.o
EDCXXFLAGS = $(CXXFLAGS) -I ./ -I ./include/ -I ./modem/ -I ./modem/include/ -I ./network/ -I ./adf4355/ -I ./spibus/ -Wall -pthread -std=c++17 -DGSNID=\"haystack\"
EDCFLAGS = $(CFLAGS) -I ./ -I ./include/ -I ./modem/ -I ./modem/include/ -I ./network/ -I ./adf4355/ -I ./spibus/ -Wall -pthread -std=gnu11 -DADIDMA_NOIRQ
TARGET = haystack.out
EDLDFLAGS = $(LDFLAGS) -lpthread -liio

all: $(COBJS) $(CPPOBJS)
	$(CXX) $(COBJS) $(CPPOBJS) -o $(TARGET) $(EDLDFLAGS)
	sudo ./$(TARGET)

%.o: %.cpp
	$(CXX) $(EDCXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(EDCFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) *.out
	$(RM) *.o
	$(RM) src/*.o
	$(RM) network/*.o
	$(RM) adf4355/*.o
	$(RM) gpiodev/*.o
	$(RM) spibus/*.o
	$(RM) modem/src/*.o
	$(RM) rxdata*.bin