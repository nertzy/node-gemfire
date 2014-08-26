const randomString = require('random-string');
const _ = require('lodash');
const microtime = require("microtime");

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

function executeQuery() {
  cache.executeQuery(query);
}

const queryCount = 10;
function benchmark(recordCount) {
  region.clear();
  region.executeFunction("io.pivotal.node_gemfire.BulkPut", [baseObject, 1]);
  region.executeFunction("io.pivotal.node_gemfire.BulkPut", [otherObject, recordCount - 1]);

  const start = microtime.now();

  _.times(queryCount, function() {
    executeQuery();
  });

  const microseconds = microtime.now() - start;
  const seconds = (microseconds / 1000000);

  const queriesPerSecond = Math.round(queryCount / seconds);
  const usecPerPut = Math.round(microseconds / queryCount);

  console.log(
    "" + recordCount + " records: ", + usecPerPut + " usec/query " + queriesPerSecond + " queries/sec"
  );
}

_.each([10, 20, 50, 100, 200, 500, 1000, 2000, 5000], function(recordCount) {
  benchmark(recordCount);
});
