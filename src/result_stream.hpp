#ifndef __RESULT_STREAM_HPP__
#define __RESULT_STREAM_HPP__

#include <gfcpp/SharedBase.hpp>
#include <gfcpp/CacheableBuiltins.hpp>
#include <uv.h>

namespace node_gemfire {

class ResultStream : public gemfire::SharedBase {
 public:
  explicit ResultStream(void * worker, uv_async_cb resultsCallback, uv_async_cb endCallback) :
    SharedBase(),
    resultsPtr(gemfire::CacheableVector::create()) {
      uv_mutex_init(&mutex);
      uv_cond_init(&endProcessedCond);
      uv_cond_init(&resultsProcessedCond);
      resultsAsync.data = worker;
      endAsync.data = worker;
      uv_async_init(uv_default_loop(), &resultsAsync, resultsCallback);
      uv_async_init(uv_default_loop(), &endAsync, endCallback);
    }

  virtual ~ResultStream() {
    uv_close(reinterpret_cast<uv_handle_t *>(&endAsync), NULL);
    uv_close(reinterpret_cast<uv_handle_t *>(&resultsAsync), NULL);
    uv_mutex_destroy(&mutex);
    uv_cond_destroy(&endProcessedCond);
    uv_cond_destroy(&resultsProcessedCond);
  }

  void add(const gemfire::CacheablePtr & resultPtr);
  void end();
  void resultsProcessed();
  void endProcessed();
  void waitUntilFinished();

  gemfire::CacheableVectorPtr nextResults();

 private:
  uv_mutex_t mutex;

  uv_async_t resultsAsync;
  uv_async_t endAsync;

  uv_cond_t resultsProcessedCond;
  uv_cond_t endProcessedCond;

  gemfire::CacheableVectorPtr resultsPtr;
};

typedef gemfire::SharedPtr<ResultStream> ResultStreamPtr;

}  // namespace node_gemfire

#endif
