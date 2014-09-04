var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");

var gemfire = require('../..');
var cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
var region = cache.getRegion("exampleRegion");

region.clear();

console.log("node-gemfire version " + gemfire.version);
console.log("GemFire version " + gemfire.gemfireVersion);

region.put('smoke', { test: 'value' });
if(JSON.stringify(region.get('smoke')) !== JSON.stringify({ test: 'value' })) {
  throw "Smoke test failed.";
}

var keyOptions = {
  length: 8,
  numeric: true,
  letter: true,
  special: false
};

var randomObject = require('../data/randomObject.json');
var gemfireKey = randomString(keyOptions);

var suffix = 0;
function putNObjects(n){
  _.times(n, function() {
    suffix++;
    region.put(gemfireKey + suffix, randomObject);
  });
}

var stringValueOptions = {
  length: 15 * 1024,
  numeric: true,
  letter: true,
  special: true
};

var stringValue = randomString(stringValueOptions);

function putNStrings(n){
  _.times(n, function() {
    suffix++;
    region.put(gemfireKey + suffix, stringValue);
  });
}

function benchmark(numberOfPuts, typeName, callback) {
  var start = microtime.now();

  callback();

  var microseconds = microtime.now() - start;
  var seconds = (microseconds / 1000000);

  var putsPerSecond = Math.round(numberOfPuts / seconds);
  var usecPerPut = Math.round(microseconds / numberOfPuts);

  console.log(
    "(" + typeName + ") " + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
  );
}

function benchmarkObjects(numberOfPuts){
  benchmark(numberOfPuts, "object", function () {
    putNObjects(numberOfPuts);
  });
}

function benchmarkStrings(numberOfPuts){
  benchmark(numberOfPuts, "string", function () {
    putNStrings(numberOfPuts);
  });
}

_.each([1, 10], function(numberOfPuts){
  benchmarkObjects(numberOfPuts);
});

_.each([100, 1000, 10000], function(numberOfPuts){
  benchmarkStrings(numberOfPuts);
});
