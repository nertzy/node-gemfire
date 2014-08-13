var childProcess = require('child_process');

describe("gemfire.Cache", function() {
  var gemfire, cache;

  beforeEach(function() {
    gemfire = require("../gemfire.js");
    cache = new gemfire.Cache();
    gemfire.clear();
  });

  afterEach(function(done) {
    setTimeout(done, 0);
  });

  describe("executeQuery", function () {
    it("executes a query that can retrieve string results", function() {
      gemfire.put("string1", "a string");
      gemfire.put("string2", "another string");
      gemfire.put("string3", "a string");

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain("a string");
      expect(results).toContain("another string");
    });

    it("executes a query with an OQL predicate", function() {
      gemfire.put("string1", "a string");
      gemfire.put("string2", "another string");

      var query = "SELECT entry.value FROM /exampleRegion.entries entry WHERE entry.key = 'string2'";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(1);

      expect(results).toContain("another string");
    });

    it("executes a query that can retrieve results of all types", function() {
      gemfire.put("a string", "a string");
      gemfire.put("an object", {"an": "object"});

      var query = "SELECT DISTINCT * FROM /exampleRegion";

      var results = cache.executeQuery(query);

      expect(results.length).toEqual(2);

      expect(results).toContain({"an": "object"});
      expect(results).toContain("a string");
    });

    it("can search for wide strings", function(){
      gemfire.put("narrow string", "Japan");
      gemfire.put("wide string", "日本");

      var narrowQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = 'Japan';"
      var narrowResults = cache.executeQuery(narrowQuery);
      expect(narrowResults).toEqual(["narrow string"]);

      var wideQuery = "SELECT key FROM /exampleRegion.entrySet WHERE value = '日本';"
      var wideResults = cache.executeQuery(wideQuery);
      expect(wideResults).toEqual(["wide string"]);
    });
  });
});
