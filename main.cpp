#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
using namespace cl::sycl;

class Particle {
  public:
  int data;
};

//class ParticleSoA {
//  int* a;
//
//  public:
//  ParticleSoA(int n) {
//    a = static_cast<int*>(malloc(n * sizeof(int)));
//  };
//};

int main(int argc, char** argv) {
  // Parse args
  int n = std::atoi(argv[1]);
  n = 3;
  std::cout << "Using N = " << n << std::endl;


  std::vector<Particle> ptl(n);
  ptl[0].data = 1;
  ptl[1].data = 1;
  ptl[2].data = 0;
  //ParticleSoA ptlSoA(n);
  //{
  //  buffer<PartielSoA, 1> num_d(num, range<1>(3));
  //}

  int num[3] = {1, 1, 0};
  queue q(gpu_selector{});
  device dev = q.get_device();
  context ctx = q.get_context();
  std::cout << "Running on "
            << dev.get_info<info::device::name>()
            << std::endl;

  { // buffer scope
    //buffer<Particle, 1> test(&ptl, range<1>(1));
    buffer<int, 1> num_d(num, range<1>(3));
    buffer<Particle, 1> ptl_d(ptl.data(), range<1>(n));
    q.submit([&] (handler& cgh) 
      { // q scope
        auto num = num_d.get_access<access::mode::read_write>(cgh);
        auto ptl = ptl_d.get_access<access::mode::read_write>(cgh);
        cgh.single_task<class oneplusone>([=] 
        {
          ptl[2].data = ptl[0].data + ptl[1].data;
          num[2] = num[0] + num[1];
        } ); // end task scope
      } ); // end q scope
  } // end buffer scope

  std::cout << "1 + 1 = " << num[2] << std::endl;
  std::cout << "ptl 1 + 1 = " << ptl[2].data << std::endl;

  return 0;
}
