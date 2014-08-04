#include <gfcpp/EntryEvent.hpp>
#include <gfcpp/CacheListener.hpp>
#include <node.h>
#include "event.hpp"

using namespace gemfire;

class NodeCacheListener: public CacheListener
{
public:
  NodeCacheListener(uv_async_t * async, uv_mutex_t * eventMutex);
  virtual void afterCreate( const EntryEvent& event );
  virtual void afterUpdate( const EntryEvent& event );
private:
  uv_async_t * async;
  uv_mutex_t * eventMutex;
  virtual void queuePutCallbacks(const char * key, const char * newValue);
};
