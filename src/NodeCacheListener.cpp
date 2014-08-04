#include "NodeCacheListener.hpp"

NodeCacheListener::NodeCacheListener(v8::Persistent<v8::Object> callbacks) {
  this->callbacks = callbacks;
}

void NodeCacheListener::afterCreate(const EntryEvent& event)
{
  CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());
  CacheableStringPtr newValuePtr = dynCast<CacheableStringPtr>(event.getNewValue());

  const char * key = keyPtr->toString();
  const char * newValue = newValuePtr->toString();

  this->callPutCallbacks(key, newValue);
}

void NodeCacheListener::afterUpdate(const EntryEvent& event)
{
  CacheableStringPtr keyPtr = dynCast<CacheableStringPtr>(event.getKey());
  CacheableStringPtr newValuePtr = dynCast<CacheableStringPtr>(event.getNewValue());

  const char * key = keyPtr->toString();
  const char * newValue = newValuePtr->toString();

  this->callPutCallbacks(key, newValue);
}

void NodeCacheListener::callPutCallbacks(const char * key, const char * newValue) {
  NanScope();

  v8::Local<v8::Value> putCallbacksValue = callbacks->Get(NanNew<v8::String>("put"));

  v8::Local<v8::Array> putCallbacks =
    v8::Local<v8::Array>::Cast(putCallbacksValue);

  for (int i = 0; i < putCallbacks->Length(); i++) {
    v8::Local<v8::Value> functionValue = putCallbacks->Get(i);
    v8::Local<v8::Function> putCallback = v8::Local<v8::Function>::Cast(functionValue);

    static const int argc = 2;
    v8::Local<v8::Value> argv[] = { NanNew<v8::String>(key), NanNew<v8::String>(newValue) };
    v8::Local<v8::Context> ctx = NanGetCurrentContext();
    NanMakeCallback(ctx->Global(), putCallback, argc, argv);
  }
}

