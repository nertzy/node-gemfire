#include <gfcpp/GemfireCppCache.hpp>
#include <assert.h>
#include <chrono>
#include <sstream>

#define VALUE_SIZE (15 * 1024)

using namespace gemfire;
using namespace std::chrono;

char * value(const int len) {
  char * s = new char[len];

  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
  return s;
}

long long int item = 0;
void putValues(int numberOfPuts, RegionPtr regionPtr, char * testValue){
  for (int n = 0; n < numberOfPuts; n++)
  {
    regionPtr->put(std::to_string(item).c_str(), (testValue + std::to_string(item)).c_str());
    item++;
  }
}

void benchmark(int numberOfPuts, RegionPtr regionPtr) {
  char * testValue = value(VALUE_SIZE);

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  putValues(numberOfPuts, regionPtr, testValue);

  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  int usecPerPut = time_span.count() * 1000000 / numberOfPuts;
  int putsPerSecond = numberOfPuts / time_span.count();

  std::stringstream stream;
  stream << numberOfPuts << " puts: " << usecPerPut << " usec/put " << \
    putsPerSecond << " puts/sec";
  LOGINFO(stream.str().c_str());
}

int main(int argc, char ** argv)
{
  CachePtr cachePtr;

  try
  {
    CacheFactoryPtr cacheFactory = CacheFactory::createCacheFactory();
    cachePtr = cacheFactory
      ->set("log-level", "info")
      ->create();
    LOGINFO("Created the GemFire Cache");

    RegionFactoryPtr regionFactory = cachePtr->createRegionFactory(LOCAL);
    LOGINFO("Created the RegionFactory");

    RegionPtr regionPtr = regionFactory->create("exampleRegion");
    LOGINFO("Created the Region Programmatically.");

    regionPtr->put("smoke", "test");
    CacheableStringPtr smokeTestOutput = regionPtr->get("smoke");
    assert(!strcmp(smokeTestOutput->asChar(), "test"));
    LOGINFO("Passed the smoke test");

    putValues(1000, regionPtr, value(VALUE_SIZE));

    benchmark(1, regionPtr);
    benchmark(10, regionPtr);
    benchmark(100, regionPtr);
    benchmark(1000, regionPtr);
    benchmark(10000, regionPtr);
    benchmark(100000, regionPtr);
  }
  // An exception should not occur
  catch(const Exception & gemfireExcp)
  {
    LOGERROR("GemFire Exception: %s", gemfireExcp.getMessage());
  }

  // Close the GemFire Cache.
  cachePtr->close();
  LOGINFO("Closed the GemFire Cache");
}

