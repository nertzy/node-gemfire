#include <v8.h>
#include <node.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>
#include "v8_object_formatter.hpp"
#include <sstream>

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

  v8::String::Utf8Value key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  if(args[1]->IsString()) {
    v8::Local<v8::String> v8Value = args[1]->ToString();
    v8::String::Utf8Value value(v8Value);

    gemfire::CacheableStringPtr valuePtr = gemfire::CacheableString::create(*value);

    regionPtr->put(keyPtr, valuePtr);
    NanReturnValue(v8Value);
  }
  else if(args[1]->IsObject()) {
    v8::Local<v8::Object> value = args[1]->ToObject();
    gemfire::PdxInstancePtr pdxInstance = V8ObjectFormatter::toPdxInstance(regionPtr->getCache(), value);

    regionPtr->put(keyPtr, pdxInstance);
    NanReturnValue(value);
  }
  else {
    std::stringstream errorMessageStream;
    errorMessageStream << "Unable to put javascript entities of type " << *v8::String::Utf8Value(args[1]);
    NanThrowError(errorMessageStream.str().c_str());
    NanReturnUndefined();
  }
}

NAN_METHOD(get) {
  NanScope();

  v8::String::Utf8Value key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  gemfire::CacheablePtr valuePtr = regionPtr->get(keyPtr);

  if(valuePtr == NULLPTR) {
    NanReturnUndefined();
  }

  int typeId = valuePtr->typeId();
  if(typeId == gemfire::GemfireTypeIds::CacheableASCIIString) {
    NanReturnValue(NanNew<v8::String>(((gemfire::CacheableStringPtr) valuePtr)->asChar()));
  }
  else if(typeId > gemfire::GemfireTypeIds::CacheableStringHuge) {
    //We are assuming these are Pdx
    NanReturnValue(V8ObjectFormatter::fromPdxInstance(valuePtr));
  }
  else {
    std::stringstream errorMessageStream;
    errorMessageStream << "Unknown typeId: " << typeId;
    NanThrowError(errorMessageStream.str().c_str());
    NanReturnUndefined();
  }
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

