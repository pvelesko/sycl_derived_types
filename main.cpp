#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
using namespace cl::sycl;

int main(int, char**) {
  int num[3] = {1, 1, 0};
  queue q(gpu_selector{});
  device dev = q.get_device();
  context ctx = q.get_context();
  std::cout << "Running on "
            << dev.get_info<info::device::name>()
            << std::endl;

  { // buffer scope
    buffer<int, 1> num_d(num, range<1>(3));
    q.submit([&] (handler& cgh) 
      { // q scope
        auto num = num_d.get_access<access::mode::read_write>(cgh);
        cgh.single_task<class oneplusone>([=] 
        {
          num[2] = num[0] + num[1];
        } ); // end task scope
      } ); // end q scope
  } // end buffer scope

  std::cout << "1 + 1 = " << num[2] << std::endl;

  return 0;
}
