#include <gfcpp/GemfireCppCache.hpp>
#include "gemfire_worker.hpp"
#include "exceptions.hpp"

using namespace v8;

namespace node_gemfire {

void GemfireWorker::Execute() {
  try {
    ExecuteGemfireWork();
  } catch(gemfire::Exception & exception) {
    exceptionPtr = exception.clone();
  }
}

void GemfireWorker::HandleErrorCallback() {
  NanScope();

  static const int argc = 1;
  Local<Value> argv[argc] = { errorObject() };
  callback->Call(argc, argv);
}

void GemfireWorker::WorkComplete() {
  NanScope();

  if (exceptionPtr != NULLPTR || ErrorMessage() != NULL) {
    HandleErrorCallback();
  } else {
    HandleOKCallback();
  }

  delete callback;
  callback = NULL;
}

void GemfireWorker::SetError(const char * name, const char * message) {
  errorName = name;
  SetErrorMessage(message);
}

Local<Value> GemfireWorker::errorObject() {
  NanEscapableScope();

  if (exceptionPtr != NULLPTR) {
    return NanEscapeScope(v8Error(*exceptionPtr));
  } else {
    Local<Object> error(NanError(ErrorMessage())->ToObject());
    error->Set(NanNew("name"), NanNew(errorName));
    return NanEscapeScope(error);
  }
}

}  // namespace node_gemfire
