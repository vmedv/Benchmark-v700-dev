#pragma once

#include <concepts>
#include <string_view>

template <typename Threading>
concept threading =
    requires(Threading t, Threading::Workload w, Threading::WorkloadArg arg) {
      // "thread-like" unit
      typename Threading::thread;
      // mutex type
      typename Threading::mutex;
      // create-and-invoke functor yield should be
      // corresponding thread yielding
      typename Threading::yield;
      // create-and-invoke functor should return thread-id
      typename Threading::id;

      // should be defined Workload type
      requires std::same_as<typename Threading::Workload, void (*)(void *)>;

      // args to executed function should be passed via void*
      requires std::same_as<typename Threading::WorkloadArg, void *>;

      // string descriptor of threading mechanism,
      // e.g. "std::thread", "boost::fiber"
      { t.mechanismId() } -> std::convertible_to<std::string_view>;

      // producer for threads
      // threads should not be created via constructor of Threading::thread
      // since thread creation may require some global state management
      // (see argobots for example)
      { t.spawn(w, arg) } -> std::same_as<typename Threading::thread>;

      // fabric method for creating environment
      // useful for c-like api, where init methods should be called
      // note that env should not be created by hand, only with this method
      { Threading::create() };
    };
