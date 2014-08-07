var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");
var pivotalGemfire = require('../../gemfire.js');

console.log("Gemfire version " + pivotalGemfire.version());

pivotalGemfire.put('smoke', { test: 'value' });
if(JSON.stringify(pivotalGemfire.get('smoke')) !== JSON.stringify({ test: 'value' })) {
  throw "Smoke test failed.";
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

var gemfireKey = randomString(keyOptions);
var str = randomString(valueOptions);

function objectForSuffix(suffix) {
  var JSKey = "foo" + suffix;
  var JSValue = str + suffix;

  var object = new Object;
  object[JSKey] = JSValue;
  return object;
};

var suffix = 0;
function putNValues(n){
  _.times(n, function(pair) {
    suffix++;
    pivotalGemfire.put(gemfireKey + suffix, objectForSuffix(suffix));
  });
}

function benchmark(numberOfPuts){
  var start = microtime.now();

  putNValues(numberOfPuts);

  var microseconds = microtime.now() - start;
  var seconds = (microseconds / 1000000);

  var putsPerSecond = Math.round(numberOfPuts / seconds);
  var usecPerPut = Math.round(microseconds / numberOfPuts);

  console.log(
    "" + numberOfPuts + " puts: ", + usecPerPut + " usec/put " + putsPerSecond + " puts/sec"
  );
}

putNValues(1000);

_.each([1, 10, 100, 1000, 10000], function(numberOfPuts){
  benchmark(numberOfPuts);
});

pivotalGemfire.close();
