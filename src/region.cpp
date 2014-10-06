#include "region.hpp"
#include <gfcpp/Region.hpp>
#include <gfcpp/FunctionService.hpp>
#include <uv.h>
#include <sstream>
#include <string>
#include "dependencies.hpp"
#include "conversions.hpp"
#include "exceptions.hpp"
#include "cache.hpp"
#include "gemfire_worker.hpp"
#include "streaming_result_collector.hpp"
#include "events.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Persistent<Function> Region::constructor;

inline bool isFunctionOrUndefined(const Local<Value> & value) {
  return value->IsUndefined() || value->IsFunction();
}

inline NanCallback * getCallback(const Local<Value> & value) {
  if (value->IsUndefined()) {
    return NULL;
  }
  return new NanCallback(Local<Function>::Cast(value));
}

Local<Value> Region::New(Local<Object> cacheObject, RegionPtr regionPtr) {
  NanEscapableScope();

  if (regionPtr == NULLPTR) {
    return NanEscapeScope(NanUndefined());
  }

  Region * region = new Region(cacheObject, regionPtr);
  Local<Object> regionObject(NanNew(Region::constructor)->NewInstance(0, NULL));

  region->Wrap(regionObject);

  return NanEscapeScope(regionObject);
}

class GemfireEventedWorker : public GemfireWorker {
 public:
  GemfireEventedWorker(
    const Local<Object> & v8Object,
    NanCallback * callback) :
      GemfireWorker(callback) {
        SaveToPersistent("v8Object", v8Object);
      }

  virtual void HandleOKCallback() {
    if (callback) {
      callback->Call(0, NULL);
    }
  }

  virtual void HandleErrorCallback() {
    NanScope();

    Local<Value> error(NanError(ErrorMessage()));

    if (callback) {
      static const int argc = 1;
      v8::Local<v8::Value> argv[argc] = { error };
      callback->Call(argc, argv);
    } else {
      emitError(GetFromPersistent("v8Object"), error);
    }
  }
};

class ClearWorker : public GemfireEventedWorker {
 public:
  ClearWorker(
    const Local<Object> & regionObject,
    Region * region,
    NanCallback * callback) :
      GemfireEventedWorker(regionObject, callback),
      region(region) {}

  void ExecuteGemfireWork() {
    region->regionPtr->clear();
  }

  Region * region;
};

