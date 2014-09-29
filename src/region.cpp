#include "region.hpp"
#include <gfcpp/Region.hpp>
#include <gfcpp/FunctionService.hpp>
#include <sstream>
#include <string>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "cache.hpp"
#include "gemfire_worker.hpp"

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
  CachePtr cachePtr(region->regionPtr->getCache());

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], cachePtr));
  CacheablePtr valuePtr(gemfireValueFromV8(args[1], cachePtr));
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
    Local<Value> argv[argc] = { NanUndefined(), v8ValueFromGemfire(valuePtr) };
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
  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], regionPtr->getCache()));

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
    Local<Value> argv[argc] = { NanUndefined(), v8ValueFromGemfire(resultsPtr) };
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

  VectorOfCacheableKeyPtr gemfireKeysPtr(
      gemfireKeysFromV8(Local<Array>::Cast(args[0]), regionPtr->getCache()));

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

  HashMapOfCacheablePtr hashMapPtr(gemfireHashMapFromV8(args[0]->ToObject(), regionPtr->getCache()));
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
  CachePtr cachePtr(regionPtr->getCache());

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], cachePtr));
  NanCallback * callback = getCallback(args[1]);
  RemoveWorker * worker = new RemoveWorker(args.This(), regionPtr, keyPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class ExecuteFunctionWorker : public GemfireWorker {
 public:
  ExecuteFunctionWorker(
      const RegionPtr & regionPtr,
      const std::string & functionName,
      const CacheablePtr & functionArguments,
      NanCallback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    functionName(functionName),
    functionArguments(functionArguments) {}

  void ExecuteGemfireWork() {
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));

    if (functionArguments != NULLPTR) {
      executionPtr = executionPtr->withArgs(functionArguments);
    }

    resultsPtr = executionPtr->execute(functionName.c_str())->getResult();
  }

  void HandleOKCallback() {
    NanScope();

    Local<Value> error(NanUndefined());
    Local<Value> returnValue;

    Local<Array> resultsArray(v8ValueFromGemfire(resultsPtr));

    unsigned int length = resultsArray->Length();
    if (length > 0) {
      Local<Value> lastResult(resultsArray->Get(length - 1));

      if (lastResult->IsNativeError()) {
        error = NanNew(lastResult);

        Local<Array> resultsExceptLast(NanNew<Array>(length - 1));
        for (unsigned int i = 0; i < length - 1; i++) {
          resultsExceptLast->Set(i, resultsArray->Get(i));
        }
        resultsArray = resultsExceptLast;
      }
    }

    const unsigned int argc = 2;
    Local<Value> argv[argc] = { error, resultsArray };
    callback->Call(argc, argv);
  }

  RegionPtr regionPtr;
  std::string functionName;
  CacheablePtr functionArguments;
  CacheableVectorPtr resultsPtr;
};

NAN_METHOD(Region::ExecuteFunction) {
  NanScope();

  const unsigned int v8ArgsLength = args.Length();

  if (v8ArgsLength == 0 || args[0]->IsFunction()) {
    NanThrowError("You must provide the name of a function to execute.");
    NanReturnUndefined();
  }

  if (v8ArgsLength == 1) {
    NanThrowError("You must pass a callback to executeFunction().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CacheablePtr functionArguments;
  NanCallback * callback;

  if (args[1]->IsFunction()) {
    functionArguments = NULLPTR;
    callback = new NanCallback(args[1].As<Function>());
  } else {
    if (v8ArgsLength == 2) {
      NanThrowError("You must pass a callback to executeFunction().");
      NanReturnUndefined();
    } else if (!args[2]->IsFunction()) {
      NanThrowError("You must pass a function as the callback to executeFunction().");
      NanReturnUndefined();
    }

    Local<Value> v8Arguments;

    if (args[1]->IsArray() || !args[1]->IsObject()) {
      v8Arguments = args[1];
    } else {
      v8Arguments = args[1]->ToObject()->Get(NanNew("arguments"));
    }

    functionArguments = gemfireValueFromV8(v8Arguments, regionPtr->getCache());

    callback = new NanCallback(args[2].As<Function>());
  }

  std::string functionName(*NanUtf8String(args[0]));

  ExecuteFunctionWorker * worker =
    new ExecuteFunctionWorker(regionPtr, functionName, functionArguments, callback);

  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
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
    Local<Value> argv[2] = { NanUndefined(), v8ValueFromGemfire(resultPtr) };
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
    Local<Value> argv[2] = { NanUndefined(), v8ValueFromGemfire(keysVectorPtr) };
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
