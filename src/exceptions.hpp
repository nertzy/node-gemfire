#ifndef __EXCEPTIONS_HPP__
#define __EXCEPTIONS_HPP__

#include <gfcpp/GemfireCppCache.hpp>
#include <v8.h>

namespace node_gemfire {

v8::Local<v8::Value> v8Error(const gemfire::Exception & exception);
v8::Local<v8::Value> v8Error(const gemfire::UserFunctionExecutionExceptionPtr & exceptionPtr);

void ThrowGemfireException(const gemfire::Exception & e);

}  // namespace node_gemfire

#endif