NAN_METHOD(Region::Clear) {
  NanScope();

  if (!isFunctionOrUndefined(args[0])) {
    NanThrowError("You must pass a function as the callback to clear().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  NanCallback * callback = getCallback(args[0]);
  ClearWorker * worker = new ClearWorker(args.This(), region, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

std::string unableToPutValueError(Local<Value> v8Value) {
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to put value " << *String::Utf8Value(v8Value->ToDetailString());
  return errorMessageStream.str();
}

CachePtr getCacheFromRegion(RegionPtr regionPtr) {
  NanScope();

  try {
    return regionPtr->getCache();
  } catch (const RegionDestroyedException exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

class PutWorker : public GemfireEventedWorker {
 public:
  PutWorker(
    const Local<Object> & regionObject,
    Region * region,
    const CacheableKeyPtr & keyPtr,
    const CacheablePtr & valuePtr,
    NanCallback * callback) :
      GemfireEventedWorker(regionObject, callback),
      region(region),
      keyPtr(keyPtr),
      valuePtr(valuePtr) { }

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    if (valuePtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire value.");
      return;
    }

    region->regionPtr->put(keyPtr, valuePtr);
  }

  Region * region;
  CacheableKeyPtr keyPtr;
  CacheablePtr valuePtr;
};

NAN_METHOD(Region::Put) {
  NanScope();

  if (args.Length() < 2) {
    NanThrowError("You must pass a key and value to put().");
    NanReturnUndefined();
  }

  if (!isFunctionOrUndefined(args[2])) {
    NanThrowError("You must pass a function as the callback to put().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  CacheableKeyPtr keyPtr(gemfireKey(args[0], cachePtr));
  CacheablePtr valuePtr(gemfireValue(args[1], cachePtr));
  NanCallback * callback = getCallback(args[2]);
  PutWorker * putWorker = new PutWorker(args.This(), region, keyPtr, valuePtr, callback);
  NanAsyncQueueWorker(putWorker);

  NanReturnValue(args.This());
}

class GetWorker : public GemfireWorker {
 public:
  GetWorker(NanCallback * callback,
           const RegionPtr & regionPtr,
           const CacheableKeyPtr & keyPtr) :
      GemfireWorker(callback),
      regionPtr(regionPtr),
      keyPtr(keyPtr) {}

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    valuePtr = regionPtr->get(keyPtr);

    if (valuePtr == NULLPTR) {
      SetErrorMessage("Key not found in region.");
    }
  }

  void HandleOKCallback() {
    NanScope();

    static const int argc = 2;
    Local<Value> argv[argc] = { NanUndefined(), v8Value(valuePtr) };
    callback->Call(argc, argv);
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
  CacheablePtr valuePtr;
};

NAN_METHOD(Region::Get) {
  NanScope();

  unsigned int argsLength = args.Length();

  if (argsLength == 0) {
    NanThrowError("You must pass a key and callback to get().");
    NanReturnUndefined();
  }

  if (argsLength == 1) {
    NanThrowError("You must pass a callback to get().");
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    NanThrowError("The second argument to get() must be a callback.");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  CacheableKeyPtr keyPtr(gemfireKey(args[0], cachePtr));

  NanCallback * callback = new NanCallback(args[1].As<Function>());
  GetWorker * getWorker = new GetWorker(callback, regionPtr, keyPtr);
  NanAsyncQueueWorker(getWorker);

  NanReturnValue(args.This());
}

class GetAllWorker : public GemfireWorker {
 public:
  GetAllWorker(
      const RegionPtr & regionPtr,
      const VectorOfCacheableKeyPtr & gemfireKeysPtr,
      NanCallback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    gemfireKeysPtr(gemfireKeysPtr) {}

  void ExecuteGemfireWork() {
    resultsPtr = new HashMapOfCacheable();

    if (gemfireKeysPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    if (gemfireKeysPtr->size() == 0) {
      return;
    }

    regionPtr->getAll(*gemfireKeysPtr, resultsPtr, NULLPTR);
  }

  void HandleOKCallback() {
    NanScope();

    static const int argc = 2;
    Local<Value> argv[argc] = { NanUndefined(), v8Value(resultsPtr) };
    callback->Call(argc, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheableKeyPtr gemfireKeysPtr;
  HashMapOfCacheablePtr resultsPtr;
};

NAN_METHOD(Region::GetAll) {
  NanScope();

  if (args.Length() == 0 || !args[0]->IsArray()) {
    NanThrowError("You must pass an array of keys and a callback to getAll().");
    NanReturnUndefined();
  }

  if (args.Length() == 1) {
    NanThrowError("You must pass a callback to getAll().");
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to getAll().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  VectorOfCacheableKeyPtr gemfireKeysPtr(gemfireKeys(Local<Array>::Cast(args[0]), cachePtr));

  NanCallback * callback = new NanCallback(args[1].As<Function>());

  GetAllWorker * worker = new GetAllWorker(regionPtr, gemfireKeysPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class PutAllWorker : public GemfireEventedWorker {
 public:
  PutAllWorker(
      const Local<Object> & regionObject,
      const RegionPtr & regionPtr,
      const HashMapOfCacheablePtr & hashMapPtr,
      NanCallback * callback) :
    GemfireEventedWorker(regionObject, callback),
    regionPtr(regionPtr),
    hashMapPtr(hashMapPtr) { }

  void ExecuteGemfireWork() {
    if (hashMapPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire value.");
      return;
    }

    regionPtr->putAll(*hashMapPtr);
  }

 private:
  RegionPtr regionPtr;
  HashMapOfCacheablePtr hashMapPtr;
};

NAN_METHOD(Region::PutAll) {
  NanScope();

  if (args.Length() == 0 || !args[0]->IsObject()) {
    NanThrowError("You must pass an object and a callback to putAll().");
    NanReturnUndefined();
  }

  if (!isFunctionOrUndefined(args[1])) {
    NanThrowError("You must pass a function as the callback to putAll().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  HashMapOfCacheablePtr hashMapPtr(gemfireHashMap(args[0]->ToObject(), cachePtr));
  NanCallback * callback = getCallback(args[1]);
  PutAllWorker * worker = new PutAllWorker(args.This(), regionPtr, hashMapPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class RemoveWorker : public GemfireEventedWorker {
 public:
  RemoveWorker(
      const Local<Object> & regionObject,
      const RegionPtr & regionPtr,
      const CacheableKeyPtr & keyPtr,
      NanCallback * callback) :
    GemfireEventedWorker(regionObject, callback),
    regionPtr(regionPtr),
    keyPtr(keyPtr) {}

  void ExecuteGemfireWork() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    try {
      regionPtr->destroy(keyPtr);
    } catch (const EntryNotFoundException & exception) {
      SetErrorMessage("Key not found in region.");
    }
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
};

NAN_METHOD(Region::Remove) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError("You must pass a key to remove().");
    NanReturnUndefined();
  }

  if (!isFunctionOrUndefined(args[1])) {
    NanThrowError("You must pass a function as the callback to remove().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  CacheableKeyPtr keyPtr(gemfireKey(args[0], cachePtr));
  NanCallback * callback = getCallback(args[1]);
  RemoveWorker * worker = new RemoveWorker(args.This(), regionPtr, keyPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class ExecuteFunctionWorker {
 public:
  ExecuteFunctionWorker(
      const RegionPtr & regionPtr,
      const std::string & functionName,
      const CacheablePtr & functionArguments,
      const CacheableVectorPtr & functionFilter,
      const Local<Object> & emitter) :
    resultStream(new ResultStream(this, DataAsyncCallback, EndAsyncCallback)),
    regionPtr(regionPtr),
    functionName(functionName),
    functionArguments(functionArguments),
    functionFilter(functionFilter) {
      NanAssignPersistent(this->emitter, emitter);
      request.data = reinterpret_cast<void *>(this);
    }

  ~ExecuteFunctionWorker() {
    NanDisposePersistent(emitter);
  }

  static void Execute(uv_work_t * request) {
    ExecuteFunctionWorker * worker = static_cast<ExecuteFunctionWorker *>(request->data);
    worker->Execute();
  }

  static void ExecuteComplete(uv_work_t * request, int status) {
    ExecuteFunctionWorker * worker = static_cast<ExecuteFunctionWorker *>(request->data);
    worker->ExecuteComplete();
    delete worker;
  }

  static void DataAsyncCallback(uv_async_t * async) {
    ExecuteFunctionWorker * worker = reinterpret_cast<ExecuteFunctionWorker *>(async->data);
    worker->Data();
  }

  static void DataAsyncCallback(uv_async_t * async, int status) {
    DataAsyncCallback(async);
  }

  static void EndAsyncCallback(uv_async_t * async) {
    ExecuteFunctionWorker * worker = reinterpret_cast<ExecuteFunctionWorker *>(async->data);
    worker->End();
  }

  static void EndAsyncCallback(uv_async_t * async, int status) {
    EndAsyncCallback(async);
  }

  void Execute() {
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));

    if (functionArguments != NULLPTR) {
      executionPtr = executionPtr->withArgs(functionArguments);
    }

    if (functionFilter != NULLPTR) {
      executionPtr = executionPtr->withFilter(functionFilter);
    }

    ResultCollectorPtr resultCollectorPtr(new StreamingResultCollector(resultStream));
    executionPtr = executionPtr->withCollector(resultCollectorPtr);

    try {
      executionPtr->execute(functionName.c_str());
      resultStream->waitUntilFinished();
    } catch (const gemfire::Exception & exception) {
      errorMessage = gemfireExceptionMessage(exception);
    }
  }

  void ExecuteComplete() {
    if (!errorMessage.empty()) {
      NanScope();
      emitError(NanNew(emitter), NanError(errorMessage.c_str()));
    }
  }

  void Data() {
    NanScope();

    Local<Object> eventEmitter(NanNew(emitter));

    CacheableVectorPtr resultsPtr(resultStream->nextResults());
    for (CacheableVector::Iterator iterator(resultsPtr->begin());
         iterator != resultsPtr->end();
         ++iterator) {
      Local<Value> result(v8Value(*iterator));

      if (result->IsNativeError()) {
        emitError(eventEmitter, result);
      } else {
        emitEvent(eventEmitter, "data", result);
      }
    }

    resultStream->resultsProcessed();
  }

  void End() {
    NanScope();

    emitEvent(NanNew(emitter), "end");
    resultStream->endProcessed();
  }

  uv_work_t request;

 private:
  ResultStreamPtr resultStream;

  RegionPtr regionPtr;
  std::string functionName;
  CacheablePtr functionArguments;
  CacheableVectorPtr functionFilter;
  Persistent<Object> emitter;
  std::string errorMessage;
};

NAN_METHOD(Region::ExecuteFunction) {
  NanScope();

  if (args.Length() == 0 || !args[0]->IsString()) {
    NanThrowError("You must provide the name of a function to execute.");
    NanReturnUndefined();
  }

  Local<Value> v8FunctionArguments;
  Local<Value> v8FunctionFilter;

  if (args[1]->IsArray()) {
    v8FunctionArguments = args[1];
  } else if (args[1]->IsObject()) {
    Local<Object> optionsObject(args[1]->ToObject());
    v8FunctionArguments = optionsObject->Get(NanNew("arguments"));
    v8FunctionFilter = optionsObject->Get(NanNew("filter"));

    if (!v8FunctionFilter->IsArray() && !v8FunctionFilter->IsUndefined()) {
      NanThrowError("You must pass an Array of keys as the filter for executeFunction().");
      NanReturnUndefined();
    }
  } else if (!args[1]->IsUndefined()) {
    NanThrowError("You must pass either an Array of arguments or an options Object to executeFunction().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  std::string functionName(*NanUtf8String(args[0]));

  CacheablePtr functionArguments;
  if (v8FunctionArguments.IsEmpty() || v8FunctionArguments->IsUndefined()) {
    functionArguments = NULLPTR;
  } else {
    functionArguments = gemfireValue(v8FunctionArguments, cachePtr);
  }

  CacheableVectorPtr functionFilter;
  if (v8FunctionFilter.IsEmpty() || v8FunctionFilter->IsUndefined()) {
    functionFilter = NULLPTR;
  } else {
    functionFilter = gemfireVector(v8FunctionFilter.As<Array>(), cachePtr);
  }

  Local<Function> eventEmitterConstructor(NanNew(dependencies)->Get(NanNew("EventEmitter")).As<Function>());
  Local<Object> eventEmitter(eventEmitterConstructor->NewInstance());

  ExecuteFunctionWorker * worker =
    new ExecuteFunctionWorker(regionPtr, functionName, functionArguments, functionFilter, eventEmitter);

  uv_queue_work(
      uv_default_loop(),
      &worker->request,
      ExecuteFunctionWorker::Execute,
      ExecuteFunctionWorker::ExecuteComplete);

  NanReturnValue(eventEmitter);
}

NAN_METHOD(Region::Inspect) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  const char * name = regionPtr->getName();

  std::stringstream inspectStream;
  inspectStream << "[Region name=\"" << name << "\"]";
  NanReturnValue(NanNew(inspectStream.str().c_str()));
}

NAN_GETTER(Region::Name) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  NanReturnValue(NanNew(regionPtr->getName()));
}

template <typename T>
class AbstractQueryWorker : public GemfireWorker {
 public:
  AbstractQueryWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      NanCallback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    queryPredicate(queryPredicate) {}

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(resultPtr) };
    callback->Call(argc, argv);
  }

  RegionPtr regionPtr;
  std::string queryPredicate;
  T resultPtr;
};

class QueryWorker : public AbstractQueryWorker<SelectResultsPtr> {
 public:
  QueryWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      NanCallback * callback) :
    AbstractQueryWorker<SelectResultsPtr>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->query(queryPredicate.c_str());
  }

  static std::string name() {
    return "query()";
  }
};

class SelectValueWorker : public AbstractQueryWorker<CacheablePtr> {
 public:
  SelectValueWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      NanCallback * callback) :
    AbstractQueryWorker<CacheablePtr>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->selectValue(queryPredicate.c_str());
  }

  static std::string name() {
    return "selectValue()";
  }
};

class ExistsValueWorker : public AbstractQueryWorker<bool> {
 public:
  ExistsValueWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      NanCallback * callback) :
    AbstractQueryWorker<bool>(regionPtr, queryPredicate, callback) {}

  void ExecuteGemfireWork() {
    resultPtr = regionPtr->existsValue(queryPredicate.c_str());
  }

  static std::string name() {
    return "existsValue()";
  }
};

template<typename T>
NAN_METHOD(Region::Query) {
  NanScope();

  if (args.Length() < 2) {
    std::stringstream errorStream;
    errorStream << "You must pass a query predicate string and a callback to " << T::name() << ".";
    NanThrowError(errorStream.str().c_str());
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    std::stringstream errorStream;
    errorStream << "You must pass a function as the callback to " << T::name() << ".";
    NanThrowError(errorStream.str().c_str());
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());

  std::string queryPredicate(*NanUtf8String(args[0]));
  NanCallback * callback = new NanCallback(args[1].As<Function>());

  T * worker = new T(region->regionPtr, queryPredicate, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class KeysWorker : public GemfireWorker {
 public:
  KeysWorker(
      const RegionPtr & regionPtr,
      NanCallback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr) {}

  void ExecuteGemfireWork() {
    keysVectorPtr = new VectorOfCacheableKey();
    regionPtr->serverKeys(*keysVectorPtr);
  }

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(keysVectorPtr) };
    callback->Call(argc, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheableKeyPtr keysVectorPtr;
};

NAN_METHOD(Region::Keys) {
  NanScope();

  if (args.Length() == 0) {
    NanThrowError("You must pass a callback to keys().");
    NanReturnUndefined();
  }

  if (!args[0]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to keys().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  NanCallback * callback = new NanCallback(args[0].As<Function>());

  KeysWorker * worker = new KeysWorker(region->regionPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnUndefined();
}

void Region::Init(Local<Object> exports) {
  NanScope();

  Local<FunctionTemplate> constructorTemplate = NanNew<FunctionTemplate>();

  constructorTemplate->SetClassName(NanNew("Region"));
  constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(constructorTemplate, "clear",
      NanNew<FunctionTemplate>(Region::Clear)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "put",
      NanNew<FunctionTemplate>(Region::Put)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "get",
      NanNew<FunctionTemplate>(Region::Get)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "getAll",
      NanNew<FunctionTemplate>(Region::GetAll)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "putAll",
      NanNew<FunctionTemplate>(Region::PutAll)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "remove",
      NanNew<FunctionTemplate>(Region::Remove)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "query",
      NanNew<FunctionTemplate>(Region::Query<QueryWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "selectValue",
      NanNew<FunctionTemplate>(Region::Query<SelectValueWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "existsValue",
      NanNew<FunctionTemplate>(Region::Query<ExistsValueWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "executeFunction",
      NanNew<FunctionTemplate>(Region::ExecuteFunction)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "keys",
      NanNew<FunctionTemplate>(Region::Keys)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "inspect",
      NanNew<FunctionTemplate>(Region::Inspect)->GetFunction());

  constructorTemplate->PrototypeTemplate()->SetAccessor(NanNew("name"), Region::Name);

  NanAssignPersistent(Region::constructor, constructorTemplate->GetFunction());
  exports->Set(NanNew("Region"), NanNew(Region::constructor));
}

}  // namespace node_gemfire
