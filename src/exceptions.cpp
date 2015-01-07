#include "exceptions.hpp"
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Local<Value> v8Error(const gemfire::Exception & exception) {
  NanEscapableScope();

  Local<Object> error(NanError(exception.getMessage())->ToObject());
  error->Set(NanNew("name"), NanNew(exception.getName()));

  return NanEscapeScope(error);
}

Local<Value> v8Error(const UserFunctionExecutionExceptionPtr & exceptionPtr) {
  NanEscapableScope();

  Local<Object> error(NanError(exceptionPtr->getMessage()->asChar())->ToObject());
  error->Set(NanNew("name"), NanNew(exceptionPtr->getName()->asChar()));

  return NanEscapeScope(error);
}

void ThrowGemfireException(const gemfire::Exception & e) {
  NanThrowError(v8Error(e));
}

}  // namespace node_gemfire
