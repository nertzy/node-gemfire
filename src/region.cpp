#include "region.hpp"
#include <gfcpp/Region.hpp>
#include <gfcpp/FunctionService.hpp>
#include <sstream>
#include <string>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "event.hpp"
#include "NodeCacheListener.hpp"
#include "cache.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Persistent<FunctionTemplate> regionConstructor;

bool cacheListenerSet = false;
uv_mutex_t * eventMutex;
Persistent<Object> callbacks;

Region::~Region() {
  NanDisposePersistent(cacheHandle);
}

void Region::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>(Region::New);

  constructor->SetClassName(NanNew("Region"));
  constructor->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(constructor, "clear",
      NanNew<FunctionTemplate>(Region::Clear)->GetFunction());
  NanSetPrototypeTemplate(constructor, "put",
      NanNew<FunctionTemplate>(Region::Put)->GetFunction());
  NanSetPrototypeTemplate(constructor, "get",
      NanNew<FunctionTemplate>(Region::Get)->GetFunction());
  NanSetPrototypeTemplate(constructor, "registerAllKeys",
      NanNew<FunctionTemplate>(Region::RegisterAllKeys)->GetFunction());
  NanSetPrototypeTemplate(constructor, "unregisterAllKeys",
      NanNew<FunctionTemplate>(Region::UnregisterAllKeys)->GetFunction());
  NanSetPrototypeTemplate(constructor, "onPut",
      NanNew<FunctionTemplate>(Region::OnPut)->GetFunction());
  NanSetPrototypeTemplate(constructor, "executeFunction",
      NanNew<FunctionTemplate>(Region::ExecuteFunction)->GetFunction());

  NanAssignPersistent(regionConstructor, constructor);

  Local<Object> callbacksObj = NanNew<Object>();
  callbacksObj->Set(NanNew("put"), NanNew<Array>());
  NanAssignPersistent(callbacks, callbacksObj);

  exports->Set(NanNew("Region"), regionConstructor->GetFunction());
}

NAN_METHOD(Region::New) {
  NanScope();

  NanReturnValue(args.This());
}

NAN_METHOD(Region::GetRegion) {
  NanScope();

  Local<Object> cacheHandle = args[0]->ToObject();

  Cache * cache = ObjectWrap::Unwrap<Cache>(cacheHandle);
  CachePtr cachePtr = cache->cachePtr;
  RegionPtr regionPtr = cachePtr->getRegion(*NanAsciiString(args[1]));

  if (regionPtr == NULLPTR) {
    NanReturnUndefined();
  }

  Region * region = new Region(cacheHandle, regionPtr);

  const unsigned int argc = 0;
  Handle<Value> argv[] = {};
  Local<Object> regionHandle = regionConstructor->GetFunction()->NewInstance(argc, argv);

  region->Wrap(regionHandle);

  NanReturnValue(regionHandle);
}

NAN_METHOD(Region::Clear) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;
  regionPtr->clear();

  NanReturnValue(NanTrue());
}

static Handle<Value> unableToPutValueError(Handle<Value> v8Value) {
  NanScope();
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to put value " << *String::Utf8Value(v8Value->ToDetailString());
  NanReturnValue(NanError(errorMessageStream.str().c_str()));
}

