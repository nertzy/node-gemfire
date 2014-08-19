#include "exceptions.hpp"
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>

void ThrowGemfireException(const gemfire::Exception & e) {
  NanThrowError(gemfireExceptionMessage(e).c_str());
}

std::string gemfireExceptionMessage(const gemfire::Exception & exception) {
  std::stringstream errorMessageStream;
  errorMessageStream << exception.getName() << ": " << exception.getMessage();
  return errorMessageStream.str();
}
