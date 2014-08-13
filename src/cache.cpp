#include "cache.hpp"
#include <v8.h>
#include <nan.h>
#include <gfcpp/Cache.hpp>
#include <gfcpp/CacheFactory.hpp>
#include <gfcpp/Region.hpp>
#include "exceptions.hpp"
#include "conversions.hpp"
#include "region.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Cache::~Cache() {
  cachePtr->close();
}

void Cache::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> cacheConstructorTemplate =
    NanNew<FunctionTemplate>(Cache::New);

  cacheConstructorTemplate->SetClassName(NanNew("Cache"));
  cacheConstructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeQuery",
      NanNew<FunctionTemplate>(Cache::ExecuteQuery)->GetFunction());

  NanSetPrototypeTemplate(cacheConstructorTemplate, "getRegion",
      NanNew<FunctionTemplate>(Cache::GetRegion)->GetFunction());

  exports->Set(NanNew("Cache"), cacheConstructorTemplate->GetFunction());
}

NAN_METHOD(Cache::New) {
  NanScope();

  CacheFactoryPtr cacheFactory = CacheFactory::createCacheFactory();
  CachePtr cachePtr = cacheFactory
    ->set("log-level", "warning")
    ->set("cache-xml-file", "benchmark/xml/BenchmarkClient.xml")
    ->create();

  Cache * cache = new Cache(cachePtr);
  cache->Wrap(args.This());

  NanReturnValue(args.This());
}

NAN_METHOD(Cache::ExecuteQuery) {
  NanScope();

  Cache * cache = ObjectWrap::Unwrap<Cache>(args.This());
  CachePtr cachePtr = cache->cachePtr;

  QueryServicePtr queryServicePtr = cachePtr->getQueryService();
  String::Utf8Value queryString(args[0]);
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

NAN_METHOD(Cache::GetRegion) {
  NanScope();

  if (args.Length() != 1) {
    NanThrowError("getRegion expects one argument: the name of a Gemfire region");
    NanReturnUndefined();
  }

  Local<Function> regionGetRegionFunction = NanNew<FunctionTemplate>(Region::GetRegion)->GetFunction();

  const unsigned int argc = 2;
  Local<Value> argv[argc] = { args.This(), args[0] };
  Local<Value> regionHandle = NanMakeCallback(args.This(), regionGetRegionFunction, argc, argv);

  NanReturnValue(regionHandle);
}

}  // namespace node_gemfire
