CXXFLAGS+=-g -Wall -ltag -lbsdconv -I/usr/local/include -L/usr/local/lib

all:
	$(CXX) ${CXXFLAGS} bsdtagconv.cc -o bsdtagconv
