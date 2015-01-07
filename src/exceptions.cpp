#include "exceptions.hpp"
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Handle<Value> v8Error(const gemfire::Exception & exception) {
  NanEscapableScope();

  Handle<Object> error(NanError(exception.getMessage())->ToObject());
  error->Set(NanNew("name"), NanNew(exception.getName()));

  return NanEscapeScope(error);
}

Handle<Value> v8Error(const UserFunctionExecutionExceptionPtr & exceptionPtr) {
  NanEscapableScope();

  Handle<Object> error(NanError(exceptionPtr->getMessage()->asChar())->ToObject());
  error->Set(NanNew("name"), NanNew(exceptionPtr->getName()->asChar()));

  return NanEscapeScope(error);
}

void ThrowGemfireException(const gemfire::Exception & e) {
  NanThrowError(v8Error(e));
}

std::string gemfireExceptionMessage(const gemfire::Exception & exception) {
  std::stringstream errorMessageStream;
  errorMessageStream << exception.getName() << ": " << exception.getMessage();
  return errorMessageStream.str();
}

}  // namespace node_gemfire
