#ifndef __EXCEPTIONS_HPP__

#include <gfcpp/GemfireCppCache.hpp>
#include <string>

std::string gemfireExceptionMessage(const gemfire::Exception & exception);
void ThrowGemfireException(const gemfire::Exception & e);

#define __EXCEPTIONS_HPP__
#endif
