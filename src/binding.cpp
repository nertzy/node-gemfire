#include <v8.h>
#include <node.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>
#include "v8_object_formatter.hpp"

gemfire::CachePtr cachePtr;
gemfire::RegionPtr regionPtr;

NAN_METHOD(version) {
  NanScope();
  NanReturnValue(NanNew<v8::String>(gemfire::CacheFactory::getVersion()));
}

NAN_METHOD(put) {
  NanScope();

  if(args.Length() != 2) {
    NanThrowError("put must be called with a key and a value");
    NanReturnUndefined();
  }

  v8::String::AsciiValue key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  v8::Local<v8::Object> value = args[1]->ToObject();
  gemfire::PdxInstancePtr pdxInstance = V8ObjectFormatter::toPdxInstance(regionPtr->getCache(), value);

  regionPtr->put(keyPtr, pdxInstance);

  NanReturnValue(value);
}

NAN_METHOD(get) {
  NanScope();

  v8::String::AsciiValue key(args[0]->ToString());

  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  gemfire::PdxInstancePtr pdxInstance = regionPtr->get(keyPtr);
  v8::Local<v8::Object> value = V8ObjectFormatter::fromPdxInstance(pdxInstance);

  NanReturnValue(value);
}

NAN_METHOD(clear) {
  NanScope();

  regionPtr->clear();

  NanReturnValue(NanTrue());
}

NAN_METHOD(close) {
  NanScope();

  cachePtr->close();

  NanReturnValue(NanTrue());
}

static void Initialize(v8::Handle<v8::Object> target) {
  NanScope();

  gemfire::CacheFactoryPtr cacheFactory = gemfire::CacheFactory::createCacheFactory();
  cachePtr = cacheFactory
    ->setPdxReadSerialized(true)
    ->set("log-level", "warning")
    ->set("cache-xml-file", "benchmark/xml/BenchmarkClient.xml")
    ->create();

  regionPtr = cachePtr->getRegion("exampleRegion");

  NODE_SET_METHOD(target, "version", version);
  NODE_SET_METHOD(target, "put", put);
  NODE_SET_METHOD(target, "get", get);
  NODE_SET_METHOD(target, "close", close);
  NODE_SET_METHOD(target, "clear", clear);
}

NODE_MODULE(pivotal_gemfire, Initialize)

