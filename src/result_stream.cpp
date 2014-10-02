#include "result_stream.hpp"

using namespace gemfire;

namespace node_gemfire {

void ResultStream::add(const CacheablePtr & resultPtr) {
  uv_mutex_lock(&mutex);
  resultsPtr->push_back(resultPtr);
  uv_mutex_unlock(&mutex);
  uv_async_send(&resultsAsync);
}

void ResultStream::end() {
  uv_mutex_lock(&mutex);
  while (resultsPtr->size() > 0) {
    uv_cond_wait(&resultsProcessedCond, &mutex);
  }
  uv_mutex_unlock(&mutex);

  uv_async_send(&endAsync);
}

void ResultStream::resultsProcessed() {
  uv_cond_signal(&resultsProcessedCond);
}

void ResultStream::endProcessed() {
  uv_cond_signal(&endProcessedCond);
}

void ResultStream::waitUntilFinished() {
  uv_cond_wait(&endProcessedCond, &mutex);
}

CacheableVectorPtr ResultStream::nextResults() {
  uv_mutex_lock(&mutex);

  CacheableVectorPtr returnValue = CacheableVector::create();

  for (CacheableVector::Iterator iterator(resultsPtr->begin());
       iterator != resultsPtr->end();
       ++iterator) {
    returnValue->push_back(*iterator);
  }

  resultsPtr->clear();

  uv_mutex_unlock(&mutex);

  return returnValue;
}

}  // namespace node_gemfire
