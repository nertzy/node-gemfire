#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;


gemfire::PdxInstancePtr pdxInstanceFromV8Object(const gemfire::CachePtr & cache,
                                                const Local<Object> & v8Object);
Handle<Value> v8ObjectFromPdxInstance(const gemfire::PdxInstancePtr & pdxInstance);

gemfire::CacheablePtr gemfireValueFromV8(const Handle<Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);

Handle<Value> v8ValueFromGemfire(const gemfire::CacheablePtr & valuePtr);
Handle<Array> v8ValueFromGemfire(const gemfire::CacheableVectorPtr & cacheableVectorPtr);
Handle<Object> v8ValueFromGemfire(const gemfire::StructPtr & structPtr);

Handle<Array> arrayFromSelectResults(const gemfire::SelectResultsPtr & selectResultsPtr);

#define __CONVERSIONS_HPP__
#endif
