#include "result_stream.hpp"

using namespace gemfire;

namespace node_gemfire {

void ResultStream::add(const CacheablePtr & resultPtr) {
  uv_mutex_lock(&resultsMutex);
  resultsPtr->push_back(resultPtr);
  uv_mutex_unlock(&resultsMutex);
  uv_async_send(resultsAsync);
}

void ResultStream::end() {
  uv_mutex_lock(&resultsProcessedMutex);
  while (resultsPtr->size() > 0) {
    uv_cond_wait(&resultsProcessedCond, &resultsProcessedMutex);
  }
  uv_mutex_unlock(&resultsProcessedMutex);

  uv_async_send(endAsync);
}

void ResultStream::resultsProcessed() {
  uv_mutex_lock(&resultsProcessedMutex);
  uv_cond_signal(&resultsProcessedCond);
  uv_mutex_unlock(&resultsProcessedMutex);
}

CacheableVectorPtr ResultStream::nextResults() {
  uv_mutex_lock(&resultsMutex);

  CacheableVectorPtr returnValue = CacheableVector::create();

  for (CacheableVector::Iterator iterator(resultsPtr->begin());
       iterator != resultsPtr->end();
       ++iterator) {
    returnValue->push_back(*iterator);
  }

  resultsPtr->clear();

  uv_mutex_unlock(&resultsMutex);

  return returnValue;
}

void ResultStream::deleteHandle(uv_handle_t * handle) {
  delete handle;
}

}  // namespace node_gemfire
