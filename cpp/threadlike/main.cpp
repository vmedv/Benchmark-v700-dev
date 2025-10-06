#include <boost/fiber/all.hpp>
#include <iostream>

#include "threading.h"

#include "argobots.h"
#include "fibers.h"
#include "threads.h"

template <threading T> void workload() {
  auto env = T::create();
  std::cout << "with threading: " << env.mechanismId() << std::endl;

  struct args_t {
    T::mutex &m;
  };
  auto work = [](void *arg) {
    auto args = static_cast<args_t *>(arg);
    for (int i = 0; i < 15; i++) {
      std::unique_lock<typename T::mutex> lg(args->m);
      std::cout << "working: " << i << " " << typename T::id{}() << std::endl;
      typename T::yield{}();
    }
  };

  typename T::mutex _m;
  args_t args{.m = _m};
  auto t1 = env.spawn(work, &args);
  auto t2 = env.spawn(work, &args);

  t1.join();
  t2.join();
}

int main() {
  workload<threads>();
  workload<fibers>();
  workload<argobots>();
}
