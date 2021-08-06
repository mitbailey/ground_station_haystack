CPP = g++
C = gcc
CPPOBJS = src/main.o src/gs_haystack.o network/network.o
COBJS = modem/src/libuio.o modem/src/adidma.o modem/src/rxmodem.o modem/src/txmodem.o adf4355/adf4355.o spibus/spibus.o gpiodev/gpiodev.o
CXXFLAGS = -I ./ -I ./include/ -I ./modem/ -I ./modem/include/ -I ./network/ -I ./adf4355/ -I ./spibus/ -Wall -pthread
TARGET = haystack.out

all: $(COBJS) $(CPPOBJS)
	$(CPP) $(CXXFLAGS) $(COBJS) $(CPPOBJS) -o $(TARGET)
	./$(TARGET)

%.o: %.cpp
	$(CPP) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	$(C) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) *.out
	$(RM) *.o
	$(RM) src/*.o
	$(RM) network/*.o
	$(RM) adf4355/*.o
	$(RM) gpiodev/*.o
	$(RM) spibus/*.o