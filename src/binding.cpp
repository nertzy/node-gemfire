#include <v8.h>
#include <node.h>
#include <nan.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <sstream>
#include "NodeCacheListener.hpp"
#include "event.hpp"
#include "exceptions.hpp"
#include "conversions.hpp"

using namespace v8;
using namespace gemfire;

CachePtr cachePtr;
RegionPtr regionPtr;

Persistent<Object> callbacks;
uv_mutex_t * eventMutex;
bool cacheListenerSet = false;

static void callPutCallbacks(event * incomingEvent) {
  const char * key = incomingEvent->key;
  const char * newValue = incomingEvent->value;

  NanScope();

  Local<Value> putCallbacksValue = callbacks->Get(NanNew("put"));

  Local<Array> putCallbacks =
    Local<Array>::Cast(putCallbacksValue);

  for (unsigned int i = 0; i < putCallbacks->Length(); i++) {
    Local<Value> functionValue = putCallbacks->Get(i);
    Local<Function> putCallback = Local<Function>::Cast(functionValue);

    static const int argc = 2;
    Local<Value> argv[] = { NanNew(key), NanNew(newValue) };
    Local<Context> ctx = NanGetCurrentContext();
    NanMakeCallback(ctx->Global(), putCallback, argc, argv);
  }
}

static void doWork(uv_async_t * async, int status) {
  uv_mutex_lock(eventMutex);
  event * incomingEvent = reinterpret_cast<event *>(async->data);

  callPutCallbacks(incomingEvent);
  uv_mutex_unlock(eventMutex);
}

static void setCacheListener() {
  if (!cacheListenerSet) {
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
  NanReturnValue(NanNew(CacheFactory::getVersion()));
}

NAN_METHOD(put) {
  NanScope();

  if (args.Length() != 2) {
    NanThrowError("put must be called with a key and a value");
    NanReturnUndefined();
  }

  String::Utf8Value key(args[0]);
  CacheableKeyPtr keyPtr = CacheableString::create(*key);

  CacheablePtr valuePtr = gemfireValueFromV8(args[1], cachePtr);

  if (valuePtr == NULLPTR) {
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

  String::Utf8Value key(args[0]);
  CacheableKeyPtr keyPtr = CacheableString::create(*key);

  CacheablePtr valuePtr = regionPtr->get(keyPtr);

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
    Local<Array>::Cast(callbacks->Get(NanNew("put")));

  putCallbacks->Set(putCallbacks->Length(), callback);

  NanReturnValue(NanNew(true));
}

NAN_METHOD(executeQuery) {
  NanScope();

  String::Utf8Value queryString(args[0]);

  QueryServicePtr queryServicePtr = cachePtr->getQueryService();
  QueryPtr queryPtr = queryServicePtr->newQuery(*queryString);
  SelectResultsPtr resultsPtr;
  try {
    resultsPtr = queryPtr->execute();
  }
  catch(const QueryException & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }

  Local<Array> array = NanNew<Array>();

  SelectResultsIterator iterator = resultsPtr->getIterator();

  while (iterator.hasNext()) {
    const SerializablePtr result = iterator.next();
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
  callbacksObj->Set(NanNew("put"), NanNew<Array>());
  NanAssignPersistent(callbacks, callbacksObj);

  CacheFactoryPtr cacheFactory = CacheFactory::createCacheFactory();
  cachePtr = cacheFactory
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

