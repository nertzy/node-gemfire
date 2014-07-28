#include <v8.h>
#include <node.h>
#include <gfcpp/CacheFactory.hpp>
#include <nan.h>

gemfire::CachePtr cachePtr;
gemfire::RegionPtr regionPtr;

NAN_METHOD(version) {
  NanScope();
  NanReturnValue(NanNew<v8::String>(gemfire::CacheFactory::getVersion()));
}

NAN_METHOD(put) {
  NanScope();

  v8::String::AsciiValue key(args[0]->ToString());
  v8::String::AsciiValue value(args[1]->ToString());

  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);
  gemfire::CacheablePtr valuePtr = gemfire::CacheableString::create(*value);

  regionPtr->put(keyPtr, valuePtr);

  NanReturnValue(NanNew<v8::String>(*value));
}

NAN_METHOD(get) {
  NanScope();

  v8::String::AsciiValue key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);
  gemfire::CacheableStringPtr valuePtr = regionPtr->get(keyPtr);

  const char* value = valuePtr->asChar();

  NanReturnValue(NanNew<v8::String>(value));
}

NAN_METHOD(close) {
  NanScope();

  cachePtr->close();

  NanReturnValue(NanNew<v8::Boolean>(true));
}

static void Initialize(v8::Handle<v8::Object> target) {
  NanScope();

  gemfire::CacheFactoryPtr cacheFactory = gemfire::CacheFactory::createCacheFactory();
  cachePtr = cacheFactory->create();
  gemfire::RegionFactoryPtr regionFactory = cachePtr->createRegionFactory(gemfire::LOCAL);
  regionPtr = regionFactory->create("exampleRegion");

  NODE_SET_METHOD(target, "version", version);
  NODE_SET_METHOD(target, "put", put);
  NODE_SET_METHOD(target, "get", get);
  NODE_SET_METHOD(target, "close", close);
}

NODE_MODULE(pivotal_gemfire, Initialize)

