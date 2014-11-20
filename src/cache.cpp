#include "cache.hpp"
#include <v8.h>
#include <nan.h>
#include <gfcpp/Cache.hpp>
#include <gfcpp/CacheFactory.hpp>
#include <gfcpp/Region.hpp>
#include "exceptions.hpp"
#include "conversions.hpp"
#include "region.hpp"
#include "gemfire_worker.hpp"
#include "dependencies.hpp"
#include "functions.hpp"
#include "region_shortcuts.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

void Cache::Init(Local<Object> exports) {
  NanScope();

  Local<FunctionTemplate> cacheConstructorTemplate =
    NanNew<FunctionTemplate>(Cache::New);

  cacheConstructorTemplate->SetClassName(NanNew("Cache"));
  cacheConstructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(cacheConstructorTemplate, "close",
      NanNew<FunctionTemplate>(Cache::Close)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeFunction",
      NanNew<FunctionTemplate>(Cache::ExecuteFunction)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeQuery",
      NanNew<FunctionTemplate>(Cache::ExecuteQuery)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "createRegion",
      NanNew<FunctionTemplate>(Cache::CreateRegion)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "getRegion",
      NanNew<FunctionTemplate>(Cache::GetRegion)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "rootRegions",
      NanNew<FunctionTemplate>(Cache::RootRegions)->GetFunction());
  NanSetPrototypeTemplate(cacheConstructorTemplate, "inspect",
      NanNew<FunctionTemplate>(Cache::Inspect)->GetFunction());

  exports->Set(NanNew("Cache"), cacheConstructorTemplate->GetFunction());
}

NAN_METHOD(Cache::New) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError("Cache constructor requires a path to an XML configuration file as its first argument.");
    NanReturnUndefined();
  }

  CacheFactoryPtr cacheFactory(CacheFactory::createCacheFactory());
  cacheFactory->set("cache-xml-file", *NanAsciiString(args[0]));
  cacheFactory->setSubscriptionEnabled(true);

  CachePtr cachePtr;
  try {
    cachePtr = cacheFactory->create();
  } catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }

  if (!cachePtr->getPdxReadSerialized()) {
    cachePtr->close();
    NanThrowError("<pdx read-serialized='true' /> must be set in your cache xml");
    NanReturnUndefined();
  }

  Cache * cache = new Cache(cachePtr);
  cache->Wrap(args.This());

  Local<Object> process(NanNew(dependencies)->Get(NanNew("process"))->ToObject());
  static const int argc = 2;
  Handle<Value> argv[argc] = { NanNew("exit"), cache->exitCallback() };
  NanMakeCallback(process, "on", argc, argv);

  NanReturnValue(args.This());
}

Local<Function> Cache::exitCallback() {
  NanEscapableScope();

  Local<Function> unboundExitCallback(NanNew<FunctionTemplate>(Cache::Close)->GetFunction());

  static const int argc = 1;
  Handle<Value> argv[argc] = { NanObjectWrapHandle(this) };
  Local<Function> boundExitCallback(
      NanMakeCallback(unboundExitCallback, "bind", argc, argv).As<Function>());

  return NanEscapeScope(boundExitCallback);
}

NAN_METHOD(Cache::Close) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  cache->close();

  NanReturnUndefined();
}

void Cache::close() {
  if (!cachePtr->isClosed()) {
    cachePtr->close();
  }
}

class ExecuteQueryWorker : public GemfireWorker {
 public:
  ExecuteQueryWorker(QueryPtr queryPtr,
                     NanCallback * callback) :
      GemfireWorker(callback),
      queryPtr(queryPtr) {}

  void ExecuteGemfireWork() {
    selectResultsPtr = queryPtr->execute();
  }

  void HandleOKCallback() {
    NanScope();

    static const int argc = 2;
    Local<Value> argv[2] = { NanUndefined(), v8Value(selectResultsPtr) };
    callback->Call(argc, argv);
  }

  QueryPtr queryPtr;
  SelectResultsPtr selectResultsPtr;
};

QueryPtr Cache::newQuery(char * queryString) {
  return cachePtr->getQueryService()->newQuery(queryString);
}

