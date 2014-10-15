#ifndef __EVENT_STREAM_HPP__
#define __EVENT_STREAM_HPP__

#include <gfcpp/SharedPtr.hpp>
#include <gfcpp/SharedBase.hpp>
#include <gfcpp/CacheableBuiltins.hpp>
#include <gfcpp/EntryEvent.hpp>
#include <uv.h>
#include <vector>
#include <cassert>

namespace node_gemfire {

class EventStream: public gemfire::SharedBase {
 public:
  explicit EventStream(
      void * target,
      uv_async_cb callback) :
    SharedBase() {
      uv_mutex_init(&mutex);
      async.data = target;
      uv_async_init(uv_default_loop(), &async, callback);
      uv_unref(reinterpret_cast<uv_handle_t *>(&async));
    }

  virtual ~EventStream() {
    uv_close(reinterpret_cast<uv_handle_t *>(&async), NULL);
    uv_mutex_destroy(&mutex);
  }

  void add(const gemfire::EntryEvent & event);
  std::vector<gemfire::EntryEventPtr> nextEvents();

 private:
  static void teardownCallback(uv_work_t * request);
  static void afterTeardownCallback(uv_work_t * request, int status);
  void teardown();

  uv_mutex_t mutex;
  uv_async_t async;

  std::vector<gemfire::EntryEventPtr> entryEventVector;
};

}  // namespace node_gemfire

#endif
