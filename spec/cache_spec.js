var childProcess = require('child_process');

describe("gemfire.Cache", function() {
  var gemfire, cache;

  beforeEach(function() {
    gemfire = require("../gemfire.js");
    cache = new gemfire.Cache();
  });

  afterEach(function(done) {
    setTimeout(done, 0);
  });

  describe(".getRegion", function() {
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
    })

    it("returns a gemfire.Region object", function() {
      var region = cache.getRegion("exampleRegion");
      expect(region.constructor).toEqual(gemfire.Region);
    });

    it("returns undefined if the region is unknown", function(){
      expect(cache.getRegion("there is no such region")).toBeUndefined();
    });
  });

  describe("executeQuery", function () {
    var region;

    beforeEach(function() {
      exampleRegion = cache.getRegion("exampleRegion");
      exampleRegion.clear();
    });

    it("executes a query that can retrieve string results", function() {
      exampleRegion.put("string1", "a string");
      exampleRegion.put("string2", "another string");
      exampleRegion.put("string3", "a string");

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain("a string");
      expect(results).toContain("another string");
    });

    it("executes a query with an OQL predicate", function() {
      exampleRegion.put("string1", "a string");
      exampleRegion.put("string2", "another string");

      var query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(1);

      expect(results).toContain("another string");
    });

    it("executes a query that can retrieve results of all types", function() {
      exampleRegion.put("a string", "a string");
      exampleRegion.put("an object", {"an": "object"});

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain({"an": "object"});
      expect(results).toContain("a string");
    });

    it("can search for wide strings", function(){
      exampleRegion.put("narrow string", "Japan");
      exampleRegion.put("wide string", "日本");

      var narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';"
      var narrowResults = cache.executeQuery(narrowQuery);
      expect(narrowResults).toEqual(["narrow string"]);

      var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';"
      var wideResults = cache.executeQuery(wideQuery);
      expect(wideResults).toEqual(["wide string"]);
    });
  });
});
