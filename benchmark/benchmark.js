var randomString = require('random-string');
var _ = require('lodash');
var microtime = require("microtime");
var nodePreGyp = require("node-pre-gyp");
var path = require("path");
var pivotalGemfirePath = nodePreGyp.find(
  path.resolve(path.join(__dirname,'../package.json'))
);
var pivotalGemfire = require(pivotalGemfirePath);

console.log("Gemfire version " + pivotalGemfire.version());

pivotalGemfire.put('smoke', 'test');
if(pivotalGemfire.get('smoke') !== 'test') {
  console.log("Smoke test failed.");
  exit(-1);
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

var key = randomString(keyOptions);
var value = randomString(valueOptions);

var suffix = 0;
function putNValues(n){
  _.times(n, function(pair) {
    suffix++;
    pivotalGemfire.put(key + suffix, value + suffix);
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

_.each([1, 10, 100, 1000, 10000, 100000], function(numberOfPuts){
  benchmark(numberOfPuts);
});

pivotalGemfire.close();
