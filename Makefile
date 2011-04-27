CXXFLAGS+=-Wall -ltag -lbsdconv -I/usr/local/include -L/usr/local/lib

all:
	$(CC) ${CXXFLAGS} bsdtagconv.cc -o bsdtagconv
