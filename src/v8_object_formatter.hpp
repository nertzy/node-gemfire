#ifndef __V8_OBJECT_FORMATTER_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;

class V8ObjectFormatter {
  public:
    static gemfire::PdxInstancePtr toPdxInstance(gemfire::CachePtr cache, Local<Object> v8Object);
    static Local<Object> fromPdxInstance(gemfire::PdxInstancePtr pdxInstance);
};

#define __V8_OBJECT_FORMATTER_HPP__
#endif
