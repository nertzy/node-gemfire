#ifndef __REGION_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <gfcpp/Region.hpp>

using namespace v8;

namespace node_gemfire {

class Region : public node::ObjectWrap {
 public:
  Region(Local<Object> cacheHandle,
         gemfire::RegionPtr regionPtr) :
    regionPtr(regionPtr) {
      NanAssignPersistent(this->cacheHandle, cacheHandle);
    }

  ~Region() {
    NanDisposePersistent(cacheHandle);
  }

  static void Init(Local<Object> exports);
  static Local<Value> New(Local<Object> cacheObject, gemfire::RegionPtr regionPtr);
  static NAN_METHOD(Clear);
  static NAN_METHOD(Put);
  static NAN_METHOD(Get);
  static NAN_METHOD(GetAll);
  static NAN_METHOD(PutAll);
  static NAN_METHOD(Remove);
  static NAN_METHOD(Keys);
  static NAN_METHOD(ExecuteFunction);
  static NAN_METHOD(Inspect);
  static NAN_GETTER(Name);

  template<typename T>
  static NAN_METHOD(Query);

  gemfire::RegionPtr regionPtr;

 private:
  Persistent<Object> cacheHandle;
  static Persistent<Function> constructor;
};

}  // namespace node_gemfire

#define __REGION_HPP__
#endif
