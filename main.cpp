#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
#include "Util.hpp"

using namespace cl::sycl;

template<class T> 
class Particle {
  public:
  T  pos[3]; // represent pos(x,y,z) coords
};

template<class T>
class ParticleAoS {
  public:
  std::vector<Particle<T>> *ptls;
  // Default constructor
  ParticleAoS(int n) {
    ptls = new std::vector<Particle<T>>(n);};
  // Copy Constructor
  ParticleAoS(ParticleAoS &other) {
    ptls = other.ptls;
  };
};

template<class T>
using ParticleSoAtype = std::vector<Particle<T>>;

template<class T>
class ParticleSoAtype_sycl {
  public:
  ParticleSoAtype<T> *hostptl;
  buffer<Particle<T>, 1> *ptls_d;
  ParticleSoAtype_sycl(int n) {
    hostptl = new ParticleSoAtype<T>(n);
    ptls_d = new buffer<Particle<T>, 1> (hostptl, range<1>(n));
  };
  ParticleSoAtype_sycl(ParticleSoAtype<T> &other) {
    hostptl = other.ptls;
    ptls_d = new buffer<Particle<T>, 1> (hostptl, range<1>(other.ptls->size())); 
  }
};

template<class T>
class ParticleAoS_sycl : public ParticleAoS<T> {
  public:
    using ParticleAoS<T>::ptls;
    buffer<Particle<T>, 1> *ptls_d;
    
    /* Construct host and device side instances*/
    ParticleAoS_sycl(int n) : ParticleAoS<T>(n) {
      ptls_d = new buffer<Particle<T>, 1>(ParticleAoS<T>::ptls->data(), range<1>(n));
    }

    /* Construct based on existing host side instance */
    ParticleAoS_sycl(ParticleAoS<T> &ptlaos) : ParticleAoS<T>(ptlaos) { 
      ptls_d = new buffer<Particle<T>, 1> (ptlaos.ptls->data(), range<1>(ptlaos.ptls->size()));
    }

    /* copy to host */
    void copytohost() {
      auto p = ptls_d->template get_access<access::mode::read>();
      for (int i = 0; i < ptls->size(); i++) {
        (*ptls)[i] = p[i];
      }
    };

};

template<class T>
void ptl_incr(Particle<T> *p, int len) {
  for (int i = 0; i < len; i++) {
    p[i].pos[0] = 1;
    p[i].pos[1] = 2;
    p[i].pos[2] = 3;
  }
}



template<class T>
class ParticleSoA {
  private:
  T size = 0;
  T sum = 0;
  T* a_array;
  buffer<T, 1> *a_buffer;

  public:
  ParticleSoA() {};
  ParticleSoA(int n) {
    size = n;
    a_array = static_cast<T*>(calloc(n, sizeof(T)));
    a_buffer = new buffer<T, 1>(a_array, n);
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

  queue q(gpu_selector{});
  device dev = q.get_device();
  context ctx = q.get_context();
  std::cout << "Running on "
            << dev.get_info<info::device::name>()
            << std::endl;

auto ptl_incr_lambda = [](auto p, int len) {
  for (int i = 0; i < len; i++) {
    p[i].pos[0] = 1;
    p[i].pos[1] = 2;
    p[i].pos[2] = 3;
  }
};

  // AoS
  ParticleAoS<float> p(n);
  ParticleAoS_sycl<float> p_d(n);
  ParticleAoS_sycl<float> pp_d(p);
  dump((*p.ptls)[0].pos, 3, "ptl_pos");
  //ptl_incr(pp_d.ptls->data(), p.ptls->size());
//  ptl_incr_lambda(pp_d.ptls->data(), p.ptls->size());



  q.submit([&] (handler& cgh) mutable { // q scope
    accessor<Particle<float>, 1, access::mode::read_write> p = pp_d.ptls_d->get_access<access::mode::read_write>(cgh);
    int size = pp_d.ptls->size();
    cgh.parallel_for<class oneplusone>(range<1>(n), [=] (id<1> idx) {
        ptl_incr_lambda(p, size);
    } ); // end task scope
  } ); // end q scope

  q.wait();
//  pp_d.copytohost();
  dump((*p.ptls)[0].pos, 3, "ptl_pos");
  auto pa = pp_d.ptls_d->template get_access<access::mode::read>();
  dump(pa[0].pos, 3, "ptl_pos");

//  // SoA
//  ParticleSoA ptl_soa(n);
//
//  // AoSoA
//  std::vector<ParticleSoA> ptl_aosoa(n);
//  for (int i = 1; i < n; i++) {
//    ptl_aosoa[i] = ParticleSoA(n);
//  }
//  ptl_aosoa[0] = ptl_soa;
//
//  // lambda to be called for host
//  auto incr = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
//    soa_in.get_array()[idx] += 1;
//  };
//
////  // lambda to be called using host accessors
////  auto incr = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
////    soa_in.get_array()[idx] += 1;
////  };
//
//
////  // lambda to be called for SYCL, I want to preserve code layout of passing structs
////  // Trying to figure out if this is possible...
////  auto incr_sycl = [=](ParticleSoA  &soa_in, Particle *aos_in, int idx) {
////    //soa_in.get_accessor()[idx] = soa_in.get_accessor()[idx] + 1;
////    (soa_in.a_accessor)[idx] = -1;
////  };
//
//
//  // host call 
//  for (int i = 0; i < ptl_aosoa[0].get_size(); i++)
//    incr(ptl_aosoa[0], ptl.data(), i);
//
//
//  // sum == n
//  std::cout << "ptl_aosoa[0] sum = " << ptl_aosoa[0].get_sum() << std::endl;
//
//
//  q.submit([&] (handler& cgh) mutable { // q scope
//    //accessor<int, 1, access::mode:read_write> a0 = accessor<int, 1, access::mode::read_write, access::target::global_buffer>(ptl_aosoa[0].get_buffer(), cgh);
//    accessor<int, 1, access::mode::read_write> a0 = accessor<int, 1, access::mode::read_write, access::target::global_buffer>(*ptl_aosoa[0].get_buffer(), cgh);
//
//    // What if I want to create an array of such accessors? Guessing that I can't
//    std::vector<accessor<int, 1, access::mode::read_write>> avec;
//    accessor<int, 1, access::mode::read_write>* aarr[n];
//    aarr[0] = &a0;
//    avec.push_back(a0);
//    cgh.parallel_for<class oneplusone>(range<1>(ptl_aosoa[0].get_size()), [=] (id<1> idx) mutable {
//      //avec[0][0] += 1; // compiler crash
//      //aarr[0][0] += 1; // compiler crash
//      //a0[idx] += 1; // works
//    } ); // end task scope
//  } ); // end q scope
//
//  int buffersum = 0;
//  for (int i = 0; i < ptl_aosoa[0].get_size(); i++) {
//    buffersum += ptl_aosoa[0].get_buffer()->get_access<access::mode::read>()[i];
//  }
//  std::cout << "ptl_aosoa[0] sum = " << ptl_aosoa[0].get_sum() << std::endl;
//  std::cout << "ptl_aosoa[0] buffer sum = " << buffersum << std::endl;
//  //std::cout << "1 + 1 = " << ptl_soa.get_array()[0] << std::endl;
//  //std::cout << "ptl 1 + 1 = " << ptl[2].data << std::endl;

  return 0;
}
