#include "exceptions.hpp"
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>

using namespace v8;

namespace node_gemfire {

Handle<Value> v8Error(const gemfire::Exception & exception) {
  NanEscapableScope();

  Handle<Object> error(NanError(exception.getMessage())->ToObject());
  error->Set(NanNew("name"), NanNew(exception.getName()));

  return NanEscapeScope(error);
}

void ThrowGemfireException(const gemfire::Exception & e) {
  NanThrowError(gemfireExceptionMessage(e).c_str());
}

std::string gemfireExceptionMessage(const gemfire::Exception & exception) {
  std::stringstream errorMessageStream;
  errorMessageStream << exception.getName() << ": " << exception.getMessage();
  return errorMessageStream.str();
}

std::string gemfireExceptionMessage(gemfire::UserFunctionExecutionExceptionPtr exceptionPtr) {
  std::stringstream errorMessageStream;
  errorMessageStream << exceptionPtr->getName()->asChar() << ": " << exceptionPtr->getMessage()->asChar();
  return errorMessageStream.str();
}

}  // namespace node_gemfire
