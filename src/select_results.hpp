#ifndef __SELECT_RESULTS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/SelectResults.hpp>
#include <node.h>

using namespace v8;

namespace node_gemfire {

class SelectResults : public node::ObjectWrap {
 public:
  explicit SelectResults(gemfire::SelectResultsPtr selectResultsPtr) :
    selectResultsPtr(selectResultsPtr) {}

  static void Init(Handle<Object> exports);
  static Handle<Object> NewInstance(const gemfire::SelectResultsPtr & selectResultsPtr);
  static NAN_METHOD(ToArray);
  static NAN_METHOD(Each);

 private:
  gemfire::SelectResultsPtr selectResultsPtr;
};


}  // namespace node_gemfire

#define __SELECT_RESULTS_HPP__
#endif
