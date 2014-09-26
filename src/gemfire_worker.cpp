#include <gfcpp/GemfireCppCache.hpp>
#include "gemfire_worker.hpp"
#include "exceptions.hpp"

namespace node_gemfire {

void GemfireWorker::Execute() {
  try {
    ExecuteGemfireWork();
  } catch(gemfire::Exception & exception) {
    SetErrorMessage(gemfireExceptionMessage(exception).c_str());
  }
}

}  // namespace node_gemfire
