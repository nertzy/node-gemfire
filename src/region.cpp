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
  NanSetPrototypeTemplate(constructor, "executeFunction",
      NanNew<FunctionTemplate>(Region::ExecuteFunction)->GetFunction());
  NanSetPrototypeTemplate(constructor, "inspect",
      NanNew<FunctionTemplate>(Region::Inspect)->GetFunction());

  constructor->PrototypeTemplate()->SetAccessor(NanNew("name"), Region::Name);

  NanAssignPersistent(regionConstructor, constructor);
}

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

Handle<Value> unableToPutValueError(Handle<Value> v8Value) {
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
  RegionPtr regionPtr(region->regionPtr);
  CachePtr cachePtr(regionPtr->getCache());

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], cachePtr));
  if (keyPtr == NULLPTR) {
    NanReturnUndefined();
  }

  CacheablePtr valuePtr(gemfireValueFromV8(args[1], cachePtr));

  if (args.Length() > 2 && args[2]->IsFunction()) {
    Local<Function> callback(Local<Function>::Cast(args[2]));

    if (valuePtr == NULLPTR) {
      Local<Value> error(NanNew(unableToPutValueError(args[1])));

      static const int argc = 2;
      Local<Value> argv[2] = { error, NanUndefined() };
      NanMakeCallback(NanGetCurrentContext()->Global(), callback, argc, argv);
    } else {
      PutBaton * baton = new PutBaton(callback, regionPtr, keyPtr, valuePtr);

      uv_work_t * request = new uv_work_t();
      request->data = reinterpret_cast<void *>(baton);

      uv_queue_work(uv_default_loop(), request, region->AsyncPut, region->AfterAsyncPut);
    }

    NanReturnValue(args.This());
  } else {
    if (valuePtr == NULLPTR) {
      NanThrowError(unableToPutValueError(args[1]));
      NanReturnUndefined();
    }

    try {
      regionPtr->put(keyPtr, valuePtr);
    }
    catch (gemfire::Exception & exception) {
      ThrowGemfireException(exception);
      NanReturnUndefined();
    }
    NanReturnValue(args[1]);
  }
}

void Region::AsyncPut(uv_work_t * request) {
  PutBaton * baton = reinterpret_cast<PutBaton *>(request->data);
  try {
    baton->regionPtr->put(baton->keyPtr, baton->valuePtr);
  }
  catch (gemfire::Exception & exception) {
    baton->errorMessage = gemfireExceptionMessage(exception);
  }
}

