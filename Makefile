CXXFLAGS = -g -Os -fomit-frame-pointer -ffunction-sections -fdata-sections
LDFLAGS  = -lX11 -Wl,--gc-sections

all: snafu

snafu.o: snafu.cpp
	$(CXX) $(CXXFLAGS) -c -o snafu.o snafu.cpp

snafu: snafu.o
	$(CXX) -o snafu snafu.o $(LDFLAGS)

clean:
	rm -f snafu snafu.o
