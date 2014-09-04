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
function putNValues(n, done){
  var success = 0;

  var i = 0;

  _.times(n, function(pair) {
    suffix++;
    region.put(gemfireKey + suffix, randomObject, function(error){
      if(error) {
        throw error;
      }

      i++;

      if(i == n) {
        done();
      }
    } );
  });
}

function benchmark(numberOfPuts, done){
  var start = microtime.now();

  putNValues(numberOfPuts, function(){
    var microseconds = microtime.now() - start;
    var seconds = (microseconds / 1000000);

    var putsPerSecond = Math.round(numberOfPuts / seconds);
    var usecPerPut = Math.round(microseconds / numberOfPuts);

    console.log(
      "(object) " + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
    );

    done();
  });
}

benchmark(1, function(){
  benchmark(10, function () {
    benchmark(100, function () {
    });
  });
});