NAN_METHOD(Region::Put) {
  NanScope();

  if (args.Length() < 2) {
    NanThrowError("put must be called with a key and a value");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;
  CachePtr cachePtr = regionPtr->getCache();
  CacheableKeyPtr keyPtr = gemfireValueFromV8(args[0], cachePtr);
  CacheablePtr valuePtr = gemfireValueFromV8(args[1], cachePtr);

  if (args.Length() > 2 && args[2]->IsFunction()) {
    Local<Function> callback = Local<Function>::Cast(args[2]);

    if (valuePtr == NULLPTR) {
      Local<Value> error = NanNew(unableToPutValueError(args[1]));

      static const int argc = 2;
      Local<Value> argv[2] = { error, NanUndefined() };
      NanMakeCallback(NanGetCurrentContext()->Global(), callback, argc, argv);;
    } else {
      PutBaton * putBaton = new PutBaton(callback, region->regionPtr, keyPtr, valuePtr);

      uv_work_t * request = new uv_work_t();
      request->data = reinterpret_cast<void *>(putBaton);

      uv_queue_work(uv_default_loop(), request, region->AsyncPut, region->AfterAsyncPut);
    }

    NanReturnValue(args.This());
  } else {
    if (valuePtr == NULLPTR) {
      NanThrowError(unableToPutValueError(args[1]));
      NanReturnUndefined();
    }

    regionPtr->put(keyPtr, valuePtr);
    NanReturnValue(args[1]);
  }
}

void Region::AsyncPut(uv_work_t * request) {
  PutBaton * putBaton = reinterpret_cast<PutBaton *>(request->data);
  putBaton->regionPtr->put(putBaton->keyPtr, putBaton->valuePtr);
}

void Region::AfterAsyncPut(uv_work_t * request, int status) {
  NanScope();

  PutBaton * putBaton = reinterpret_cast<PutBaton *>(request->data);

  Local<Value> error = NanNull();
  Local<Value> returnValue = NanNew(v8ValueFromGemfire(putBaton->valuePtr));

  static const int argc = 2;
  Local<Value> argv[2] = { error, returnValue };
  NanMakeCallback(NanGetCurrentContext()->Global(), putBaton->callback, argc, argv);;

  delete request;
  delete putBaton;
}

NAN_METHOD(Region::Get) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;

  CacheableKeyPtr keyPtr = gemfireValueFromV8(args[0], region->regionPtr->getCache());

  if (args.Length() > 1 && args[1]->IsFunction()) {
    Local<Function> callback = Local<Function>::Cast(args[1]);
    GetBaton * getBaton = new GetBaton(callback, region->regionPtr, keyPtr);

    uv_work_t * request = new uv_work_t();
    request->data = reinterpret_cast<void *>(getBaton);

    uv_queue_work(uv_default_loop(), request, region->AsyncGet, region->AfterAsyncGet);

    NanReturnValue(args.This());
  } else {
    CacheablePtr valuePtr = regionPtr->get(keyPtr);
    NanReturnValue(v8ValueFromGemfire(valuePtr));
  }
}

void Region::AsyncGet(uv_work_t * request) {
  GetBaton * getBaton = reinterpret_cast<GetBaton *>(request->data);
  getBaton->valuePtr = getBaton->regionPtr->get(getBaton->keyPtr);
}

void Region::AfterAsyncGet(uv_work_t * request, int status) {
  NanScope();

  GetBaton * getBaton = reinterpret_cast<GetBaton *>(request->data);

  Local<Value> returnValue = NanNew(v8ValueFromGemfire(getBaton->valuePtr));

  Local<Value> error;
  if (returnValue->IsUndefined()) {
    error = NanError("Key not found in region.");
  } else {
    error = NanNull();
  }

  static const int argc = 2;
  Local<Value> argv[2] = { error, returnValue };
  NanMakeCallback(NanGetCurrentContext()->Global(), getBaton->callback, argc, argv);;

  delete request;
  delete getBaton;
}

NAN_METHOD(Region::RegisterAllKeys) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;

  regionPtr->registerAllKeys();

  NanReturnValue(NanTrue());
}

NAN_METHOD(Region::UnregisterAllKeys) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;

  regionPtr->unregisterAllKeys();

  NanReturnValue(NanTrue());
}

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

static void setCacheListener(RegionPtr regionPtr) {
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

NAN_METHOD(Region::OnPut) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr = region->regionPtr;

  setCacheListener(regionPtr);

  Local<Function> callback = Local<Function>::Cast(args[0]);

  Local<Array> putCallbacks =
    Local<Array>::Cast(callbacks->Get(NanNew("put")));

  putCallbacks->Set(putCallbacks->Length(), callback);

  NanReturnValue(NanNew(true));
}

