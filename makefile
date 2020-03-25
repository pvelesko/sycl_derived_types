.PHONY: clean all

all: intel

intel: main.cpp Util.hpp
	dpcpp -std=c++14 -fsycl -I./ ./main.cpp -o intel -g

codeplay: main.cpp Util.hpp
	compute++ -std=c++14 -sycl -sycl-driver -lComputeCpp ./main.cpp -o codeplay

hipsycl: main.cpp Util.hpp
	syclcc-clang-wrapper -std=c++14 ./main.cpp -o hipsycl 

clean:
	rm -f ./intel ./codeplay ./hipsycl
