#include "NodeCacheListener.hpp"
#include <cstring>

using namespace gemfire;

NodeCacheListener::NodeCacheListener(uv_async_t * async, uv_mutex_t * eventMutex) {
  this->async = async;
  this->eventMutex = eventMutex;
}

void NodeCacheListener::afterCreate(const EntryEvent& event) {
  CacheablePtr newValuePtr = event.getNewValue();

  // TODO: call the callback for non-string values
  if (newValuePtr->typeId() == GemfireTypeIds::CacheableASCIIString) {
    CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());
    const char * key = keyPtr->toString();
    const char * newValue = dynCast<CacheableStringPtr>(newValuePtr)->toString();

    this->queuePutCallbacks(key, newValue);
  }
}

void NodeCacheListener::afterUpdate(const EntryEvent& event) {
  CacheablePtr newValuePtr = event.getNewValue();

  // TODO: call the callback for non-string values
  if (newValuePtr->typeId() == GemfireTypeIds::CacheableASCIIString) {
    CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());

    const char * key = keyPtr->toString();
    const char * newValue = dynCast<CacheableStringPtr>(newValuePtr)->toString();

    this->queuePutCallbacks(key, newValue);
  }
}

void NodeCacheListener::queuePutCallbacks(const char * key, const char * newValue) {
  uv_mutex_lock(eventMutex);

  event * outgoingEvent = reinterpret_cast<event *>(async->data);
  outgoingEvent->key = reinterpret_cast<char *>(malloc(sizeof(char) * strlen(key)));
  outgoingEvent->value = reinterpret_cast<char *>(malloc(sizeof(char) * strlen(newValue)));
  snprintf(outgoingEvent->key, strlen(key) + 1, "%s", key);
  snprintf(outgoingEvent->value, strlen(newValue) + 1, "%s", newValue);

  uv_mutex_unlock(eventMutex);

  uv_async_send(async);
}

