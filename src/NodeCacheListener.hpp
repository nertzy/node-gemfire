#include <gfcpp/EntryEvent.hpp>
#include <gfcpp/CacheListener.hpp>
#include <node.h>

class NodeCacheListener: public gemfire::CacheListener {
 public:
  NodeCacheListener(uv_async_t * async, uv_mutex_t * eventMutex);
  virtual void afterCreate(const gemfire::EntryEvent & event);
  virtual void afterUpdate(const gemfire::EntryEvent & event);
 private:
  uv_async_t * async;
  uv_mutex_t * eventMutex;
  virtual void queuePutCallbacks(const char * key, const char * newValue);
};
