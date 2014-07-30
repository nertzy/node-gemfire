benchmark: benchmark.cpp
	g++ \
	-std=c++0x \
	-D_REENTRANT \
	-m64 \
	-I${GFCPP}/include \
	-Wl,-rpath,${GFCPP}/lib \
	-Wl,-L${GFCPP}/lib \
	benchmark.cpp \
	-o benchmark \
	-lgfcppcache

test:
	make benchmark && ./benchmark
