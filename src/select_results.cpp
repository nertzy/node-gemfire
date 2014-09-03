#include "conversions.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Persistent<FunctionTemplate> selectResultsConstructor;

void SelectResults::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>();

  constructor->SetClassName(NanNew("SelectResults"));
  constructor->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(constructor, "toArray",
      NanNew<FunctionTemplate>(SelectResults::ToArray)->GetFunction());

  NanAssignPersistent(selectResultsConstructor, constructor);
}

Handle<Object> SelectResults::NewInstance(const SelectResultsPtr & selectResultsPtr) {
  NanScope();

  const unsigned int argc = 0;
  Handle<Value> argv[argc] = {};
  Handle<Object> v8Object = selectResultsConstructor->GetFunction()->NewInstance(argc, argv);

  SelectResults * selectResults = new SelectResults(selectResultsPtr);
  selectResults->Wrap(v8Object);

  NanReturnValue(v8Object);
}

NAN_METHOD(SelectResults::ToArray) {
  NanScope();

  SelectResults * selectResults = ObjectWrap::Unwrap<SelectResults>(args.This());
  SelectResultsPtr selectResultsPtr = selectResults->selectResultsPtr;

  unsigned int length = selectResultsPtr->size();

  Local<Array> array = NanNew<Array>(length);
  for (unsigned int i = 0; i < length; i++) {
    array->Set(i, v8ValueFromGemfire((*selectResultsPtr)[i]));
  }

  NanReturnValue(array);
}

}  // namespace node_gemfire
