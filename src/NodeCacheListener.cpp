#include "NodeCacheListener.hpp"
#include <cstring>

NodeCacheListener::NodeCacheListener(uv_async_t * async, uv_mutex_t * eventMutex) {
  this->async = async;
  this->eventMutex = eventMutex;
}

void NodeCacheListener::afterCreate(const EntryEvent& event)
{
  CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());
  CacheableStringPtr newValuePtr = dynCast<CacheableStringPtr>(event.getNewValue());

  const char * key = keyPtr->toString();
  const char * newValue = newValuePtr->toString();

  this->queuePutCallbacks(key, newValue);
}

void NodeCacheListener::afterUpdate(const EntryEvent& event)
{
  CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());
  CacheableStringPtr newValuePtr = dynCast<CacheableStringPtr>(event.getNewValue());

  const char * key = keyPtr->toString();
  const char * newValue = newValuePtr->toString();

  this->queuePutCallbacks(key, newValue);
}

void NodeCacheListener::queuePutCallbacks(const char * key, const char * newValue) {
  uv_mutex_lock(eventMutex);

  event * outgoingEvent = (event *) async->data;
  outgoingEvent->key = (char *) malloc(sizeof(char) * strlen(key));
  outgoingEvent->value = (char *) malloc(sizeof(char) * strlen(newValue));
  strcpy(outgoingEvent->key, key);
  strcpy(outgoingEvent->value, newValue);

  uv_mutex_unlock(eventMutex);

  uv_async_send(async);
}

