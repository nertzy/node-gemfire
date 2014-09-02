#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;

gemfire::CacheablePtr gemfireValueFromV8(const Handle<Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::PdxInstancePtr gemfireValueFromV8(const Handle<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);

Handle<Value> v8ValueFromGemfire(const gemfire::CacheablePtr & valuePtr);
Handle<Object> v8ValueFromGemfire(const gemfire::StructPtr & structPtr);
Handle<Value> v8ValueFromGemfire(const gemfire::PdxInstancePtr & pdxInstance);

template<typename T> Handle<Array> v8ValueFromGemfire(const T & vectorPtr) {
  NanScope();

  unsigned int length = vectorPtr->size();

  Local<Array> array = NanNew<Array>(length);
  for (unsigned int i = 0; i < length; i++) {
    array->Set(i, v8ValueFromGemfire((*vectorPtr)[i]));
  }

  NanReturnValue(array);
}

#define __CONVERSIONS_HPP__
#endif
