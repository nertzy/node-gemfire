#ifndef __GEMFIRE_WORKER_HPP__
#define __GEMFIRE_WORKER_HPP__

#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>

namespace node_gemfire {

class GemfireWorker : public NanAsyncWorker {
 public:
  explicit GemfireWorker(
      NanCallback * callback) :
    NanAsyncWorker(callback),
    exceptionPtr(NULLPTR),
    errorName() {}

  virtual void SetError(const char * name, const char * message);
  virtual void ExecuteGemfireWork() = 0;
  virtual void HandleErrorCallback();
  virtual void WorkComplete();
  virtual void Execute();

 protected:
  v8::Local<v8::Value> errorObject();

  gemfire::ExceptionPtr exceptionPtr;
  std::string errorName;
};

}  // namespace node_gemfire

#endif
