#include <v8.h>
#include <nan.h>
#include "gtest/gtest.h"

using namespace v8;

TEST(FooTest, Bar) {
  ASSERT_EQ(true, true);
}

NAN_METHOD(run) {
  NanScope();
  int testReturnCode = RUN_ALL_TESTS();

  NanReturnValue(NanNew(testReturnCode));
}

static void Initialize(Handle<Object> exports) {
  int argc = 0;
  char ** argv = NULL;
  ::testing::InitGoogleTest(&argc, argv);
  NODE_SET_METHOD(exports, "run", run);
}

NODE_MODULE(test, Initialize)
