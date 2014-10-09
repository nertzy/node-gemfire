#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <gfcpp/Cache.hpp>

using namespace v8;

namespace node_gemfire {

class Cache : public node::ObjectWrap {
 public:
  static void Init(Local<Object> exports);

  gemfire::CachePtr cachePtr;

 protected:
  explicit Cache(
      gemfire::CachePtr cachePtr) :
    cachePtr(cachePtr) {}

  ~Cache() {
    close();
  }

  void close();

  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(ExecuteFunction);
  static NAN_METHOD(ExecuteQuery);
  static NAN_METHOD(CreateRegion);
  static NAN_METHOD(GetRegion);
  static NAN_METHOD(RootRegions);
  static NAN_METHOD(Inspect);

  gemfire::QueryPtr newQuery(char * queryString);

 private:
  Local<Function> exitCallback();
};

}  // namespace node_gemfire

#endif
