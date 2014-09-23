#ifndef __EXCEPTIONS_HPP__

#include <gfcpp/GemfireCppCache.hpp>
#include <v8.h>
#include <string>

std::string gemfireExceptionMessage(const gemfire::Exception & exception);
std::string gemfireExceptionMessage(gemfire::UserFunctionExecutionExceptionPtr exceptionPtr);
void ThrowGemfireException(const gemfire::Exception & e);
void emitError(const v8::Handle<v8::Object> & object, const v8::Handle<v8::Value> & error);

#define __EXCEPTIONS_HPP__
#endif
