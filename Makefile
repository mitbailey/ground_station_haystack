CXX = g++
CPPOBJS = src/main.o src/gs_haystack.o network/network.o
COBJS = modem/src/libuio.o modem/src/adidma.o modem/src/rxmodem.o modem/src/txmodem.o
CXXFLAGS = -I ./include/ -I ./modem/ -I ./modem/include/ -I ./network/ -Wall -pthread
TARGET = haystack.out

all: $(COBJS) $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(COBJS) $(CPPOBJS) -o $(TARGET)
	./$(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	$(RM) *.out
	$(RM) *.o
	$(RM) src/*.o
	$(RM) network/*.o