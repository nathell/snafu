CXXFLAGS = -g -Os -Wall -fomit-frame-pointer -ffunction-sections -fdata-sections -std=c++11
LDFLAGS  = -lX11 -lXi -Wl,--gc-sections

all: snafu

snafu.o: snafu.cpp
	$(CXX) $(CXXFLAGS) -c -o snafu.o snafu.cpp

snafu: snafu.o
	$(CXX) -o snafu snafu.o $(LDFLAGS)
	strip snafu

clean:
	rm -f snafu snafu.o
