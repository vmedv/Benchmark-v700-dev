#pragma once

#include <boost/fiber/all.hpp>
#include <string_view>

struct fibers {
public:
  using thread = boost::fibers::fiber;
  using mutex = boost::fibers::mutex;
  using yield = decltype([] { boost::this_fiber::yield(); });
  using id = decltype([] { return boost::this_fiber::get_id(); });
  using Workload = void (*)(void *);
  using WorkloadArg = void *;

  std::string_view mechanismId() const { return "boost.fibers"; }
  thread spawn(Workload const &w, WorkloadArg arg) {
    return boost::fibers::fiber(w, arg);
  }

  static fibers create() { return fibers(); }

private:
  fibers() = default;
};
