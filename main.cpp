#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
using namespace cl::sycl;

class Particle {
  public:
  int data;
};

class ParticleSoA {
  private:
  int size = 0;
  int sum = 0;
  int* a_array;
  buffer<int, 1> *a_buffer;

  public:
  ParticleSoA() {};
  ParticleSoA(int n) {
    size = n;
    a_array = static_cast<int*>(calloc(n, sizeof(int)));
    a_buffer = new buffer<int, 1>(a_array, n);
  };

  auto *get_array() { return a_array;};
  auto *get_buffer() { return a_buffer;};
  auto  get_size() { return size;};

  // TODO
  auto  get_sum() { 
    int s = 0; 
    for (int i = 0; i < size; i++) s += a_array[i];
    return s;
    ;};
  void  set_sum(int s) { sum = s;};
};


int main(int argc, char** argv) {
  // Parse args
  int n;
  if (argc > 1) {
    n = std::atoi(argv[1]);
  } else {
    n = 3;
  }
  std::cout << "Using N = " << n << std::endl;


  // AoS
  std::vector<Particle> ptl(n);

  // SoA
  ParticleSoA ptl_soa(n);

  // AoSoA
  std::vector<ParticleSoA> ptl_aosoa(n);
  for (int i = 1; i < n; i++) {
    ptl_aosoa[i] = ParticleSoA(n);
  }
  ptl_aosoa[0] = ptl_soa;

  // lambda to be called for host
  auto incr = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
    soa_in.get_array()[idx] += 1;
  };

//  // lambda to be called using host accessors
//  auto incr = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
//    soa_in.get_array()[idx] += 1;
//  };


//  // lambda to be called for SYCL, I want to preserve code layout of passing structs
//  // Trying to figure out if this is possible...
//  auto incr_sycl = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
//    //soa_in.get_accessor()[idx] = soa_in.get_accessor()[idx] + 1;
//    (soa_in.a_accessor)[idx] = -1;
//  };


  // host call 
  for (int i = 0; i < ptl_aosoa[0].get_size(); i++)
    incr(ptl_aosoa[0], ptl.data(), i);


  // sum == n
  std::cout << "ptl_aosoa[0] sum = " << ptl_aosoa[0].get_sum() << std::endl;

  queue q(gpu_selector{});
  device dev = q.get_device();
  context ctx = q.get_context();
  std::cout << "Running on "
            << dev.get_info<info::device::name>()
            << std::endl;

  q.submit([&] (handler& cgh) mutable { // q scope
    //accessor<int, 1, access::mode:read_write> a0 = accessor<int, 1, access::mode::read_write, access::target::global_buffer>(ptl_aosoa[0].get_buffer(), cgh);
    accessor<int, 1, access::mode::read_write> a0 = accessor<int, 1, access::mode::read_write, access::target::global_buffer>(*ptl_aosoa[0].get_buffer(), cgh);

    // What if I want to create an array of such accessors? Guessing that I can't
    std::vector<accessor<int, 1, access::mode::read_write>> avec;
    accessor<int, 1, access::mode::read_write>* aarr[n];
    aarr[0] = &a0;
    avec.push_back(a0);
    cgh.parallel_for<class oneplusone>(range<1>(ptl_aosoa[0].get_size()), [=] (id<1> idx) mutable {
      //avec[0][0] += 1; // compiler crash
      aarr[0][0] += 1; // compiler crash
      //a0[idx] += 1; // works
    } ); // end task scope
  } ); // end q scope

  int buffersum = 0;
  for (int i = 0; i < ptl_aosoa[0].get_size(); i++) {
    buffersum += ptl_aosoa[0].get_buffer()->get_access<access::mode::read>()[i];
  }
  std::cout << "ptl_aosoa[0] sum = " << ptl_aosoa[0].get_sum() << std::endl;
  std::cout << "ptl_aosoa[0] buffer sum = " << buffersum << std::endl;
  //std::cout << "1 + 1 = " << ptl_soa.get_array()[0] << std::endl;
  //std::cout << "ptl 1 + 1 = " << ptl[2].data << std::endl;

  return 0;
}
