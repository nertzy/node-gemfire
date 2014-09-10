var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");

var gemfire = require('../..');
var cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
var region = cache.getRegion("exampleRegion");

const Q = require("q");

console.log("node-gemfire version " + gemfire.version);
console.log("GemFire version " + gemfire.gemfireVersion);

function smokeTest() {
  var deferred = Q.defer();

  region.put('smoke', { test: 'value' }, function(error){
    if(error) {
      throw error;
    }

    region.get('smoke', function(error, value) {
      if(error) {
        throw error;
      }

      if(JSON.stringify(value) !== JSON.stringify({ test: 'value' })) {
        throw "Smoke test failed.";
      }

      deferred.resolve();
    });
  });

  return deferred.promise;
}

var keyOptions = {
  length: 8,
  numeric: true,
  letter: true,
  special: false
};

var valueOptions = {
  length: 15 * 1024,
  numeric: true,
  letter: true,
  special: true
};

var randomObject = require('../data/randomObject.json');
var stringValue = randomString(valueOptions);
var gemfireKey = randomString(keyOptions);

var suffix = 0;
function benchmark(numberOfPuts, title, callback) {
  region.clear();

  var deferred = Q.defer();
  var start = microtime.now();

  callback(numberOfPuts).then(function(){
    var microseconds = microtime.now() - start;
    var seconds = (microseconds / 1000000);

    var putsPerSecond = Math.round(numberOfPuts / seconds);
    var usecPerPut = Math.round(microseconds / numberOfPuts);

    console.log(
      "(" + title + ") " + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
    );

    deferred.resolve();
  });

  return deferred.promise;
}

function putNValues(value) {
  var deferred = Q.defer();
  var successes = 0;

  return function(iterationCount) {
    function putCallback(error) {
      if(error) {
        throw error;
      } else {
        successes++;

        if(successes == iterationCount) {
          deferred.resolve();
        }
      }
    }

    for(var i = 0; i < iterationCount; i++) {
      suffix++;
      region.put(gemfireKey + suffix, value, putCallback);
    }

    return deferred.promise;
  };
}

function benchmarkStrings(numberOfPuts){
  return benchmark(numberOfPuts, "string", putNValues(stringValue));
}

function benchmarkSimpleObjects(numberOfPuts){
  return benchmark(numberOfPuts, "simple object", putNValues({ foo: stringValue }));
}

function benchmarkComplexObjects(numberOfPuts){
  return benchmark(numberOfPuts, "complex object", putNValues(randomObject));
}

Q()
  .then(function(){ return smokeTest(); })
  .then(function(){ return benchmarkStrings(10000); })
  .then(function(){ return benchmarkSimpleObjects(1000); })
  .then(function(){ return benchmarkComplexObjects(100); })
  .done();
