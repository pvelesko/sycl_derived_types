#include <CL/sycl.hpp>
#include <stdlib.h>
#include <iostream>
using namespace cl::sycl;

int main(int, char**) {
  int num[3] = {1, 1, 0};
  {
  queue queue(cpu_selector{});
    std::cout << "Running on "
              << queue.get_device().get_info<info::device::name>()
              << "\n";
  buffer<int, 1> num_d(num, range<1>(3));
  queue.submit([&] (handler& cgh) {
      auto num = num_d.get_access<access::mode::read_write>(cgh);
      cgh.single_task<class oneplusone>([=] {
        num[2] = num[0] + num[1];
    } );
    } );
  }

  std::cout << "1 + 1 = " << num[2] << std::endl;

  return 0;
}

