#ifndef __EXCEPTIONS_HPP__
#define __EXCEPTIONS_HPP__

#include <gfcpp/GemfireCppCache.hpp>
#include <v8.h>
#include <string>

namespace node_gemfire {

v8::Handle<v8::Value> v8Error(const gemfire::Exception & exception);
v8::Handle<v8::Value> v8Error(const gemfire::UserFunctionExecutionExceptionPtr & exceptionPtr);

std::string gemfireExceptionMessage(const gemfire::Exception & exception);
void ThrowGemfireException(const gemfire::Exception & e);

}  // namespace node_gemfire

#endif
