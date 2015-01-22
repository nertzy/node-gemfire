#include "region.hpp"
#include <gfcpp/Region.hpp>
#include <uv.h>
#include <sstream>
#include <string>
#include <vector>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "cache.hpp"
#include "gemfire_worker.hpp"
#include "events.hpp"
#include "functions.hpp"
#include "region_event_registry.hpp"
#include "dependencies.hpp"

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

  Local<Object> regionObject(NanNew(Region::constructor)->NewInstance(0, NULL));
  node_gemfire::Region * region =
    new node_gemfire::Region(regionObject, cacheObject, regionPtr);

  RegionEventRegistry::getInstance()->add(region);

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

    if (callback) {
      static const int argc = 1;
      Local<Value> argv[argc] = { errorObject() };
      callback->Call(argc, argv);
    } else {
      emitError(GetFromPersistent("v8Object"), errorObject());
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
    // Workaround: We don't want to call clear on the region if the cache is closed.
    // After cache is cleared, getCache() will throw an exception, whereas clear() causes a segfault.
    region->regionPtr->getCache();

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
  } catch (const RegionDestroyedException & exception) {
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
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    if (valuePtr == NULLPTR) {
      SetError("InvalidValueError", "Invalid GemFire value.");
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
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    valuePtr = regionPtr->get(keyPtr);

    if (valuePtr == NULLPTR) {
      SetError("KeyNotFoundError", "Key not found in region.");
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
      SetError("InvalidKeyError", "Invalid GemFire key.");
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
      SetError("InvalidValueError", "Invalid GemFire value.");
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
      SetError("InvalidKeyError", "Invalid GemFire key.");
      return;
    }

    try {
      regionPtr->destroy(keyPtr);
    } catch (const EntryNotFoundException & exception) {
      SetError("KeyNotFoundError", "Key not found in region.");
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

NAN_METHOD(Region::ExecuteFunction) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CachePtr cachePtr(getCacheFromRegion(region->regionPtr));
  if (cachePtr == NULLPTR) {
    NanReturnUndefined();
  }

  try {
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));
    NanReturnValue(executeFunction(args, cachePtr, executionPtr));
  } catch (const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
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

NAN_GETTER(Region::Attributes) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  RegionAttributesPtr regionAttributesPtr(regionPtr->getAttributes());

  Local<Object> returnValue(NanNew<Object>());

  returnValue->Set(NanNew("cachingEnabled"),
      NanNew(regionAttributesPtr->getCachingEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("clientNotificationEnabled"),
      NanNew(regionAttributesPtr->getClientNotificationEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("concurrencyChecksEnabled"),
      NanNew(regionAttributesPtr->getConcurrencyChecksEnabled()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("concurrencyLevel"),
      NanNew(regionAttributesPtr->getConcurrencyLevel()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("diskPolicy"),
      NanNew(DiskPolicyType::fromOrdinal(regionAttributesPtr->getDiskPolicy())),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("entryIdleTimeout"),
      NanNew(regionAttributesPtr->getEntryIdleTimeout()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("entryTimeToLive"),
      NanNew(regionAttributesPtr->getEntryTimeToLive()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("initialCapacity"),
      NanNew(regionAttributesPtr->getInitialCapacity()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("loadFactor"),
      NanNew(regionAttributesPtr->getLoadFactor()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("lruEntriesLimit"),
      NanNew(regionAttributesPtr->getLruEntriesLimit()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("lruEvicationAction"),
      NanNew(ExpirationAction::fromOrdinal(regionAttributesPtr->getLruEvictionAction())),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  const char * poolName = regionAttributesPtr->getPoolName();
  if (poolName == NULL) {
    returnValue->Set(NanNew("poolName"),
        NanNull(),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete));
  } else {
    returnValue->Set(NanNew("poolName"),
        NanNew(poolName),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete));
  }

  returnValue->Set(NanNew("regionIdleTimeout"),
      NanNew(regionAttributesPtr->getRegionIdleTimeout()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("regionTimeToLive"),
      NanNew(regionAttributesPtr->getRegionTimeToLive()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  returnValue->Set(NanNew("scope"),
      NanNew(ScopeType::fromOrdinal(regionAttributesPtr->getScope())),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  NanReturnValue(returnValue);
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

class ServerKeysWorker : public GemfireWorker {
 public:
  ServerKeysWorker(
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

NAN_METHOD(Region::ServerKeys) {
  NanScope();

  if (args.Length() == 0) {
    NanThrowError("You must pass a callback to serverKeys().");
    NanReturnUndefined();
  }

  if (!args[0]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to serverKeys().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  NanCallback * callback = new NanCallback(args[0].As<Function>());

  ServerKeysWorker * worker = new ServerKeysWorker(region->regionPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnUndefined();
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
    regionPtr->keys(*keysVectorPtr);
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

NAN_METHOD(Region::RegisterAllKeys) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  try {
    region->regionPtr->registerAllKeys();
  } catch (const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
  }

  NanReturnUndefined();
}

NAN_METHOD(Region::UnregisterAllKeys) {
  NanScope();

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  try {
    region->regionPtr->unregisterAllKeys();
  } catch (const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
  }

  NanReturnUndefined();
}

class ValuesWorker : public GemfireWorker {
 public:
  ValuesWorker(
      const RegionPtr & regionPtr,
      NanCallback * callback) :
    GemfireWorker(callback),
    regionPtr(regionPtr) {}

  void ExecuteGemfireWork() {
    valuesVectorPtr = new VectorOfCacheable();
    regionPtr->values(*valuesVectorPtr);
  }

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(valuesVectorPtr) };
    callback->Call(argc, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfCacheablePtr valuesVectorPtr;
};

NAN_METHOD(Region::Values) {
  NanScope();

  if (args.Length() == 0) {
    NanThrowError("You must pass a callback to values().");
    NanReturnUndefined();
  }

  if (!args[0]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to values().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  NanCallback * callback = new NanCallback(args[0].As<Function>());

  ValuesWorker * worker = new ValuesWorker(region->regionPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnUndefined();
}

class EntriesWorker : public GemfireWorker {
 public:
  EntriesWorker(
      const RegionPtr & regionPtr,
      NanCallback * callback,
      bool recursive = true) :
    GemfireWorker(callback),
    regionPtr(regionPtr),
    recursive(recursive) {}

  void ExecuteGemfireWork() {
    regionEntryVector = new VectorOfRegionEntry();
    regionPtr->entries(*regionEntryVector, recursive);
  }

  void HandleOKCallback() {
    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(*regionEntryVector) };
    callback->Call(argc, argv);
  }

 private:
  RegionPtr regionPtr;
  VectorOfRegionEntry* regionEntryVector;
  bool recursive;
};


NAN_METHOD(Region::Entries) {
  NanScope();

  if (args.Length() == 0) {
    NanThrowError("You must pass a callback to entries().");
    NanReturnUndefined();
  }

  if (!args[0]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to entries().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  NanCallback * callback = new NanCallback(args[0].As<Function>());

  EntriesWorker * worker = new EntriesWorker(region->regionPtr, callback, true);
  NanAsyncQueueWorker(worker);

  NanReturnUndefined();
}

class DestroyRegionWorker : public GemfireEventedWorker {
 public:
  DestroyRegionWorker(
    const Local<Object> & regionObject,
    Region * region,
    NanCallback * callback,
    bool local = true) :
      GemfireEventedWorker(regionObject, callback),
      region(region),
      local(local) {}

  void ExecuteGemfireWork() {
    if (local) {
      region->regionPtr->localDestroyRegion();
    } else {
      region->regionPtr->destroyRegion();
    }
  }

 private:
  Region * region;
  bool local;
};

NAN_METHOD(Region::DestroyRegion) {
  NanScope();

  if (!isFunctionOrUndefined(args[0])) {
    NanThrowError("You must pass a function as the callback to destroyRegion().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());

  NanCallback * callback = getCallback(args[0]);
  DestroyRegionWorker * worker = new DestroyRegionWorker(args.This(), region, callback, false);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

NAN_METHOD(Region::LocalDestroyRegion) {
  NanScope();

  if (!isFunctionOrUndefined(args[0])) {
    NanThrowError("You must pass a function as the callback to localDestroyRegion().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());

  NanCallback * callback = getCallback(args[0]);
  DestroyRegionWorker * worker = new DestroyRegionWorker(args.This(), region, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
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
  NanSetPrototypeTemplate(constructorTemplate, "entries",
      NanNew<FunctionTemplate>(Region::Entries)->GetFunction());
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
  NanSetPrototypeTemplate(constructorTemplate, "serverKeys",
      NanNew<FunctionTemplate>(Region::ServerKeys)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "keys",
      NanNew<FunctionTemplate>(Region::Keys)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "values",
      NanNew<FunctionTemplate>(Region::Values)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "inspect",
      NanNew<FunctionTemplate>(Region::Inspect)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "registerAllKeys",
      NanNew<FunctionTemplate>(Region::RegisterAllKeys)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "unregisterAllKeys",
      NanNew<FunctionTemplate>(Region::UnregisterAllKeys)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "destroyRegion",
      NanNew<FunctionTemplate>(Region::DestroyRegion)->GetFunction());
  NanSetPrototypeTemplate(constructorTemplate, "localDestroyRegion",
      NanNew<FunctionTemplate>(Region::LocalDestroyRegion)->GetFunction());

  constructorTemplate->PrototypeTemplate()->SetAccessor(NanNew("name"), Region::Name);
  constructorTemplate->PrototypeTemplate()->SetAccessor(NanNew("attributes"), Region::Attributes);

  NanAssignPersistent(Region::constructor, constructorTemplate->GetFunction());
  exports->Set(NanNew("Region"), NanNew(Region::constructor));
}

}  // namespace node_gemfire
