#ifndef __CACHE_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <gfcpp/Cache.hpp>

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

class Cache : public node::ObjectWrap {
 public:
    static void Init(Handle<Object> exports);
 protected:
    static NAN_METHOD(New);
    static NAN_METHOD(ExecuteQuery);

    explicit Cache(CachePtr cachePtr) : cachePtr(cachePtr) {}
 private:
    CachePtr cachePtr;
};

}  // namespace node_gemfire

#define __CACHE_HPP__
#endif
