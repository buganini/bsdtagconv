CXXFLAGS+=-g -Wall -lbsdconv -Itaglib/build/temp/include -I/usr/local/include -L/usr/local/lib

all:
	cd taglib && \
	mkdir -p build && \
	cd build && \
	mkdir -p temp && \
	cmake -DCMAKE_INSTALL_PREFIX=`realpath temp` .. && \
	make all install
	$(CXX) ${CXXFLAGS} taglib/build/taglib/libtag.so bsdtagconv.cc -o bsdtagconv

clean:
	rm -rf bsdtagconv
	rm -rf taglib/build
