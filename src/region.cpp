#include "region.hpp"
#include <gfcpp/Region.hpp>
#include <gfcpp/FunctionService.hpp>
#include <sstream>
#include <string>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "cache.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Persistent<FunctionTemplate> regionConstructor;

NAN_METHOD(Region::GetRegion) {
  NanScope();

  Local<Object> cacheHandle(args[0]->ToObject());

  Cache * cache = ObjectWrap::Unwrap<Cache>(cacheHandle);
  CachePtr cachePtr(cache->cachePtr);
  RegionPtr regionPtr(cachePtr->getRegion(*NanAsciiString(args[1])));

  if (regionPtr == NULLPTR) {
    NanReturnUndefined();
  }

  Region * region = new Region(cacheHandle, regionPtr);

  const unsigned int argc = 0;
  Handle<Value> argv[] = {};
  Local<Object> regionHandle(regionConstructor->GetFunction()->NewInstance(argc, argv));

  region->Wrap(regionHandle);

  NanReturnValue(regionHandle);
}

NAN_METHOD(Region::Clear) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);
  regionPtr->clear();

  NanReturnValue(NanTrue());
}

std::string unableToPutValueError(Handle<Value> v8Value) {
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to put value " << *String::Utf8Value(v8Value->ToDetailString());
  return errorMessageStream.str();
}

class PutWorker : public NanAsyncWorker {
 public:
  PutWorker(
    const RegionPtr & regionPtr,
    const CacheableKeyPtr & keyPtr,
    const CacheablePtr & valuePtr,
    NanCallback * callback) :
      NanAsyncWorker(callback),
      regionPtr(regionPtr),
      keyPtr(keyPtr),
      valuePtr(valuePtr) {}

  void Execute() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    try {
      regionPtr->put(keyPtr, valuePtr);
    } catch (gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
  }

  void Fail(std::string errorMessage) {
    SetErrorMessage(errorMessage.c_str());
    WorkComplete();
  }

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), NanNew(v8ValueFromGemfire(valuePtr)) };
    callback->Call(argc, argv);
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
  CacheablePtr valuePtr;
};

NAN_METHOD(Region::Put) {
  NanScope();

  if (args.Length() < 3) {
    NanThrowError("You must pass a key, value, and callback to put().");
    NanReturnUndefined();
  }

  if (!args[2]->IsFunction()) {
    NanThrowError("You must pass a callback to put().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);
  CachePtr cachePtr(regionPtr->getCache());

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], cachePtr));
  CacheablePtr valuePtr(gemfireValueFromV8(args[1], cachePtr));
  NanCallback * callback = new NanCallback(args[2].As<Function>());

  PutWorker * putWorker = new PutWorker(regionPtr, keyPtr, valuePtr, callback);

  if (valuePtr == NULLPTR) {
    putWorker->Fail(unableToPutValueError(args[1]));
  } else {
    NanAsyncQueueWorker(putWorker);
  }

  NanReturnValue(args.This());
}

class GetWorker : public NanAsyncWorker {
 public:
  GetWorker(NanCallback * callback,
           const RegionPtr & regionPtr,
           const CacheableKeyPtr & keyPtr) :
      NanAsyncWorker(callback),
      regionPtr(regionPtr),
      keyPtr(keyPtr) {}

  void Execute() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    try {
      valuePtr = regionPtr->get(keyPtr);
    } catch (gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }

    if (valuePtr == NULLPTR) {
      SetErrorMessage("Key not found in region.");
    }
  }

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[argc] = { NanUndefined(), NanNew(v8ValueFromGemfire(valuePtr)) };
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

class GetAllWorker : public NanAsyncWorker {
 public:
  GetAllWorker(
      const RegionPtr & regionPtr,
      const VectorOfCacheableKeyPtr & gemfireKeysPtr,
      NanCallback * callback) :
    NanAsyncWorker(callback),
    regionPtr(regionPtr),
    gemfireKeysPtr(gemfireKeysPtr) {}

