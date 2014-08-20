var childProcess = require('child_process');
const gemfire = require("..");
const factories = require("./support/factories.js");

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
      expect(region.constructor).toEqual(gemfire.Region);
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

    it("executes a query that can retrieve string results", function() {
      region.put("string1", "a string");
      region.put("string2", "another string");
      region.put("string3", "a string");

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain("a string");
      expect(results).toContain("another string");
    });

    it("executes a query with an OQL predicate", function() {
      region.put("string1", "a string");
      region.put("string2", "another string");

      var query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(1);

      expect(results).toContain("another string");
    });

    it("executes a query that can retrieve results of all types", function() {
      region.put("a string", "a string");
      region.put("an object", {"an": "object"});

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain({"an": "object"});
      expect(results).toContain("a string");
    });

    it("can search for wide strings", function(){
      region.put("narrow string", "Japan");
      region.put("wide string", "日本");

      var narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';";
      var narrowResults = cache.executeQuery(narrowQuery);
      expect(narrowResults).toEqual(["narrow string"]);

      var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
      var wideResults = cache.executeQuery(wideQuery);
      expect(wideResults).toEqual(["wide string"]);
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

    describe("asynchronous API", function() {
      it("returns the cache for chaining", function(done) {
        var query = "SELECT DISTINCT * FROM /exampleRegion;";
        var returnValue = cache.executeQuery(query, function(error, results) {
          done();
        });
        expect(returnValue).toEqual(cache);
      });

      it("supports async query execution when a callback is passed", function(done) {
        region.put("narrow string", "Japan");
        region.put("wide string", "日本");

        var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';";
        cache.executeQuery(wideQuery, function(error, results){
          expect(error).toBeNull();
          expect(results).toEqual(["wide string"]);
          done();
        });
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
  });
});
