#ifndef __GEMFIRE_WORKER_HPP__
#define __GEMFIRE_WORKER_HPP__

#include <nan.h>

namespace node_gemfire {

class GemfireWorker : public NanAsyncWorker {
 public:
  explicit GemfireWorker(
      NanCallback * callback) :
    NanAsyncWorker(callback) {}

  virtual void ExecuteGemfireWork() = 0;

  void Execute();
};

}  // namespace node_gemfire

#endif
