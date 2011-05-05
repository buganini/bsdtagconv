CXXFLAGS+=-Wall -lid3 -ltag -lbsdconv -I/usr/local/include -L/usr/local/lib

all:
	$(CXX) ${CXXFLAGS} fileref.cpp bsdtagconv.cc -o bsdtagconv

clean:
	rm -rf bsdtagconv
