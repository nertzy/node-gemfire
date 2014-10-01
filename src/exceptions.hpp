#ifndef __EXCEPTIONS_HPP__
#define __EXCEPTIONS_HPP__

#include <gfcpp/GemfireCppCache.hpp>
#include <v8.h>
#include <string>

std::string gemfireExceptionMessage(const gemfire::Exception & exception);
std::string gemfireExceptionMessage(gemfire::UserFunctionExecutionExceptionPtr exceptionPtr);
void ThrowGemfireException(const gemfire::Exception & e);
void emitError(const v8::Local<v8::Object> & object, const v8::Local<v8::Value> & error);

#endif
