.PHONY: clean all

all: intel

intel: main.cpp
	clang++ -std=c++14 -fsycl -lsycl -lOpenCL ./main.cpp -o intel

codeplay: main.cpp
	compute++ -std=c++11 -sycl -sycl-driver -lComputeCpp ./main.cpp -o codeplay

clean:
	rm -f ./intel ./codeplay
