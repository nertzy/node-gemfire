const randomString = require('random-string');
const _ = require('lodash');
const async = require('async');

const gemfire = require('../..');
const cache = new gemfire.Cache('xml/ExampleClient.xml');
const region = cache.getRegion("exampleRegion");

console.log("node version " + process.version);
console.log("node-gemfire version " + gemfire.version);
console.log("GemFire version " + gemfire.gemfireVersion);

function smokeTest(callback) {
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

      callback();
    });
  });
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
function benchmark(numberOfPuts, title, functionToTest, callback) {
  region.clear();

  var start = process.hrtime();

  async.series([
    function(next) { functionToTest(numberOfPuts, next); },
    function(next) {
      const duration = process.hrtime(start);
      const nanoseconds = duration[0] * 1e9 + duration[1];
      const microseconds = nanoseconds / 1e3;
      const seconds = (microseconds / 1e6);

      const putsPerSecond = Math.round(numberOfPuts / seconds);
      const usecPerPut = Math.round(microseconds / numberOfPuts);

      console.log(
        title + " put:" , + usecPerPut + " usec/op " + putsPerSecond + " ops/sec"
      );

      next();
    }
  ], callback);
}

function putNValues(value) {
  return function(iterationCount, callback) {
    async.times(iterationCount, function(n, done) {
      suffix++;
      region.put(gemfireKey + suffix, value, function(error) {
        if(error) {
          throw error;
        }

        done();
      });
    }, function(error, results) {
      callback();
    });
  };
}

function benchmarkStrings(numberOfPuts, callback){
  return benchmark(numberOfPuts, "String", putNValues(stringValue), callback);
}

function benchmarkSimpleObjects(numberOfPuts, callback){
  return benchmark(numberOfPuts, "Simple object", putNValues({ foo: stringValue }), callback);
}

function benchmarkComplexObjects(numberOfPuts, callback){
  return benchmark(numberOfPuts, "Complex object", putNValues(randomObject), callback);
}

async.series([
  function(next){ smokeTest(next); },
  function(next){ benchmarkStrings(10000, next); },
  function(next){ benchmarkSimpleObjects(1000, next); },
  function(next){ benchmarkComplexObjects(100, next); }
], function(){
  require('./oql.js');
});
