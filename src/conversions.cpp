#include <node.h>
#include <nan.h>
#include <v8.h>
#include <math.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>
#include <set>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

std::string getClassName(const Local<Object> & v8Object) {
  NanScope();

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());

  std::set<std::string> fieldNames;
  unsigned int length = v8Keys->Length();
  for (unsigned int i = 0; i < length; i++) {
    Local<Value> v8Key(v8Keys->Get(i));
    NanUtf8String utf8FieldName(v8Key);
    char * fieldName = *utf8FieldName;
    unsigned int size = utf8FieldName.Size();

    std::stringstream fullFieldName;

    for (unsigned int i = 0; i < size - 1; i++) {
      char fieldNameChar = fieldName[i];
      switch (fieldNameChar) {
        case ',':
        case '[':
        case ']':
        case '\\':
          fullFieldName << '\\';
      }
      fullFieldName << fieldNameChar;
    }

    Local<Value> v8Value(v8Object->Get(v8Key));
    if (v8Value->IsArray() && !v8Value->IsString()) {
      fullFieldName << "[]";
    }
    fullFieldName << ',';

    fieldNames.insert(fullFieldName.str());
  }

  std::stringstream className;
  className << "JSON: ";

  for (std::set<std::string>::iterator i(fieldNames.begin()); i != fieldNames.end(); ++i) {
    className << *i;
  }

  return className.str();
}

std::wstring wstringFromV8String(const Local<String> & v8String) {
  NanScope();

  unsigned int length = v8String->Length();
  wchar_t * buffer = new wchar_t[length + 1];
  NanUcs2String v8Data(v8String);
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = (*v8Data)[i];
  }
  buffer[length] = 0;

  std::wstring wstring(buffer);
  delete [] buffer;

  return wstring;
}

Local<String> v8StringFromWstring(const std::wstring & wideString) {
  NanEscapableScope();

  unsigned int length = wideString.length();
  uint16_t * buffer = new uint16_t[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = wideString[i];
  }
  buffer[length] = 0;

  Local<String> v8String(NanNew(buffer));
  delete[] buffer;

  return NanEscapeScope(v8String);
}

void ConsoleWarn(const char * message) {
  NanScope();

  Local<Object> global(NanGetCurrentContext()->Global());
  Local<Function> warn(global->Get(NanNew("console"))->ToObject()->Get(NanNew("warn")).As<Function>());
  NanCallback callback(warn);

  static const int argc = 1;
  Local<Value> argv[argc] = { NanNew(message) };

  callback.Call(argc, argv);
}

CacheablePtr gemfireValue(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  if (v8Value->IsString() || v8Value->IsStringObject()) {
    return CacheableString::create(wstringFromV8String(v8Value->ToString()).c_str());
  } else if (v8Value->IsBoolean()) {
    return CacheableBoolean::create(v8Value->ToBoolean()->Value());
  } else if (v8Value->IsNumber() || v8Value->IsNumberObject()) {
    return CacheableDouble::create(v8Value->ToNumber()->Value());
  } else if (v8Value->IsDate()) {
    return gemfireValue(Local<Date>::Cast(v8Value));
  } else if (v8Value->IsArray()) {
    return gemfireValue(Local<Array>::Cast(v8Value), cachePtr);
  } else if (v8Value->IsBooleanObject()) {
#if (NODE_MODULE_VERSION > 0x000B)
    return CacheableBoolean::create(BooleanObject::Cast(*v8Value)->ValueOf());
#else
    return CacheableBoolean::create(BooleanObject::Cast(*v8Value)->BooleanValue());
#endif
  } else if (v8Value->IsFunction()) {
    NanThrowError("Unable to serialize to GemFire; functions are not supported.");
    return NULLPTR;
  } else if (v8Value->IsObject()) {
    return gemfireValue(v8Value->ToObject(), cachePtr);
  } else if (v8Value->IsUndefined()) {
    return CacheableUndefined::create();
  } else if (v8Value->IsNull()) {
    return NULLPTR;
  } else {
    std::string errorMessage("Unable to serialize to GemFire; unknown JavaScript object: ");
    errorMessage.append(*NanUtf8String(v8Value->ToDetailString()));
    NanThrowError(errorMessage.c_str());
    return NULLPTR;
  }
}

