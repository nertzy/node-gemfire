#include "streaming_result_collector.hpp"

using namespace gemfire;

namespace node_gemfire {

void StreamingResultCollector::addResult(CacheablePtr & resultPtr) {
  resultStream->add(resultPtr);
}

void StreamingResultCollector::endResults() {
  resultStream->end();
}

}  // namespace node_gemfire

