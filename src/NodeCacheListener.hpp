#include <gfcpp/EntryEvent.hpp>
#include <v8.h>
#include <nan.h>
#include <gfcpp/CacheListener.hpp>

using namespace gemfire;

class NodeCacheListener: public CacheListener
{
public:
  NodeCacheListener(v8::Persistent<v8::Object> callbacks);
  virtual void afterCreate( const EntryEvent& event );
  virtual void afterUpdate( const EntryEvent& event );
private:
  v8::Persistent<v8::Object> callbacks;
  virtual void callPutCallbacks(const char * key, const char * newValue);
};
