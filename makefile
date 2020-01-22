.PHONY: clean all

all: intel

intel: main.cpp
	clang++ -std=c++14 -fsycl -lsycl -lOpenCL ./main.cpp -o intel

codeplay: main.cpp
	compute++ -std=c++14 -sycl -sycl-driver -lComputeCpp ./main.cpp -o codeplay

hipsycl: main.cpp
	syclcc-clang-wrapper -std=c++14 ./main.cpp -o hipsycl 

clean:
	rm -f ./intel ./codeplay
