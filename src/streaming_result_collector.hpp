#ifndef __STREAMING_RESULT_COLLECTOR_HPP__
#define __STREAMING_RESULT_COLLECTOR_HPP__

#include <gfcpp/ResultCollector.hpp>
#include "result_stream.hpp"

namespace node_gemfire {

class StreamingResultCollector : public gemfire::ResultCollector {
 public:
  explicit StreamingResultCollector(ResultStreamPtr resultStream) :
      ResultCollector(),
      resultStream(resultStream) {}

  virtual void addResult(gemfire::CacheablePtr & resultPtr);
  virtual void endResults();

 private:
  ResultStreamPtr resultStream;
};

}  // namespace node_gemfire

#endif