void Region::AfterAsyncPut(uv_work_t * request, int status) {
  NanScope();

  PutBaton * baton = reinterpret_cast<PutBaton *>(request->data);

  Local<Value> error;
  Local<Value> returnValue;

  if (baton->errorMessage.empty()) {
    error = NanNull();
    returnValue = NanNew(v8ValueFromGemfire(baton->valuePtr));
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

NAN_METHOD(Region::Get) {
  NanScope();

  if (args.Length() < 1) {
    NanThrowError("You must pass a key to get()");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CacheableKeyPtr keyPtr(gemfireKeyFromV8(args[0], regionPtr->getCache()));
  if (keyPtr == NULLPTR) {
    NanReturnUndefined();
  }

  if (args.Length() > 1 && args[1]->IsFunction()) {
    Local<Function> callback(Local<Function>::Cast(args[1]));
    GetBaton * baton = new GetBaton(callback, regionPtr, keyPtr);

    uv_work_t * request = new uv_work_t();
    request->data = reinterpret_cast<void *>(baton);

    uv_queue_work(uv_default_loop(), request, region->AsyncGet, region->AfterAsyncGet);

    NanReturnValue(args.This());
  } else {
    CacheablePtr valuePtr;
    try {
      valuePtr = regionPtr->get(keyPtr);
    }
    catch (gemfire::Exception & exception) {
      ThrowGemfireException(exception);
      NanReturnUndefined();
    }

    NanReturnValue(v8ValueFromGemfire(valuePtr));
  }
}

void Region::AsyncGet(uv_work_t * request) {
  GetBaton * baton = reinterpret_cast<GetBaton *>(request->data);
  try {
    baton->valuePtr = baton->regionPtr->get(baton->keyPtr);

    if (baton->valuePtr == NULLPTR) {
      baton->errorMessage = "Key not found in region.";
    }
  }
  catch (gemfire::Exception & exception) {
    baton->errorMessage = gemfireExceptionMessage(exception);
  }
}

void Region::AfterAsyncGet(uv_work_t * request, int status) {
  NanScope();

  GetBaton * baton = reinterpret_cast<GetBaton *>(request->data);

  Local<Value> returnValue;
  Local<Value> error;

  if (baton->errorMessage.empty())  {
    returnValue = NanNew(v8ValueFromGemfire(baton->valuePtr));
    error = NanNull();
  } else {
    returnValue = NanUndefined();
    error = NanError(baton->errorMessage.c_str());
  }

  static const int argc = 2;
  Local<Value> argv[2] = { error, returnValue };
  NanMakeCallback(NanGetCurrentContext()->Global(), baton->callback, argc, argv);

  delete request;
  delete baton;
}

NAN_METHOD(Region::ExecuteFunction) {
  NanScope();

  const unsigned int v8ArgsLength = args.Length();

  if (v8ArgsLength == 0 || args[0]->IsFunction()) {
    NanThrowError("You must provide the name of a function to execute.");
    NanReturnUndefined();
  }

  Local<Value> lastArgument(args[v8ArgsLength - 1]);
  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);
  std::string functionName(*NanUtf8String(args[0]));
  CacheablePtr functionArguments(NULLPTR);

  if (v8ArgsLength > 1 && !args[1]->IsFunction()) {
    functionArguments = gemfireValueFromV8(args[1], regionPtr->getCache());
  }

  if (lastArgument->IsFunction()) {
    Local<Function> callback(Local<Function>::Cast(lastArgument));

    ExecuteFunctionBaton * baton = new ExecuteFunctionBaton(regionPtr,
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
    ExecutionPtr executionPtr(FunctionService::onRegion(regionPtr));
    if (functionArguments != NULLPTR) {
      executionPtr = executionPtr->withArgs(functionArguments);
    }

    try {
      ResultCollectorPtr resultCollectorPtr(executionPtr->execute(functionName.c_str()));
      CacheableVectorPtr resultsPtr(resultCollectorPtr->getResult());
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

  ExecutionPtr executionPtr(FunctionService::onRegion(baton->regionPtr));

  if (baton->functionArguments != NULLPTR) {
    executionPtr = executionPtr->withArgs(baton->functionArguments);
  }

  try {
    baton->resultsPtr = executionPtr->execute(baton->functionName.c_str())->getResult();
  }
  catch (gemfire::Exception &exception) {
    baton->errorMessage = gemfireExceptionMessage(exception);
  }
}

void Region::AfterAsyncExecuteFunction(uv_work_t * request, int status) {
  ExecuteFunctionBaton * baton = reinterpret_cast<ExecuteFunctionBaton *>(request->data);

  Local<Value> error;
  Local<Value> returnValue;

  if (baton->errorMessage.empty()) {
    Handle<Array> resultsArray(v8ValueFromGemfire(baton->resultsPtr));
    error = NanNull();

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

    returnValue = NanNew(resultsArray);
  } else {
    error = NanError(baton->errorMessage.c_str());
    returnValue = NanUndefined();
  }

  const unsigned int argc = 2;
  Handle<Value> argv[argc] = { error, returnValue };
  NanMakeCallback(NanGetCurrentContext()->Global(), baton->callback, argc, argv);

  delete request;
  delete baton;
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

}  // namespace node_gemfire
