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
  NanSetPrototypeTemplate(constructor, "remove",
      NanNew<FunctionTemplate>(Region::Remove)->GetFunction());
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

std::string unableToPutValueError(Handle<Value> v8Value) {
  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to put value " << *String::Utf8Value(v8Value->ToDetailString());
  return errorMessageStream.str();
}

class PutWorker : public NanAsyncWorker {
 public:
  PutWorker(
    RegionPtr regionPtr,
    CacheableKeyPtr keyPtr,
    CacheablePtr valuePtr,
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
    Local<Value> argv[2] = { NanNull(), NanNew(v8ValueFromGemfire(valuePtr)) };
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
           RegionPtr regionPtr,
           CacheableKeyPtr keyPtr) :
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
    Local<Value> argv[argc] = { NanNull(), NanNew(v8ValueFromGemfire(valuePtr)) };
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

  Local<Function> callback(Local<Function>::Cast(args[1]));

  if (keyPtr == NULLPTR) {
    Local<Value> error(NanError("Invalid GemFire key."));

    static const int argc = 2;
    Local<Value> argv[2] = { error, NanUndefined() };
    NanMakeCallback(NanGetCurrentContext()->Global(), callback, argc, argv);
  } else {
    RemoveBaton * baton = new RemoveBaton(callback, regionPtr, keyPtr);

    uv_work_t * request = new uv_work_t();
    request->data = reinterpret_cast<void *>(baton);

    uv_queue_work(uv_default_loop(), request, region->AsyncRemove, region->AfterAsyncRemove);
  }

  NanReturnValue(args.This());
}

void Region::AsyncRemove(uv_work_t * request) {
  RemoveBaton * baton = reinterpret_cast<RemoveBaton *>(request->data);

  try {
    baton->regionPtr->destroy(baton->keyPtr);
  } catch (const gemfire::EntryNotFoundException & exception) {
    baton->errorMessage = "Key not found in region.";
  } catch (const gemfire::Exception & exception) {
    baton->errorMessage = gemfireExceptionMessage(exception);
  }
}

void Region::AfterAsyncRemove(uv_work_t * request, int status) {
  NanScope();

  RemoveBaton * baton = reinterpret_cast<RemoveBaton *>(request->data);

  Local<Value> returnValue;
  Local<Value> error;

  if (baton->errorMessage.empty()) {
    returnValue = NanTrue();
    error = NanNull();
  } else {
    returnValue = NanUndefined();
    error = NanError(baton->errorMessage.c_str());
  }

  static const unsigned int argc = 2;
  Handle<Value> argv[argc] = { error, returnValue };
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

  if (v8ArgsLength == 1) {
    NanThrowError("You must pass a callback to executeFunction().");
    NanReturnUndefined();
  }

  Region * region = ObjectWrap::Unwrap<Region>(args.This());
  RegionPtr regionPtr(region->regionPtr);

  CacheablePtr functionArguments;
  Local<Function> callback;

  if (args[1]->IsFunction()) {
    functionArguments = NULLPTR;
    callback = Local<Function>::Cast(args[1]);
  } else {
    if (v8ArgsLength == 2) {
      NanThrowError("You must pass a callback to executeFunction().");
      NanReturnUndefined();
    } else if (!args[2]->IsFunction()) {
      NanThrowError("You must pass a function as the callback to executeFunction().");
      NanReturnUndefined();
    }
    functionArguments = gemfireValueFromV8(args[1], regionPtr->getCache());
    callback = Local<Function>::Cast(args[2]);
  }

  std::string functionName(*NanUtf8String(args[0]));

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
