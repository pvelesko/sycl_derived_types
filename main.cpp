#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
#include "Util.hpp"

using namespace cl::sycl;

template<class T> 
struct Particle {
  T  pos[3]; // represent pos(x,y,z) coords
  T  vel[3]; 
  T  acc[3]; 
};

template<class T>
class ParticleAoS {
  int* ref; // reference counter for USM
  Particle<T>* ptls;
  public:
  ParticleAoS(int n) {
    ptls = static_cast<Particle<T>*>(malloc_shared(n * sizeof(Particle<T>), q));
    ref = static_cast<int*>(malloc_shared(sizeof(int), q));
    for (int i = 0; i << n; i++) {
      ptls[i].pos[0] = 0; ptls[i].pos[1] = 0; ptls[i].pos[2] = 0;
      ptls[i].vel[0] = 0; ptls[i].vel[1] = 0; ptls[i].vel[2] = 0;
      ptls[i].acc[0] = 0; ptls[i].acc[1] = 0; ptls[i].acc[2] = 0;
    }
    *ref = 1;
  };
  Particle<T>* data() const {
    return ptls;
  };

  /* This section gets compiled for the host only 
   * Allows for proper deallocation of USM */
#ifndef __SYCL_DEVICE_ONLY__
  ParticleAoS(const ParticleAoS<T>& other) {
    ptls = other.ptls;
    ref = other.ref;
    (*ref)++;
  };

  ~ParticleAoS() {
    (*ref)--;
    if (*ref == 0) {
      free(ptls, q);
      free(ref, q);
      std::cout << "USM deallocated" << std::endl;
    }
  };
#endif
  
};

template<class T>
inline void ptl_incr(ParticleAoS<T>& p, int i, int len) {
  p.data()[i].pos[0] = 1;
  p.data()[i].pos[1] = 2;
  p.data()[i].pos[2] = 3;
}

int main(int argc, char** argv) {
  init(); // Queue initialize
  process_args(argc, argv);
  int N = n;
  ParticleAoS<float> particles(N);

  dump(particles.data()[0].pos, "particles[0].pos");
  q.submit([&](handler& cgh) { // q scope
    cgh.parallel_for(range<1>(N), [=](id<1> idx) mutable { // needs to be mutable, otherwise particles are const
      ptl_incr(particles, idx, N);
    } ); // end task scope
  } ); // end q scope
  q.wait();
  dump(particles.data()[0].pos, "particles[0].pos");

  return 0;
}
