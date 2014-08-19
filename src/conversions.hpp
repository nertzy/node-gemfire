#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;

class V8ObjectFormatter {
 public:
    static gemfire::PdxInstancePtr toPdxInstance(gemfire::CachePtr cache, const Local<Object> & v8Object);
    static Handle<Value> fromPdxInstance(const gemfire::PdxInstancePtr & pdxInstance);
};

gemfire::CacheablePtr gemfireValueFromV8(const Handle<Value> & v8Value, gemfire::CachePtr cachePtr);
Handle<Value> v8ValueFromGemfire(const gemfire::CacheablePtr & valuePtr);

Handle<Array> arrayFromSelectResults(gemfire::SelectResultsPtr selectResultsPtr);

#define __CONVERSIONS_HPP__
#endif
