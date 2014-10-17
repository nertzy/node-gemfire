#include "functions.hpp"
#include <gfcpp/FunctionService.hpp>
#include <nan.h>
#include <v8.h>
#include <string>
#include "conversions.hpp"
#include "dependencies.hpp"
#include "exceptions.hpp"
#include "events.hpp"
#include "streaming_result_collector.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

class ExecuteFunctionWorker {
 public:
  ExecuteFunctionWorker(
      const ExecutionPtr & executionPtr,
      const std::string & functionName,
      const CacheablePtr & functionArguments,
      const CacheableVectorPtr & functionFilter,
      const Local<Object> & emitterHandle) :
    resultStream(
        new ResultStream(this,
                        (uv_async_cb) DataAsyncCallback,
                        (uv_async_cb) EndAsyncCallback)),
    executionPtr(executionPtr),
    functionName(functionName),
    functionArguments(functionArguments),
    functionFilter(functionFilter),
    ended(false),
    executeCompleted(false) {
      NanAssignPersistent(emitter, emitterHandle);
      request.data = reinterpret_cast<void *>(this);
    }

  ~ExecuteFunctionWorker() {
    NanDisposePersistent(emitter);
    delete resultStream;
  }

  static void Execute(uv_work_t * request) {
    ExecuteFunctionWorker * worker = static_cast<ExecuteFunctionWorker *>(request->data);
    worker->Execute();
  }

  static void ExecuteComplete(uv_work_t * request, int status) {
    ExecuteFunctionWorker * worker = static_cast<ExecuteFunctionWorker *>(request->data);
    worker->ExecuteComplete();
  }

  static void DataAsyncCallback(uv_async_t * async, int status) {
    ExecuteFunctionWorker * worker = reinterpret_cast<ExecuteFunctionWorker *>(async->data);
    worker->Data();
  }

  static void EndAsyncCallback(uv_async_t * async, int status) {
    ExecuteFunctionWorker * worker = reinterpret_cast<ExecuteFunctionWorker *>(async->data);
    worker->End();
  }

  void Execute() {
    try {
      if (functionArguments != NULLPTR) {
        executionPtr = executionPtr->withArgs(functionArguments);
      }

      if (functionFilter != NULLPTR) {
        executionPtr = executionPtr->withFilter(functionFilter);
      }

      ResultCollectorPtr resultCollectorPtr
        (new StreamingResultCollector(resultStream));
      executionPtr = executionPtr->withCollector(resultCollectorPtr);

      executionPtr->execute(functionName.c_str());
    } catch (const gemfire::Exception & exception) {
      errorMessage = gemfireExceptionMessage(exception);
    }
  }

  void ExecuteComplete() {
    if (!errorMessage.empty()) {
      NanScope();
      emitError(NanNew(emitter), NanError(errorMessage.c_str()));
      ended = true;
    }

    executeCompleted = true;
    teardownIfReady();
  }

  void Data() {
    NanScope();

    Local<Object> eventEmitter(NanNew(emitter));

    CacheableVectorPtr resultsPtr(resultStream->nextResults());
    for (CacheableVector::Iterator iterator(resultsPtr->begin());
         iterator != resultsPtr->end();
         ++iterator) {
      Local<Value> result(v8Value(*iterator));

      if (result->IsNativeError()) {
        emitError(eventEmitter, result);
      } else {
        emitEvent(eventEmitter, "data", result);
      }
    }

    resultStream->resultsProcessed();
  }

  void End() {
    NanScope();

    emitEvent(NanNew(emitter), "end");

    ended = true;
    teardownIfReady();
  }

  void teardownIfReady() {
    if (ended && executeCompleted) {
      delete this;
    }
  }

  uv_work_t request;

 private:
  ResultStream * resultStream;

  ExecutionPtr executionPtr;
  std::string functionName;
  CacheablePtr functionArguments;
  CacheableVectorPtr functionFilter;
  Persistent<Object> emitter;
  std::string errorMessage;

  bool ended;
  bool executeCompleted;
};

Local<Value> executeFunction(_NAN_METHOD_ARGS,
                             const CachePtr & cachePtr,
                             const ExecutionPtr & executionPtr) {
  NanEscapableScope();

  if (args.Length() == 0 || !args[0]->IsString()) {
    NanThrowError("You must provide the name of a function to execute.");
    return NanEscapeScope(NanUndefined());
  }

  Local<Value> v8FunctionArguments;
  Local<Value> v8FunctionFilter;

  if (args[1]->IsArray()) {
    v8FunctionArguments = args[1];
  } else if (args[1]->IsObject()) {
    Local<Object> optionsObject(args[1]->ToObject());
    v8FunctionArguments = optionsObject->Get(NanNew("arguments"));
    v8FunctionFilter = optionsObject->Get(NanNew("filter"));

    if (!v8FunctionFilter->IsArray() && !v8FunctionFilter->IsUndefined()) {
      NanThrowError("You must pass an Array of keys as the filter for executeFunction().");
      return NanEscapeScope(NanUndefined());
    }
  } else if (!args[1]->IsUndefined()) {
    NanThrowError("You must pass either an Array of arguments or an options Object to executeFunction().");
    return NanEscapeScope(NanUndefined());
  }

  std::string functionName(*NanUtf8String(args[0]));

  CacheablePtr functionArguments;
  if (v8FunctionArguments.IsEmpty() || v8FunctionArguments->IsUndefined()) {
    functionArguments = NULLPTR;
  } else {
    functionArguments = gemfireValue(v8FunctionArguments, cachePtr);
  }

  CacheableVectorPtr functionFilter;
  if (v8FunctionFilter.IsEmpty() || v8FunctionFilter->IsUndefined()) {
    functionFilter = NULLPTR;
  } else {
    functionFilter = gemfireVector(v8FunctionFilter.As<Array>(), cachePtr);
  }

  Local<Function> eventEmitterConstructor(NanNew(dependencies)->Get(NanNew("EventEmitter")).As<Function>());
  Local<Object> eventEmitter(eventEmitterConstructor->NewInstance());

  ExecuteFunctionWorker * worker =
    new ExecuteFunctionWorker(executionPtr, functionName, functionArguments, functionFilter, eventEmitter);

  uv_queue_work(
      uv_default_loop(),
      &worker->request,
      ExecuteFunctionWorker::Execute,
      ExecuteFunctionWorker::ExecuteComplete);

  return NanEscapeScope(eventEmitter);
}

}  // namespace node_gemfire