NAN_METHOD(Region::ExecuteFunction) {
  NanScope();

  const unsigned int v8ArgsLength = args.Length();

  if (v8ArgsLength == 0 || args[0]->IsFunction()) {
    NanThrowError("You must provide the name of a function to execute.");
    NanReturnUndefined();
  }

  Local<Value> lastArgument = args[v8ArgsLength - 1];
  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  std::string functionName(*NanUtf8String(args[0]));
  CacheablePtr functionArguments = NULLPTR;

  if (v8ArgsLength > 1 && !args[1]->IsFunction()) {
    functionArguments = gemfireValueFromV8(args[1], region->regionPtr->getCache());
  }

  if (lastArgument->IsFunction()) {
    Local<Function> callback = Local<Function>::Cast(lastArgument);

    ExecuteFunctionBaton * baton = new ExecuteFunctionBaton(region->regionPtr,
                                                            functionName,
                                                            functionArguments,
                                                            callback);

    uv_work_t * request = new uv_work_t();
    request->data = reinterpret_cast<void *>(baton);

    uv_queue_work(uv_default_loop(),
                  request,
                  region->AsyncExecuteFunction,
                  region->AfterAsyncExecuteFunction);

    NanReturnValue(args.This());
  } else {
    ExecutionPtr executionPtr = FunctionService::onRegion(region->regionPtr);
    if (functionArguments != NULLPTR) {
      executionPtr = executionPtr->withArgs(functionArguments);
    }

    try {
      ResultCollectorPtr resultCollectorPtr = executionPtr->execute(functionName.c_str());
      CacheableVectorPtr resultsPtr = resultCollectorPtr->getResult();
      NanReturnValue(v8ValueFromGemfire(resultsPtr));
    }
    catch (gemfire::Exception &exception) {
      ThrowGemfireException(exception);
      NanReturnUndefined();
    }
  }
}

void Region::AsyncExecuteFunction(uv_work_t * request) {
  ExecuteFunctionBaton * baton = reinterpret_cast<ExecuteFunctionBaton *>(request->data);

  ExecutionPtr executionPtr = FunctionService::onRegion(baton->regionPtr);

  if (baton->functionArguments != NULLPTR) {
    executionPtr = executionPtr->withArgs(baton->functionArguments);
  }

  try {
    baton->resultsPtr = executionPtr->execute(baton->functionName.c_str())->getResult();
    baton->executionSucceded = true;
  }
  catch (gemfire::Exception &exception) {
    baton->exceptionPtr = new gemfire::Exception(exception);
    baton->executionSucceded = false;
  }
}

void Region::AfterAsyncExecuteFunction(uv_work_t * request, int status) {
  ExecuteFunctionBaton * baton = reinterpret_cast<ExecuteFunctionBaton *>(request->data);

  Local<Value> error;
  Local<Value> returnValue;

  if (baton->executionSucceded) {
    Handle<Array> resultsArray = v8ValueFromGemfire(baton->resultsPtr);
    error = NanNull();

    unsigned int length = resultsArray->Length();
    if (length > 0) {
      Handle<Value> lastResult = resultsArray->Get(length - 1);

      if (lastResult->IsNativeError()) {
        error = NanNew(lastResult);

        Local<Array> resultsExceptLast = NanNew<Array>(length - 1);
        for (unsigned int i = 0; i < length - 1; i++) {
          resultsExceptLast->Set(i, resultsArray->Get(i));
        }
        resultsArray = resultsExceptLast;
      }
    }

    returnValue = NanNew(resultsArray);
  } else {
    error = NanError(gemfireExceptionMessage(*(baton->exceptionPtr)).c_str());
    returnValue = NanUndefined();
  }

  const unsigned int argc = 2;
  Handle<Value> argv[argc] = { error, returnValue };
  Local<Context> ctx = NanGetCurrentContext();
  NanMakeCallback(ctx->Global(), baton->callback, argc, argv);

  delete request;
  delete baton;
}

}  // namespace node_gemfire