  void Execute() {
    resultsPtr = new HashMapOfCacheable();

    if (gemfireKeysPtr->size() == 0) {
      return;
    }

    try {
      regionPtr->getAll(*gemfireKeysPtr, resultsPtr, NULLPTR);
    } catch (gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
  }

  void HandleOKCallback() {
    static const int argc = 2;

    Handle<Value> argv[argc] = { NanUndefined(), v8ValueFromGemfire(resultsPtr) };
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
      gemfireKeysFromV8(Handle<Array>::Cast(args[0]), regionPtr->getCache()));
  NanCallback * callback = new NanCallback(args[1].As<Function>());

  GetAllWorker * worker = new GetAllWorker(regionPtr, gemfireKeysPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class PutAllWorker : public NanAsyncWorker {
 public:
  PutAllWorker(
      const RegionPtr & regionPtr,
      const HashMapOfCacheablePtr & hashMapPtr,
      NanCallback * callback) :
    NanAsyncWorker(callback),
    regionPtr(regionPtr),
    hashMapPtr(hashMapPtr) {}

  void Execute() {
    try {
      regionPtr->putAll(*hashMapPtr);
    } catch (gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
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

  if (args.Length() == 1) {
    NanThrowError("You must pass a callback to putAll().");
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to putAll().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  HashMapOfCacheablePtr hashMapPtr(gemfireHashMapFromV8(args[0]->ToObject(), regionPtr->getCache()));

  NanCallback * callback = new NanCallback(args[1].As<Function>());
  PutAllWorker * worker = new PutAllWorker(regionPtr, hashMapPtr, callback);
  NanAsyncQueueWorker(worker);
  NanReturnValue(args.This());
}

class RemoveWorker : public NanAsyncWorker {
 public:
  RemoveWorker(
      const RegionPtr & regionPtr,
      const CacheableKeyPtr & keyPtr,
      NanCallback * callback) :
    NanAsyncWorker(callback),
    regionPtr(regionPtr),
    keyPtr(keyPtr) {}

  void Execute() {
    if (keyPtr == NULLPTR) {
      SetErrorMessage("Invalid GemFire key.");
      return;
    }

    try {
      regionPtr->destroy(keyPtr);
    } catch (const EntryNotFoundException & exception) {
      SetErrorMessage("Key not found in region.");
    } catch (const gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
  }

  RegionPtr regionPtr;
  CacheableKeyPtr keyPtr;
};

NAN_METHOD(Region::Remove) {
  NanScope();

  if (args.Length() < 2) {
    NanThrowError("You must pass a key and a callback to remove().");
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to remove().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);
  CachePtr cachePtr(regionPtr->getCache());

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], cachePtr));
  NanCallback * callback = new NanCallback(args[1].As<Function>());

  RemoveWorker * worker = new RemoveWorker(regionPtr, keyPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

class ExecuteFunctionWorker : public NanAsyncWorker {
 public:
  ExecuteFunctionWorker(
      const RegionPtr & regionPtr,
      const std::string & functionName,
      const CacheablePtr & functionArguments,
      NanCallback * callback) :
    NanAsyncWorker(callback),
    regionPtr(regionPtr),
    functionName(functionName),
    functionArguments(functionArguments) {}

  void Execute() {
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));

    if (functionArguments != NULLPTR) {
      executionPtr = executionPtr->withArgs(functionArguments);
    }

    try {
      resultsPtr = executionPtr->execute(functionName.c_str())->getResult();
    }
    catch (gemfire::Exception &exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
  }

  void HandleOKCallback() {
    Local<Value> error(NanUndefined());
    Local<Value> returnValue;

    Handle<Array> resultsArray(v8ValueFromGemfire(resultsPtr));

    unsigned int length = resultsArray->Length();
    if (length > 0) {
      Handle<Value> lastResult(resultsArray->Get(length - 1));

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
    Handle<Value> argv[argc] = { error, NanNew(resultsArray) };
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

    functionArguments = gemfireValueFromV8(args[1], regionPtr->getCache());
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
class AbstractQueryWorker : public NanAsyncWorker {
 public:
  AbstractQueryWorker(
      const RegionPtr & regionPtr,
      const std::string & queryPredicate,
      NanCallback * callback) :
    NanAsyncWorker(callback),
    regionPtr(regionPtr),
    queryPredicate(queryPredicate) {}

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), NanNew(v8ValueFromGemfire(resultPtr)) };
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

  void Execute() {
    try {
      resultPtr = regionPtr->query(queryPredicate.c_str());
    } catch(const gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
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

  void Execute() {
    try {
      resultPtr = regionPtr->selectValue(queryPredicate.c_str());
    } catch(const gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
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

  void Execute() {
    try {
      resultPtr = regionPtr->existsValue(queryPredicate.c_str());
    } catch(const gemfire::Exception & exception) {
      SetErrorMessage(gemfireExceptionMessage(exception).c_str());
    }
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

void Region::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>();

  constructor->SetClassName(NanNew("Region"));
  constructor->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(constructor, "clear",
      NanNew<FunctionTemplate>(Region::Clear)->GetFunction());
  NanSetPrototypeTemplate(constructor, "put",
      NanNew<FunctionTemplate>(Region::Put)->GetFunction());
  NanSetPrototypeTemplate(constructor, "get",
      NanNew<FunctionTemplate>(Region::Get)->GetFunction());
  NanSetPrototypeTemplate(constructor, "getAll",
      NanNew<FunctionTemplate>(Region::GetAll)->GetFunction());
  NanSetPrototypeTemplate(constructor, "putAll",
      NanNew<FunctionTemplate>(Region::PutAll)->GetFunction());
  NanSetPrototypeTemplate(constructor, "remove",
      NanNew<FunctionTemplate>(Region::Remove)->GetFunction());
  NanSetPrototypeTemplate(constructor, "query",
      NanNew<FunctionTemplate>(Region::Query<QueryWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructor, "selectValue",
      NanNew<FunctionTemplate>(Region::Query<SelectValueWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructor, "existsValue",
      NanNew<FunctionTemplate>(Region::Query<ExistsValueWorker>)->GetFunction());
  NanSetPrototypeTemplate(constructor, "executeFunction",
      NanNew<FunctionTemplate>(Region::ExecuteFunction)->GetFunction());
  NanSetPrototypeTemplate(constructor, "inspect",
      NanNew<FunctionTemplate>(Region::Inspect)->GetFunction());

  constructor->PrototypeTemplate()->SetAccessor(NanNew("name"), Region::Name);

  NanAssignPersistent(regionConstructor, constructor);
}

}  // namespace node_gemfire
