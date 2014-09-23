#include <gfcpp/SelectResultsIterator.hpp>
#include <sstream>
#include "conversions.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

Persistent<FunctionTemplate> selectResultsConstructor;

void SelectResults::Init(Handle<Object> exports) {
  NanScope();

  Local<FunctionTemplate> constructor(NanNew<FunctionTemplate>());

  constructor->SetClassName(NanNew("SelectResults"));
  constructor->InstanceTemplate()->SetInternalFieldCount(1);

  NanSetPrototypeTemplate(constructor, "toArray",
      NanNew<FunctionTemplate>(SelectResults::ToArray)->GetFunction());
  NanSetPrototypeTemplate(constructor, "each",
      NanNew<FunctionTemplate>(SelectResults::Each)->GetFunction());
  NanSetPrototypeTemplate(constructor, "inspect",
      NanNew<FunctionTemplate>(SelectResults::Inspect)->GetFunction());

  NanAssignPersistent(selectResultsConstructor, constructor);
}

Handle<Object> SelectResults::NewInstance(const SelectResultsPtr & selectResultsPtr) {
  NanEscapableScope();

  const unsigned int argc = 0;
  Handle<Value> argv[argc] = {};
  Handle<Object> v8Object(selectResultsConstructor->GetFunction()->NewInstance(argc, argv));

  SelectResults * selectResults = new SelectResults(selectResultsPtr);
  selectResults->Wrap(v8Object);

  return NanEscapeScope(v8Object);
}

NAN_METHOD(SelectResults::ToArray) {
  NanScope();

  SelectResults * selectResults = ObjectWrap::Unwrap<SelectResults>(args.This());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  unsigned int length = selectResultsPtr->size();

  Local<Array> array(NanNew<Array>(length));
  for (unsigned int i = 0; i < length; i++) {
    array->Set(i, v8ValueFromGemfire((*selectResultsPtr)[i]));
  }

  NanReturnValue(array);
}

NAN_METHOD(SelectResults::Each) {
  NanScope();

  if (args.Length() == 0 || !args[0]->IsFunction()) {
    NanThrowError("You must pass a callback to each()");
    NanReturnUndefined();
  }

  SelectResults * selectResults = ObjectWrap::Unwrap<SelectResults>(args.This());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  SelectResultsIterator iterator(selectResultsPtr->getIterator());
  Local<Function> callback(Local<Function>::Cast(args[0]));

  while (iterator.hasNext()) {
    const unsigned int argc = 1;
    Handle<Value> argv[argc] = { v8ValueFromGemfire(iterator.next()) };
    Local<Value> regionHandle(NanMakeCallback(args.This(), callback, argc, argv));
  }

  NanReturnValue(args.This());
}

NAN_METHOD(SelectResults::Inspect) {
  NanScope();

  SelectResults * selectResults = ObjectWrap::Unwrap<SelectResults>(args.This());
  SelectResultsPtr selectResultsPtr(selectResults->selectResultsPtr);

  std::stringstream inspectStream;
  inspectStream << "[SelectResults size=" << selectResultsPtr->size() << "]";

  NanReturnValue(NanNew(inspectStream.str().c_str()));
}

}  // namespace node_gemfire