NAN_METHOD(Cache::ExecuteQuery) {
  NanScope();

  if (args.Length() == 0 || !args[0]->IsString()) {
    NanThrowError("You must pass a query string and callback to executeQuery().");
    NanReturnUndefined();
  }

  if (args.Length() < 2) {
    NanThrowError("You must pass a callback to executeQuery().");
    NanReturnUndefined();
  }

  if (!args[1]->IsFunction()) {
    NanThrowError("You must pass a function as the callback to executeQuery().");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  if (cache->cachePtr->isClosed()) {
    NanThrowError("Cannot execute query; cache is closed.");
    NanReturnUndefined();
  }

  QueryPtr queryPtr(cache->newQuery(*NanUtf8String(args[0])));

  NanCallback * callback = new NanCallback(args[1].As<Function>());

  ExecuteQueryWorker * worker = new ExecuteQueryWorker(queryPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

NAN_METHOD(Cache::CreateRegion) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError(
        "createRegion: You must pass the name of a GemFire region to create "
        "and a region configuration object.");
    NanReturnUndefined();
  }

  if (!args[0]->IsString()) {
    NanThrowError("createRegion: You must pass a string as the name of a GemFire region.");
    NanReturnUndefined();
  }

  if (!args[1]->IsObject()) {
    NanThrowError("createRegion: You must pass a configuration object as the second argument.");
    NanReturnUndefined();
  }

  Local<Object> regionConfiguration(args[1]->ToObject());
  Local<Value> regionType(regionConfiguration->Get(NanNew("type")));

  if (regionType->IsUndefined()) {
    NanThrowError("createRegion: The region configuration object must have a type property.");
    NanReturnUndefined();
  }

  RegionShortcut regionShortcut(getRegionShortcut(*NanUtf8String(regionType)));
  if (regionShortcut == invalidRegionShortcut) {
    NanThrowError("createRegion: This type is not a valid GemFire client region type");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);

  RegionPtr regionPtr;
  try {
    RegionFactoryPtr regionFactoryPtr(cachePtr->createRegionFactory(regionShortcut));
    regionPtr = regionFactoryPtr->create(*NanUtf8String(args[0]));
  } catch (const gemfire::Exception & exception) {
    NanThrowError(gemfireExceptionMessage(exception).c_str());
    NanReturnUndefined();
  }

  NanReturnValue(Region::New(args.This(), regionPtr));
}

NAN_METHOD(Cache::GetRegion) {
  NanScope();

  if (args.Length() != 1) {
    NanThrowError("You must pass the name of a GemFire region to getRegion.");
    NanReturnUndefined();
  }

  if (!args[0]->IsString()) {
    NanThrowError("You must pass a string as the name of a GemFire region to getRegion.");
    NanReturnUndefined();
  }

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);
  RegionPtr regionPtr(cachePtr->getRegion(*NanAsciiString(args[0])));

  NanReturnValue(Region::New(args.This(), regionPtr));
}

NAN_METHOD(Cache::RootRegions) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());

  VectorOfRegion regions;
  cache->cachePtr->rootRegions(regions);

  unsigned int size = regions.size();
  Local<Array> rootRegions(NanNew<Array>(size));

  for (unsigned int i = 0; i < size; i++) {
    rootRegions->Set(i, Region::New(args.This(), regions[i]));
  }

  NanReturnValue(rootRegions);
}

NAN_METHOD(Cache::Inspect) {
  NanScope();
  NanReturnValue(NanNew("[Cache]"));
}

NAN_METHOD(Cache::ExecuteFunction) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr(cache->cachePtr);
  if (cachePtr->isClosed()) {
    NanThrowError("Cannot execute function; cache is closed.");
    NanReturnUndefined();
  }

  if (args[1]->IsObject() && !args[1]->IsArray()) {
    Local<Value> filter = args[1]->ToObject()->Get(NanNew("filter"));
    if (!filter->IsUndefined()) {
      NanThrowError("You cannot pass a filter to executeFunction for a Cache.");
      NanReturnUndefined();
    }
  }

  try {
    // FIXME: Workaround for the situation where there are no regions yet.
    //
    // As of GemFire Native Client 8.0.0.0, if no regions have ever been present, it's possible that
    // the cachePtr has no default pool set. Attempting to execute a function on this cachePtr will
    // throw a NullPointerException.
    //
    // To avoid this problem, we grab the first pool we can find and execute the function on that
    // pool's poolPtr instead of on the cachePtr. Note that this might not be the best choice of
    // poolPtr at the moment.
    //
    // See https://www.pivotaltracker.com/story/show/82079194 for the original bug.
    // See https://www.pivotaltracker.com/story/show/82125288 for a potential enhancement.
    HashMapOfPools hashMapOfPools(PoolManager::getAll());
    HashMapOfPools::Iterator iterator(hashMapOfPools.begin());
    PoolPtr poolPtr(iterator.second());

    ExecutionPtr executionPtr(FunctionService::onServer(poolPtr));
    NanReturnValue(executeFunction(args, cachePtr, executionPtr));
  } catch (const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
}

}  // namespace node_gemfire
