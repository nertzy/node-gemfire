#include <v8.h>
#include <node.h>
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include "v8_object_formatter.hpp"
#include "NodeCacheListener.hpp"
#include <sstream>
#include "event.hpp"

using namespace v8;

gemfire::CachePtr cachePtr;
gemfire::RegionPtr regionPtr;

Persistent<Object> callbacks;
uv_mutex_t * eventMutex;
bool cacheListenerSet = false;

gemfire::CacheablePtr gemfireValueFromV8(Handle<Value> v8Value) {
  gemfire::CacheablePtr gemfireValuePtr;

  if(v8Value->IsString()) {
    gemfireValuePtr = gemfire::CacheableString::create(*String::Utf8Value(v8Value));
  }
  else if(v8Value->IsBoolean()) {
    gemfireValuePtr = gemfire::CacheableBoolean::create(v8Value->ToBoolean()->Value());
  }
  else if(v8Value->IsNumber()) {
    gemfireValuePtr = gemfire::CacheableDouble::create(v8Value->ToNumber()->Value());
  }
  else if(v8Value->IsDate()) {
    long millisecondsSinceEpoch = Date::Cast(*v8Value)->NumberValue();

    timeval timeSinceEpoch;
    timeSinceEpoch.tv_sec = millisecondsSinceEpoch / 1000;
    timeSinceEpoch.tv_usec = (millisecondsSinceEpoch % 1000) * 1000;

    gemfireValuePtr = gemfire::CacheableDate::create(timeSinceEpoch);
  }
  else if(v8Value->IsObject()) {
    gemfireValuePtr = V8ObjectFormatter::toPdxInstance(regionPtr->getCache(), v8Value->ToObject());
  }
  else if(v8Value->IsNull()) {
    gemfireValuePtr = gemfire::CacheableUndefined::create();
  }
  else {
    gemfireValuePtr = NULLPTR;
  }

  return gemfireValuePtr;
};

