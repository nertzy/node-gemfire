var childProcess = require('child_process');
const gemfire = require("./support/gemfire.js");
const factories = require("./support/factories.js");
const async = require("async");

describe("gemfire.Cache", function() {
  afterEach(function(done) {
    setTimeout(done, 0);
  });

  describe("constructor", function(){
    function runExternalTest(name, done, callback) {
      if(!done) { throw("You must run this test asynchronously and pass done");  }
      if(!callback) { throw("You must pass a callback");  }

      var filename = "spec/support/cache/" + name + ".js";

      childProcess.execFile("node", [filename], function(error, stdout, stderr) {
        callback(error, stdout, stderr);
        done();
      });
    }

    function expectExternalSuccess(name, done){
      runExternalTest(name, done, function(error, stdout, stderr) {
        expect([error, stderr]).toEqual([null, '']);
      });
    }

    function expectExternalFailure(name, done, message){
      runExternalTest(name, done, function(error, stdout, stderr) {
        expect(error).toBeTruthy();
        expect(stderr).toContain(message);
      });
    }

    it("throws an error if the file is not found", function(done) {
      var expectedMessage = 'I/O warning : failed to load external entity "/bad/path.xml"';
      expectExternalFailure("missing_xml_file", done, expectedMessage);
    });

    it("throws an error if setReadSerialized not set to true", function(done) {
      var expectedMessage = "<pdx read-serialized='true' /> must be set in your cache xml";
      expectExternalFailure("not_pdx_read_serialized", done, expectedMessage);
    });

    it("accepts an xml file path", function(done) {
      expectExternalSuccess("correct_xml_file", done);
    });

    it("throws an error if there are no parameters", function(done) {
      expectExternalFailure("no_parameters", done, "Cache constructor requires a path to an XML configuration file as its first argument.");
    });
  });

  describe(".getRegion", function() {
    var cache;

    beforeEach(function() {
      cache = factories.getCache();
    });

    it("validates arguments", function(){
      function callWithZeroArguments(){
        cache.getRegion();
      }
      function callWithOneArgument(){
        cache.getRegion("exampleRegion");
      }
      function callWithTwoArguments(){
        cache.getRegion("exampleRegion", "foo");
      }

      expect(callWithZeroArguments).toThrow("getRegion expects one argument: the name of a Gemfire region");
      expect(callWithOneArgument).not.toThrow();
      expect(callWithTwoArguments).toThrow("getRegion expects one argument: the name of a Gemfire region");
    });

    it("returns a gemfire.Region object", function() {
      var region = cache.getRegion("exampleRegion");
      expect(region.constructor.name).toEqual("Region");
    });

    it("returns undefined if the region is unknown", function(){
      expect(cache.getRegion("there is no such region")).toBeUndefined();
    });
  });

  describe("executeQuery", function () {
    var cache, region;

    beforeEach(function() {
      cache = factories.getCache();
      region = cache.getRegion("exampleRegion");
      region.clear();
    });

    it("executes a query that can retrieve string results", function(done) {
      async.parallel(
        [
          function(callback) { region.put("string1", "a string", callback); },
          function(callback) { region.put("string2", "another string", callback); },
          function(callback) { region.put("string3", "a string", callback); },
        ],
        function(){
          const query = "SELECT DISTINCT * FROM /exampleRegion";

          cache.executeQuery(query, function(error, response) {
            expect(error).toBeFalsy();
            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain("a string");
            expect(results).toContain("another string");

            done();
          });
        }
      );
    });

    it("executes a query with an OQL predicate", function(done) {
      async.parallel(
        [
          function(callback) { region.put("string1", "a string", callback); },
          function(callback) { region.put("string2", "another string", callback); },
        ],
        function() {
          const query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";

          cache.executeQuery(query, function(error, response) {
            expect(error).toBeFalsy();
            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain("another string");

            done();
          });
        }
      );
    });

    it("executes a query with a where clause on a field", function(done){
      const object = { foo: 'bar' };

      async.parallel(
        [
          function(callback) { region.put("object", object, callback); },
          function(callback) { region.put("other object", { foo: 'qux' }, callback); },
          function(callback) { region.put("empty", {}, callback); },
        ],
        function() {
          const query = "SELECT * FROM /exampleRegion WHERE foo = 'bar'";

          cache.executeQuery(query, function(error, response) {
            expect(error).toBeFalsy();

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object);

            done();
          });
        }
      );
    });

    it("executes a query with a nested object", function(done) {
      const object1 = { foo: [{ bar: 'baz' }] };

      async.parallel(
        [
          function(callback) { region.put("object1", object1, callback); },
          function(callback) { region.put("object2", { foo: [{ bar: 'qux' }] }, callback); },
        ],
        function(){
          const query = "SELECT record FROM /exampleRegion AS record, record.foo AS foo WHERE foo.bar = 'baz'";

          cache.executeQuery(query, function(error,response) {
            expect(error).toBeFalsy();

            const results = response.toArray();

            expect(results.length).toEqual(1);
            expect(results).toContain(object1);

            done();
          });
        }
      );
    });

    it("executes a query that returns a GemFire struct", function(done) {
      region.put("object", { foo: 1, bar: 2, baz: 3 }, function(error) {
        expect(error).toBeFalsy();

        const query = "SELECT foo, bar FROM /exampleRegion";

        cache.executeQuery(query, function(error, response){
          expect(error).toBeFalsy();
          const results = response.toArray();

          expect(results.length).toEqual(1);
          expect(results[0]).toEqual({ foo: 1, bar: 2 });

          done();
        });
      });
    });

    it("executes a query that can retrieve results of all types", function(done) {
      async.parallel(
        [
          function(callback) { region.put("a string", "a string", callback); },
          function(callback) { region.put("an object", {"an": "object"}, callback); },
        ],
        function(){
          const query = "SELECT DISTINCT * FROM /exampleRegion";

          cache.executeQuery(query, function(error, response) {
            expect(error).toBeFalsy();
            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain({"an": "object"});
            expect(results).toContain("a string");

            done();
          });
        }
      );
    });

    it("can search for wide strings", function(done){
      async.series(
        [
          function(callback) { region.put("narrow string", "Japan", callback); },
          function(callback) { region.put("wide string", "日本", callback); },
          function(callback) {
            const narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';";
            cache.executeQuery(narrowQuery, function(error, response){
              expect(error).toBeFalsy();
              const results = response.toArray();
              expect(results).toEqual(["narrow string"]);
              callback();
            });
          },
          function(callback) {
            const wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
            cache.executeQuery(wideQuery, function(error, response){
              expect(error).toBeFalsy();
              const results = response.toArray();
              expect(results).toEqual(["wide string"]);
              callback();
            });
          },
        ],
        done
      );
    });

    it("throws an error when no query is passed", function() {
      function callWithoutQuery() {
        cache.executeQuery();
      }

      expect(callWithoutQuery).toThrow("You must pass a query string and callback to executeQuery().");
    });

    it("throws an error if you don't pass a callback", function(){
      function callWithoutCallback() {
        cache.executeQuery("SELECT * FROM /exampleRegion");
      }

      expect(callWithoutCallback).toThrow("You must pass a callback to executeQuery().");
    });

    it("throws an error if you pass a non-function as the callback", function(){
      function callWithNonCallback() {
        cache.executeQuery("SELECT * FROM /exampleRegion", "Not a callback");
      }

      expect(callWithNonCallback).toThrow("You must pass a function as the callback to executeQuery().");
    });

    it("returns the cache for chaining", function(done) {
      var query = "SELECT DISTINCT * FROM /exampleRegion;";
      var returnValue = cache.executeQuery(query, function(error, results) {
        done();
      });
      expect(returnValue).toEqual(cache);
    });

    it("passes an error to the callback for invalid queries", function(done) {
      var exception;

      cache.executeQuery("INVALID;", function(error, results) {
        expect(error.message).toMatch(/gemfire::QueryException/);
        expect(results).toBeUndefined();
        done();
      });
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the cache", function() {
      expect(factories.getCache().inspect()).toEqual('[Cache]');
    });
  });
});
