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
        expect(error).not.toBeNull();
        expect(stderr).toContain(message);
      });
    }

    it("throws an exception if the file is not found", function(done) {
      var expectedMessage = 'I/O warning : failed to load external entity "/bad/path.xml"';
      expectExternalFailure("missing_xml_file", done, expectedMessage);
    });

    it("throws an exception if setReadSerialized not set to true", function(done) {
      var expectedMessage = "<pdx read-serialized='true' /> must be set in your cache xml";
      expectExternalFailure("not_pdx_read_serialized", done, expectedMessage);
    });

    it("accepts an xml file path", function(done) {
      expectExternalSuccess("correct_xml_file", done);
    });

    it("throws an exception if there are no parameters", function(done) {
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
          var query = "SELECT DISTINCT * FROM /exampleRegion";

          var results = cache.executeQuery(query).toArray();

          expect(results.length).toEqual(2);

          expect(results).toContain("a string");
          expect(results).toContain("another string");

          done();
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
          var query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";
          var results = cache.executeQuery(query).toArray();

          expect(results.length).toEqual(1);

          expect(results).toContain("another string");

          done();
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
          const results = cache.executeQuery(query).toArray();

          expect(results.length).toEqual(1);
          expect(results).toContain(object);

          done();
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
          var query = "SELECT record FROM /exampleRegion AS record, record.foo AS foo WHERE foo.bar = 'baz'";
          var results = cache.executeQuery(query).toArray();

          expect(results.length).toEqual(1);
          expect(results).toContain(object1);

          done();
        }
      );
    });

    it("executes a query that returns a GemFire struct", function(done) {
      region.put("object", { foo: 1, bar: 2, baz: 3 }, function(error) {
        expect(error).toBeNull();

        const query = "SELECT foo, bar FROM /exampleRegion";
        const results = cache.executeQuery(query).toArray();

        expect(results.length).toEqual(1);
        expect(results[0]).toEqual({ foo: 1, bar: 2 });

        done();
      });
    });

    it("executes a query that can retrieve results of all types", function(done) {
      async.parallel(
        [
          function(callback) { region.put("a string", "a string", callback); },
          function(callback) { region.put("an object", {"an": "object"}, callback); },
        ],
        function(){
          var query = "SELECT DISTINCT * FROM /exampleRegion";

          var results = cache.executeQuery(query).toArray();

          expect(results.length).toEqual(2);

          expect(results).toContain({"an": "object"});
          expect(results).toContain("a string");

          done();
        }
      );
    });

    it("can search for wide strings", function(){
      async.parallel(
        [
          function(callback) { region.put("narrow string", "Japan", callback); },
          function(callback) { region.put("wide string", "日本", callback); },
        ],
        function(){
          var narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';";
          var narrowResults = cache.executeQuery(narrowQuery).toArray();
          expect(narrowResults).toEqual(["narrow string"]);

          var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
          var wideResults = cache.executeQuery(wideQuery).toArray();
          expect(wideResults).toEqual(["wide string"]);
        }
      );
    });

    it("throws an error for invalid queries", function() {
      var exception;

      try {
        cache.executeQuery("INVALID;");
      } catch(e) {
        exception = e;
      }

      expect(exception).toBeDefined();
      expect(exception.message).toMatch(/gemfire::QueryException/);
    });

    it("throws an error when no query is passed", function() {
      function callWithoutQuery() {
        cache.executeQuery();
      }

      expect(callWithoutQuery).toThrow("You must pass a query string to executeQuery()");
    });

    describe("asynchronous API", function() {
      it("returns the cache for chaining", function(done) {
        var query = "SELECT DISTINCT * FROM /exampleRegion;";
        var returnValue = cache.executeQuery(query, function(error, results) {
          done();
        });
        expect(returnValue).toEqual(cache);
      });

      it("supports async query execution when a callback is passed", function(done) {
        async.parallel(
          [
            function(callback) { region.put("narrow string", "Japan", callback); },
            function(callback) { region.put("wide string", "日本", callback); }
          ],
          function() {
            var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
            cache.executeQuery(wideQuery, function(error, results){
              expect(error).toBeNull();
              expect(results.toArray()).toEqual(["wide string"]);
              done();
            });
          }
        );
      });

      it("passes an error to the callback for invalid queries", function(done) {
        var exception;

        cache.executeQuery("INVALID;", function(error, results) {
          expect(error.message).toMatch(/gemfire::QueryException/);
          expect(results).toBeUndefined();
          done();
        });
      });

      it("throws an error for missing queries", function() {
        function callWithoutQuery() {
          cache.executeQuery(function(){});
        }

        expect(callWithoutQuery).toThrow("You must pass a query string to executeQuery()");
      });
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the cache", function() {
      expect(factories.getCache().inspect()).toEqual('[Cache]');
    });
  });
});