PdxInstancePtr gemfireValue(const Local<Object> & v8Object, const CachePtr & cachePtr) {
  NanEscapableScope();

  try {
    std::string pdxClassName = getClassName(v8Object);
    PdxInstanceFactoryPtr pdxInstanceFactory = cachePtr->createPdxInstanceFactory(pdxClassName.c_str());

    Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
    unsigned int length = v8Keys->Length();

    for (unsigned int i = 0; i < length; i++) {
      Local<Value> v8Key(v8Keys->Get(i));
      Local<Value> v8Value(v8Object->Get(v8Key));

      NanUtf8String fieldName(v8Key);

      CacheablePtr cacheablePtr(gemfireValue(v8Value, cachePtr));
      if (v8Value->IsArray()) {
        pdxInstanceFactory->writeObjectArray(*fieldName, cacheablePtr);
      } else {
        pdxInstanceFactory->writeObject(*fieldName, cacheablePtr);
      }
    }

    return pdxInstanceFactory->create();
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

gemfire::CacheableObjectArrayPtr gemfireValue(const Local<Array> & v8Array,
                                         const gemfire::CachePtr & cachePtr) {
  CacheableObjectArrayPtr objectArrayPtr(CacheableObjectArray::create());

  unsigned int length = v8Array->Length();
  for (unsigned int i = 0; i < length; i++) {
    objectArrayPtr->push_back(gemfireValue(v8Array->Get(i), cachePtr));
  }

  return objectArrayPtr;
}

gemfire::CacheableDatePtr gemfireValue(const Local<Date> & v8Date) {
  uint64 millisecondsSinceEpoch = v8Date->NumberValue();

  timeval timeSinceEpoch;
  timeSinceEpoch.tv_sec = millisecondsSinceEpoch / 1000;
  timeSinceEpoch.tv_usec = (millisecondsSinceEpoch % 1000) * 1000;

  return CacheableDate::create(timeSinceEpoch);
}

CacheableKeyPtr gemfireKey(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  CacheableKeyPtr keyPtr;
  try {
    keyPtr = gemfireValue(v8Value, cachePtr);
  }
  catch(const ClassCastException & exception) {
    return NULLPTR;
  }

  return keyPtr;
}

VectorOfCacheableKeyPtr gemfireKeys(const Local<Array> & v8Value,
                                          const CachePtr & cachePtr) {
  VectorOfCacheableKeyPtr vectorPtr(new VectorOfCacheableKey());

  for (unsigned int i = 0; i < v8Value->Length(); i++) {
    CacheableKeyPtr keyPtr = gemfireKey(v8Value->Get(i), cachePtr);

    if (keyPtr == NULLPTR) {
      return NULLPTR;
    } else {
      vectorPtr->push_back(keyPtr);
    }
  }

  return vectorPtr;
}

HashMapOfCacheablePtr gemfireHashMap(const Local<Object> & v8Object,
                                           const CachePtr & cachePtr) {
  NanScope();

  HashMapOfCacheablePtr hashMapPtr(new HashMapOfCacheable());

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
  unsigned int length = v8Keys->Length();

  for (unsigned int i = 0; i < length; i++) {
    Local<String> v8Key(v8Keys->Get(i)->ToString());

    CacheableKeyPtr keyPtr(gemfireKey(v8Key, cachePtr));
    CacheablePtr valuePtr(gemfireValue(v8Object->Get(v8Key), cachePtr));

    if (valuePtr == NULLPTR) {
      return NULLPTR;
    }

    hashMapPtr->insert(keyPtr, valuePtr);
  }

  return hashMapPtr;
}

CacheableVectorPtr gemfireVector(const Local<Array> & v8Array, const CachePtr & cachePtr) {
  NanScope();

  unsigned int length = v8Array->Length();
  CacheableVectorPtr vectorPtr = CacheableVector::create();

  for (unsigned int i = 0; i < length; i++) {
    vectorPtr->push_back(gemfireValue(v8Array->Get(i), cachePtr));
  }

  return vectorPtr;
}

Local<Value> v8Value(const CacheablePtr & valuePtr) {
  NanEscapableScope();

  if (valuePtr == NULLPTR) {
    return NanEscapeScope(NanNull());
  }

  int typeId = valuePtr->typeId();
  switch (typeId) {
    case GemfireTypeIds::CacheableASCIIString:
    case GemfireTypeIds::CacheableASCIIStringHuge:
      return NanEscapeScope(NanNew((static_cast<CacheableStringPtr>(valuePtr))->asChar()));
    case GemfireTypeIds::CacheableString:
    case GemfireTypeIds::CacheableStringHuge:
      return NanEscapeScope(v8StringFromWstring((static_cast<CacheableStringPtr>(valuePtr))->asWChar()));
    case GemfireTypeIds::CacheableBoolean:
      return NanEscapeScope(NanNew((static_cast<CacheableBooleanPtr>(valuePtr))->value()));
    case GemfireTypeIds::CacheableDouble:
      return NanEscapeScope(NanNew((static_cast<CacheableDoublePtr>(valuePtr))->value()));
    case GemfireTypeIds::CacheableFloat:
      return NanEscapeScope(NanNew((static_cast<CacheableFloatPtr>(valuePtr))->value()));
    case GemfireTypeIds::CacheableInt16:
      return NanEscapeScope(NanNew((static_cast<CacheableInt16Ptr>(valuePtr))->value()));
    case GemfireTypeIds::CacheableInt32:
      return NanEscapeScope(NanNew((static_cast<CacheableInt32Ptr>(valuePtr))->value()));
    case GemfireTypeIds::CacheableInt64:
      return NanEscapeScope(v8Value(static_cast<CacheableInt64Ptr>(valuePtr)));
    case GemfireTypeIds::CacheableDate:
      return NanEscapeScope(v8Value(static_cast<CacheableDatePtr>(valuePtr)));
    case GemfireTypeIds::CacheableUndefined:
      return NanEscapeScope(NanUndefined());
    case GemfireTypeIds::Struct:
      return NanEscapeScope(v8Value(static_cast<StructPtr>(valuePtr)));
    case GemfireTypeIds::CacheableObjectArray:
      return NanEscapeScope(v8Array(static_cast<CacheableObjectArrayPtr>(valuePtr)));
    case GemfireTypeIds::CacheableVector:
      return NanEscapeScope(v8Array(static_cast<CacheableVectorPtr>(valuePtr)));
    case GemfireTypeIds::CacheableHashMap:
      return NanEscapeScope(v8Object(static_cast<CacheableHashMapPtr>(valuePtr)));
    case GemfireTypeIds::CacheableHashSet:
      return NanEscapeScope(v8Array(static_cast<CacheableHashSetPtr>(valuePtr)));
    case 0:
      try {
        UserFunctionExecutionExceptionPtr functionExceptionPtr =
          static_cast<UserFunctionExecutionExceptionPtr>(valuePtr);

        return NanEscapeScope(NanError(gemfireExceptionMessage(functionExceptionPtr).c_str()));
      } catch (ClassCastException & exception) {
        // fall through to default error case
      }
      break;
  }

  if (typeId > GemfireTypeIds::CacheableStringHuge) {
    // We are assuming these are Pdx
    return NanEscapeScope(v8Value(static_cast<PdxInstancePtr>(valuePtr)));
  }

  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to serialize value from GemFire; unknown typeId: " << typeId;
  NanThrowError(errorMessageStream.str().c_str());
  return NanEscapeScope(NanUndefined());
}

Local<Value> v8Value(const PdxInstancePtr & pdxInstance) {
  NanEscapableScope();

  try {
    CacheableStringArrayPtr gemfireKeys(pdxInstance->getFieldNames());

    if (gemfireKeys == NULLPTR) {
      return NanEscapeScope(NanNew<Object>());
    }

    Local<Object> v8Object(NanNew<Object>());
    int length = gemfireKeys->length();

    for (int i = 0; i < length; i++) {
      const char * key = gemfireKeys[i]->asChar();

      CacheablePtr value;

      // FIXME: This workaround is necessary for GemFire 8.0.0.
      //
      // There is no API in the GemFire Native Client for detecting whether or not a
      // PdxInstance field is an array. So we must guess whether to use a CacheablePtr or
      // a CacheableObjectArrayPtr.
      //
      // Unfortunately, getting an array field from Gemfire as a CacheablePtr either throws
      // an exception or returns a corrupted CacheablePtr. So we must try to read each field
      // as a CacheableObjectArrayPtr first, since that seems to reliably throw an exception
      // on failure.
      bool notArray = false;
      try {
        CacheableObjectArrayPtr valueArray;
        pdxInstance->getField(key, valueArray);
        value = valueArray;
      } catch(const OutOfRangeException & exception) {
        notArray = true;
      } catch(const IllegalStateException & exception) {
        notArray = true;
      } catch(const gemfire::Exception & exception) {
        std::stringstream errorMessageStream;
        errorMessageStream << "PdxInstance field `" << key << "` ";
        errorMessageStream << "triggered an unexpected GemFire exception, which might indicate the need ";
        errorMessageStream << "for another special case in node-gemfire: ";
        errorMessageStream << exception.getName() << ": " << exception.getMessage();

        NanThrowError(errorMessageStream.str().c_str());
        return NanEscapeScope(NanUndefined());
      }

      if (notArray) {
        // When reading as an array throws an exception, we learn we are dealing with
        // a non-array type. So we read as a CacheablePtr field instead.
        pdxInstance->getField(key, value);
      }

      v8Object->Set(NanNew(key), v8Value(value));
    }

    return NanEscapeScope(v8Object);
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NanEscapeScope(NanUndefined());
  }
}

Local<Value> v8Value(const CacheableInt64Ptr & valuePtr) {
  NanEscapableScope();

  static const int64_t maxSafeInteger = pow(2, 53) - 1;
  static const int64_t minSafeInteger = -1 * maxSafeInteger;

  int64_t value = static_cast<CacheableInt64Ptr>(valuePtr)->value();
  if (value > maxSafeInteger) {
    ConsoleWarn("Received 64 bit integer from GemFire greater than Number.MAX_SAFE_INTEGER (2^53 - 1)");
  } else if (value < minSafeInteger) {
    ConsoleWarn("Received 64 bit integer from GemFire less than Number.MIN_SAFE_INTEGER (-1 * 2^53 + 1)");
  }

  return NanEscapeScope(NanNew<Number>(value));
}

Local<Date> v8Value(const CacheableDatePtr & datePtr) {
  NanEscapableScope();

  double epochMillis = datePtr->milliseconds();

  return NanEscapeScope(NanNew<Date>(epochMillis));
}


Local<Value> v8Value(const CacheableKeyPtr & keyPtr) {
  return v8Value(static_cast<CacheablePtr>(keyPtr));
}

Local<Object> v8Value(const StructPtr & structPtr) {
  NanEscapableScope();

  Local<Object> v8Object(NanNew<Object>());

  unsigned int length = structPtr->length();
  for (unsigned int i = 0; i < length; i++) {
    v8Object->Set(NanNew(structPtr->getFieldName(i)),
                  v8Value((*structPtr)[i]));
  }

  return NanEscapeScope(v8Object);
}

Local<Object> v8Value(const HashMapOfCacheablePtr & hashMapPtr) {
  return v8Object(hashMapPtr);
}

Local<Array> v8Value(const VectorOfCacheableKeyPtr & vectorPtr) {
  return v8Array(vectorPtr);
}

Local<Object> v8Value(const SelectResultsPtr & selectResultsPtr) {
  NanEscapableScope();

  Local<Object> selectResults(SelectResults::NewInstance(selectResultsPtr));

  return NanEscapeScope(selectResults);
}

Local<Boolean> v8Value(bool value) {
  NanEscapableScope();
  return NanEscapeScope(NanNew(value));
}

}  // namespace node_gemfire
