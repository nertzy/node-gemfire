const randomString = require('random-string');
const _ = require('lodash');
const microtime = require("microtime");
const async = require('async');

const gemfire = require('../..');
const cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
const region = cache.getRegion("oqlBenchmark");

console.log("node-gemfire version " + gemfire.version);
console.log("GemFire version " + gemfire.gemfireVersion);

const keyOptions = {
  length: 8,
  numeric: true,
  letter: true,
  special: false
};

const baseObject = {
   "name": "Jane Doe",
   "addresses": [
     { "phoneNumbers":
       [
         { "number": "212-987-5440" },
         { "number": "717-734-2230" }
       ]
     },
     { "city": "New York" }
   ]
 };
const otherObject = {
   "name": "Jane Doe",
   "addresses": [
     { "phoneNumbers":
       [
         { "number": "555-555-1212" },
         { "number": "415-77-PIVOT" }
       ]
     },
     { "city": "New York" }
   ]
 };

/* jshint multistr:true */
const query = "SELECT person.name \
FROM \
  (SELECT * FROM /oqlBenchmark jr \
   WHERE is_defined(jr.addresses)) person, \
  (SELECT * FROM person.addresses) a \
WHERE is_defined(a.phoneNumbers) \
  AND '212-987-5440' IN ( \
    SELECT n.number FROM a.phoneNumbers n)";
console.log("query: ", query);

function executeQuery(callback) {
  cache.executeQuery(query, callback);
}

const queryCount = 10;
function benchmark(recordCount, callback) {
  region.clear();
  region.executeFunction("io.pivotal.node_gemfire.BulkPut", [baseObject, 1], function(){});
  region.executeFunction("io.pivotal.node_gemfire.BulkPut", [otherObject, recordCount - 1], function(){});

  const start = microtime.now();

  async.times(queryCount, function(n, done) {
    executeQuery(done);
  }, function(error, results) {
    const microseconds = microtime.now() - start;
    const seconds = (microseconds / 1000000);

    const queriesPerSecond = Math.round(queryCount / seconds);
    const usecPerPut = Math.round(microseconds / queryCount);

    console.log(
      "" + recordCount + " records: ", + usecPerPut + " usec/query " + queriesPerSecond + " queries/sec"
    );

    callback();
  });
}

async.series([
  function(next){ benchmark(10, next); },
  function(next){ benchmark(20, next); },
  function(next){ benchmark(100, next); },
  function(next){ benchmark(200, next); },
  function(next){ benchmark(500, next); },
  function(next){ benchmark(1000, next); },
  function(next){ benchmark(2000, next); },
  function(next){ benchmark(5000, next); }
]);