Handle<Value> v8ValueFromGemfire(gemfire::CacheablePtr valuePtr) {
  NanScope();

  if(valuePtr == NULLPTR) {
    NanReturnUndefined();
  }

  int typeId = valuePtr->typeId();
  if(typeId == gemfire::GemfireTypeIds::CacheableASCIIString) {
    NanReturnValue(NanNew<String>(((gemfire::CacheableStringPtr) valuePtr)->asChar()));
  }
  if(typeId == gemfire::GemfireTypeIds::CacheableBoolean) {
    NanReturnValue(NanNew<Boolean>(((gemfire::CacheableBooleanPtr) valuePtr)->value()));
  }
  if(typeId == gemfire::GemfireTypeIds::CacheableDouble) {
    NanReturnValue(NanNew<Number>(((gemfire::CacheableDoublePtr) valuePtr)->value()));
  }
  if(typeId == gemfire::GemfireTypeIds::CacheableDate) {
    NanReturnValue(NanNew<Date>((double) ((gemfire::CacheableDatePtr) valuePtr)->milliseconds()));
  }
  if(typeId == gemfire::GemfireTypeIds::CacheableUndefined) {
    NanReturnNull();
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

static void callPutCallbacks(event * incomingEvent) {
  const char * key = incomingEvent->key;
  const char * newValue = incomingEvent->value;

  NanScope();

  Local<Value> putCallbacksValue = callbacks->Get(NanNew<String>("put"));

  Local<Array> putCallbacks =
    Local<Array>::Cast(putCallbacksValue);

  for (unsigned int i = 0; i < putCallbacks->Length(); i++) {
    Local<Value> functionValue = putCallbacks->Get(i);
    Local<Function> putCallback = Local<Function>::Cast(functionValue);

    static const int argc = 2;
    Local<Value> argv[] = { NanNew<String>(key), NanNew<String>(newValue) };
    Local<Context> ctx = NanGetCurrentContext();
    NanMakeCallback(ctx->Global(), putCallback, argc, argv);
  }
}

static void doWork(uv_async_t * async, int status) {
  uv_mutex_lock(eventMutex);
  event * incomingEvent = (event *) async->data;

  callPutCallbacks(incomingEvent);
  uv_mutex_unlock(eventMutex);
}

static void setCacheListener() {
  if(!cacheListenerSet) {
    uv_async_t * async = new uv_async_t();
    async->data = new event;
    uv_async_init(uv_default_loop(), async, doWork);

    eventMutex = new uv_mutex_t();
    uv_mutex_init(eventMutex);

    NodeCacheListener * nodeCacheListener = new NodeCacheListener(async, eventMutex);

    AttributesMutatorPtr attrMutatorPtr = regionPtr->getAttributesMutator();
    attrMutatorPtr->setCacheListener(CacheListenerPtr(nodeCacheListener));

    cacheListenerSet = true;
  }
}

NAN_METHOD(version) {
  NanScope();
  NanReturnValue(NanNew<String>(gemfire::CacheFactory::getVersion()));
}

NAN_METHOD(put) {
  NanScope();

  if(args.Length() != 2) {
    NanThrowError("put must be called with a key and a value");
    NanReturnUndefined();
  }

  String::Utf8Value key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  gemfire::CacheablePtr valuePtr = gemfireValueFromV8(args[1]);

  if(valuePtr == NULLPTR) {
    std::stringstream errorMessageStream;
    errorMessageStream << "Unable to put value " << *String::Utf8Value(args[1]->ToDetailString());
    NanThrowError(errorMessageStream.str().c_str());
    NanReturnUndefined();
  }

  regionPtr->put(keyPtr, valuePtr);
  NanReturnValue(args[1]);
}

NAN_METHOD(get) {
  NanScope();

  String::Utf8Value key(args[0]->ToString());
  gemfire::CacheableKeyPtr keyPtr = gemfire::CacheableString::create(*key);

  gemfire::CacheablePtr valuePtr = regionPtr->get(keyPtr);

  NanReturnValue(v8ValueFromGemfire(valuePtr));
}

NAN_METHOD(clear) {
  NanScope();

  regionPtr->clear();

  NanReturnValue(NanTrue());
}

NAN_METHOD(onPut) {
  setCacheListener();

  NanScope();

  Local<Function> callback = Local<Function>::Cast(args[0]);

  Local<Array> putCallbacks =
    Local<Array>::Cast(callbacks->Get(NanNew<String>("put")));

  putCallbacks->Set(putCallbacks->Length(), callback);

  NanReturnValue(NanNew<Boolean>(true));
}

NAN_METHOD(executeQuery) {
  NanScope();

  String::Utf8Value queryString(args[0]);

  gemfire::QueryServicePtr queryServicePtr = cachePtr->getQueryService();
  gemfire::QueryPtr queryPtr = queryServicePtr->newQuery(*queryString);
  gemfire::SelectResultsPtr resultsPtr;
  try {
    resultsPtr = queryPtr->execute();
  }
  catch(const gemfire::QueryException & exception) {
    NanThrowError(exception.getMessage());
    NanReturnUndefined();
  }

  Local<Array> array = NanNew<Array>();

  gemfire::SelectResultsIterator iterator = resultsPtr->getIterator();

  while (iterator.hasNext())
  {
    const gemfire::SerializablePtr result = iterator.next();
    Handle<Value> v8Value = v8ValueFromGemfire(result);
    array->Set(array->Length(), v8Value);
  }

  NanReturnValue(array);
}

NAN_METHOD(close) {
  NanScope();

  cachePtr->close();

  NanReturnValue(NanTrue());
}

NAN_METHOD(registerAllKeys) {
  NanScope();

  regionPtr->registerAllKeys();

  NanReturnValue(NanTrue());
}

NAN_METHOD(unregisterAllKeys) {
  NanScope();

  regionPtr->unregisterAllKeys();

  NanReturnValue(NanTrue());
}

static void Initialize(Handle<Object> exports) {
  NanScope();

  Local<Object> callbacksObj = NanNew<Object>();
  callbacksObj->Set(NanNew<String>("put"), NanNew<Array>());
  NanAssignPersistent(callbacks, callbacksObj);

  gemfire::CacheFactoryPtr cacheFactory = gemfire::CacheFactory::createCacheFactory();
  cachePtr = cacheFactory
    ->setPdxReadSerialized(true)
    ->set("log-level", "warning")
    ->set("cache-xml-file", "benchmark/xml/BenchmarkClient.xml")
    ->create();

  regionPtr = cachePtr->getRegion("exampleRegion");

  NODE_SET_METHOD(exports, "version", version);
  NODE_SET_METHOD(exports, "put", put);
  NODE_SET_METHOD(exports, "get", get);
  NODE_SET_METHOD(exports, "onPut", onPut);
  NODE_SET_METHOD(exports, "close", close);
  NODE_SET_METHOD(exports, "clear", clear);
  NODE_SET_METHOD(exports, "registerAllKeys", registerAllKeys);
  NODE_SET_METHOD(exports, "unregisterAllKeys", unregisterAllKeys);
  NODE_SET_METHOD(exports, "executeQuery", executeQuery);
}

NODE_MODULE(pivotal_gemfire, Initialize)

