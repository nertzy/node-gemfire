#include <v8.h>
#include <nan.h>
#include <string>
#include "../../src/conversions.hpp"
#include "gtest/gtest.h"

using namespace v8;
using namespace node_gemfire;

TEST(getClassName, emptyObject) {
  NanScope();

  EXPECT_STREQ(getClassName(NanNew<Object>()).c_str(),
               getClassName(NanNew<Object>()).c_str());
}

TEST(getClassName, tinyObject) {
  NanScope();

  Local<Object> tinyObject = NanNew<Object>();
  tinyObject->Set(NanNew("foo"), NanNew("bar"));

  EXPECT_STREQ(getClassName(tinyObject).c_str(),
               getClassName(tinyObject).c_str());

  EXPECT_STRNE(getClassName(tinyObject).c_str(),
               getClassName(NanNew<Object>()).c_str());
}

TEST(getClassName, valueTypeMatters) {
  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("foo"), NanNew("bar"));

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("foo"), NanNew<Array>());

  EXPECT_STRNE(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}

TEST(getClassName, indifferentToOrder) {
  NanScope();

  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("foo"), NanNew("bar"));
  firstObject->Set(NanNew("baz"), NanNew("qux"));

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("baz"), NanNew("qux"));
  secondObject->Set(NanNew("foo"), NanNew("bar"));

  EXPECT_STREQ(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}

TEST(getClassName, fieldNamesAreDelimited) {
  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("ab"), NanNull());
  firstObject->Set(NanNew("c"), NanNull());

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("a"), NanNull());
  secondObject->Set(NanNew("bc"), NanNull());

  EXPECT_STRNE(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}

TEST(getClassName, fieldNamesCanContainCommas) {
  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("a,b"), NanNull());

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("a"), NanNull());
  secondObject->Set(NanNew("b"), NanNull());

  EXPECT_STRNE(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}

TEST(getClassName, fieldNamesCanContainBrackets) {
  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("a"), NanNew<Array>());

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("a[]"), NanNull());

  EXPECT_STRNE(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}

TEST(getClassName, fieldNamesCanContainBackslash) {
  Local<Object> firstObject = NanNew<Object>();
  firstObject->Set(NanNew("a,b"), NanNull());

  Local<Object> secondObject = NanNew<Object>();
  secondObject->Set(NanNew("a\\"), NanNull());
  secondObject->Set(NanNew("b"), NanNull());

  EXPECT_STRNE(getClassName(firstObject).c_str(),
               getClassName(secondObject).c_str());
}



NAN_METHOD(run) {
  NanScope();

  int argc = 0;
  char ** argv = {};
  ::testing::InitGoogleTest(&argc, argv);

  int testReturnCode = RUN_ALL_TESTS();

  NanReturnValue(NanNew(testReturnCode));
}

static void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "run", run);
}

NODE_MODULE(test, Initialize)
