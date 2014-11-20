#include <v8.h>
#include <nan.h>
#include <string>
#include "../../src/conversions.hpp"
#include "../../src/region_shortcuts.hpp"
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

TEST(getRegionShortcut, proxy) {
  EXPECT_EQ(gemfire::PROXY, getRegionShortcut("PROXY"));
}

TEST(getRegionShortcut, cachingProxy) {
  EXPECT_EQ(gemfire::CACHING_PROXY, getRegionShortcut("CACHING_PROXY"));
}

TEST(getRegionShortcut, local) {
  EXPECT_EQ(gemfire::LOCAL, getRegionShortcut("LOCAL"));
}

TEST(getRegionShortcut, cachingProxyEntryLru) {
  EXPECT_EQ(gemfire::CACHING_PROXY_ENTRY_LRU, getRegionShortcut("CACHING_PROXY_ENTRY_LRU"));
}

TEST(getRegionShortcut, localEntryLru) {
  EXPECT_EQ(gemfire::LOCAL_ENTRY_LRU, getRegionShortcut("LOCAL_ENTRY_LRU"));
}

TEST(getRegionShortcut, incorrectShortcut) {
  EXPECT_NE(gemfire::PROXY, getRegionShortcut("NULL"));
  EXPECT_NE(gemfire::CACHING_PROXY, getRegionShortcut("NULL"));
  EXPECT_NE(gemfire::CACHING_PROXY_ENTRY_LRU, getRegionShortcut("NULL"));
  EXPECT_NE(gemfire::LOCAL, getRegionShortcut("NULL"));
  EXPECT_NE(gemfire::LOCAL_ENTRY_LRU, getRegionShortcut("NULL"));
}

NAN_METHOD(run) {
  NanScope();

  int argc = 0;
  char * argv[0] = {};
  ::testing::InitGoogleTest(&argc, argv);

  int testReturnCode = RUN_ALL_TESTS();

  NanReturnValue(NanNew(testReturnCode));
}

static void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "run", run);
}

NODE_MODULE(test, Initialize)
