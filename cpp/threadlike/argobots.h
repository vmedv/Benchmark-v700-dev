#pragma once

#include "abt.h"
#include <string_view>

struct argobots {
public:
  using Workload = void (*)(void *);
  using WorkloadArg = void *;
  struct thread {
    explicit thread(Workload const &w, WorkloadArg arg, argobots *manager) {
      _manager = manager;
      ABT_thread_create(manager->pool, w, arg, ABT_THREAD_ATTR_NULL, &_thread);
    }
    void join() { ABT_thread_join(_thread); }
    ~thread() { ABT_thread_free(&_thread); }

  private:
    ABT_thread _thread;
    argobots *_manager;
  };
  struct mutex {
  public:
    explicit mutex() { ABT_mutex_create(&mut); }
    ~mutex() { ABT_mutex_free(&mut); }
    void lock() { ABT_mutex_lock(mut); }
    bool try_lock() { return !ABT_mutex_trylock(mut); }
    void unlock() { ABT_mutex_unlock(mut); }

  private:
    ABT_mutex mut;
  };
  using yield = decltype([] { ABT_self_yield(); });
  using id = decltype([] {
    ABT_unit_id id;
    ABT_self_get_thread_id(&id);
    return id;
  });

  static argobots create() { return argobots(); }

  thread spawn(Workload const &w, WorkloadArg arg) {
    return thread(w, arg, this);
  }

  std::string_view mechanismId() const { return "argobots"; }

  ~argobots() { ABT_finalize(); }

private:
  argobots() {
    ABT_init(0, NULL);
    ABT_xstream_self(&xstream);
    ABT_xstream_get_main_pools(xstream, 1, &pool);
  }

  ABT_xstream xstream;
  ABT_pool pool;
};
