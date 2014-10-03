const randomString = require('random-string');
const _ = require('lodash');
const async = require('async');

const gemfire = require('../..');
const cache = new gemfire.Cache('benchmark/xml/BenchmarkClient.xml');
const region = cache.getRegion("oqlBenchmark");

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

function executeQuery(callback) {
  cache.executeQuery(query, callback);
}

const queryCount = 4;
function benchmark(recordCount, callback) {
  async.series([
    function(next) { region.clear(next); },
    function(next) {
      region.executeFunction("io.pivotal.node_gemfire.BulkPut", [baseObject, 1])
        .on('end', next);
    },
    function(next) {
      region.executeFunction("io.pivotal.node_gemfire.BulkPut", [otherObject, recordCount - 1])
        .on('end', next);
    },
    function(next) {
      const start = process.hrtime();

      async.times(queryCount, function(n, done) {
        executeQuery(done);
      }, function(error, results) {
        const duration = process.hrtime(start);
        const nanoseconds = duration[0] * 1e9 + duration[1];
        const microseconds = nanoseconds / 1e3;
        const seconds = (microseconds / 1e6);

        const queriesPerSecond = Math.round(queryCount / seconds);
        const usecPerPut = Math.round(microseconds / queryCount);

        console.log(
          "OQL (" + recordCount + " entries): ", + usecPerPut + " usec/query " + queriesPerSecond + " queries/sec"
        );

        next();
      });
    }
  ], callback);
}

async.series([
  function(next){ benchmark(100, next); },
  function(next){ benchmark(1000, next); },
  function(next){ benchmark(5000, next); }
]);
