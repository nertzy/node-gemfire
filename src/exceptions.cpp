#include "exceptions.hpp"
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <sstream>

void ThrowGemfireException(const gemfire::Exception & e) {
  std::stringstream errorMessageStream;
  errorMessageStream << e.getName() << ": " << e.getMessage();

  e.printStackTrace();

  NanThrowError(errorMessageStream.str().c_str());
}
