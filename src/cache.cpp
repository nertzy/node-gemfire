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
  CachePtr cachePtr(cache->cachePtr);

  QueryServicePtr queryServicePtr(cachePtr->getQueryService());
  String::Utf8Value queryString(args[0]);
  QueryPtr queryPtr(queryServicePtr->newQuery(*queryString));

  Local<Function> callback(Local<Function>::Cast(args[1]));

  ExecuteQueryBaton * baton = new ExecuteQueryBaton(callback, queryPtr);

  uv_work_t * request = new uv_work_t();
  request->data = reinterpret_cast<void *>(baton);

  uv_queue_work(uv_default_loop(), request, cache->AsyncExecuteQuery, cache->AfterAsyncExecuteQuery);

  NanReturnValue(args.This());
}

void Cache::AsyncExecuteQuery(uv_work_t * request) {
  ExecuteQueryBaton * baton = reinterpret_cast<ExecuteQueryBaton *>(request->data);

  try {
    baton->selectResultsPtr = baton->queryPtr->execute();
  }
  catch(const gemfire::Exception & exception) {
    baton->errorMessage = gemfireExceptionMessage(exception);
  }
}

void Cache::AfterAsyncExecuteQuery(uv_work_t * request, int status) {
  NanScope();

  ExecuteQueryBaton * baton = reinterpret_cast<ExecuteQueryBaton *>(request->data);

  Local<Value> error;
  Local<Value> returnValue;

  if (baton->errorMessage.empty()) {
    error = NanNull();
    returnValue = NanNew(v8ValueFromGemfire(baton->selectResultsPtr));
  } else {
    error = NanError(baton->errorMessage.c_str());
    returnValue = NanUndefined();
  }

  static const int argc = 2;
  Local<Value> argv[2] = { error, returnValue };
  NanMakeCallback(NanGetCurrentContext()->Global(), baton->callback, argc, argv);

  delete request;
  delete baton;
}

NAN_METHOD(Cache::GetRegion) {
  NanScope();

  if (args.Length() != 1) {
    NanThrowError("getRegion expects one argument: the name of a Gemfire region");
    NanReturnUndefined();
  }

  Local<Function> regionGetRegionFunction =
    NanNew<FunctionTemplate>(Region::GetRegion)->GetFunction();

  const unsigned int argc = 2;
  Local<Value> argv[argc] = { args.This(), args[0] };
  Local<Value> regionHandle(NanMakeCallback(args.This(), regionGetRegionFunction, argc, argv));

  NanReturnValue(regionHandle);
}

NAN_METHOD(Cache::Inspect) {
  NanScope();
  NanReturnValue(NanNew("[Cache]"));
}

}  // namespace node_gemfire
