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

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

void Cache::Init(Local<Object> exports) {
  NanScope();

  Local<FunctionTemplate> cacheConstructorTemplate =
    NanNew<FunctionTemplate>(Cache::New);

  cacheConstructorTemplate->SetClassName(NanNew("Cache"));
  cacheConstructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeQuery",
      NanNew<FunctionTemplate>(Cache::ExecuteQuery)->GetFunction());
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

  CacheFactoryPtr cacheFactory(
      CacheFactory::createCacheFactory()
          ->set("cache-xml-file", *NanAsciiString(args[0])));

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

  NanReturnValue(args.This());
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
    Local<Value> argv[2] = { NanUndefined(), v8ValueFromGemfire(selectResultsPtr) };
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
  QueryPtr queryPtr(cache->newQuery(*(NanUtf8String(args[0]))));

  NanCallback * callback = new NanCallback(args[1].As<Function>());

  ExecuteQueryWorker * worker = new ExecuteQueryWorker(queryPtr, callback);
  NanAsyncQueueWorker(worker);

  NanReturnValue(args.This());
}

NAN_METHOD(Cache::GetRegion) {
  NanScope();

  if (args.Length() != 1) {
    NanThrowError("getRegion expects one argument: the name of a Gemfire region");
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

}  // namespace node_gemfire
