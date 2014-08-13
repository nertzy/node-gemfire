#include "cache.hpp"
#include <v8.h>
#include <nan.h>
#include <gfcpp/Cache.hpp>
#include "exceptions.hpp"
#include "conversions.hpp"

using namespace v8;
using namespace gemfire;

void node_gemfire::Cache::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> cacheConstructorTemplate =
    NanNew<FunctionTemplate>(node_gemfire::Cache::New);
  cacheConstructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);

  Local<FunctionTemplate> executeQueryTemplate =
    NanNew<FunctionTemplate>(node_gemfire::Cache::ExecuteQuery);

  NanSetPrototypeTemplate(cacheConstructorTemplate, "executeQuery",
      executeQueryTemplate->GetFunction());

  exports->Set(NanNew("Cache"), cacheConstructorTemplate->GetFunction());
}

NAN_METHOD(node_gemfire::Cache::New) {
  NanScope();

  CacheFactoryPtr cacheFactory = CacheFactory::createCacheFactory();
  CachePtr cachePtr = cacheFactory
    ->set("log-level", "warning")
    ->set("cache-xml-file", "benchmark/xml/BenchmarkClient.xml")
    ->create();

  node_gemfire::Cache * cache = new node_gemfire::Cache(cachePtr);
  cache->Wrap(args.This());

  NanReturnValue(args.This());
}

NAN_METHOD(node_gemfire::Cache::ExecuteQuery) {
  NanScope();

  node_gemfire::Cache * cache = ObjectWrap::Unwrap<node_gemfire::Cache>(args.This());
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
