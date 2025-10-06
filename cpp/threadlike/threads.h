#pragma once

#include <string_view>
#include <thread>

struct threads {
public:
  using thread = std::thread;
  using mutex = std::mutex;
  using yield = decltype([] { std::this_thread::yield(); });
  using id = decltype([] { return std::this_thread::get_id(); });
  using Workload = void (*)(void *);
  using WorkloadArg = void *;

  std::string_view mechanismId() const { return "std.threads"; }

  thread spawn(Workload const &w, WorkloadArg arg) {
    return std::thread(w, arg);
  }

  static threads create() { return threads(); }

private:
  threads() = default;
};
