CFLAGS+=-Wall -ltag -lbsdconv -I/usr/local/include -L/usr/local/lib

all:
	$(CC) ${CFLAGS} bsdtagconv.cc -o bsdtagconv
