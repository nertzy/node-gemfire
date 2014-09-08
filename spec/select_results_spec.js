const async = require('async');
const cache = require("./support/factories.js").getCache();

describe("SelectResults", function() {
  var selectResults;

  beforeEach(function(done) {
    const region = cache.getRegion('exampleRegion');

    region.clear();

    async.series([
      function(callback) { region.put("1", "one", callback); },
      function(callback) { region.put("2", "two", callback); },
      function(callback) { region.put("3", "three", callback); },
    ],
    function() {
      selectResults = cache.executeQuery("SELECT * FROM /exampleRegion");
      done();
    });
  });

  describe("toArray", function() {
    it("returns the results as an array", function() {
      const array = selectResults.toArray();
      expect(array.length).toEqual(3);
      expect(array).toContain("one");
      expect(array).toContain("two");
      expect(array).toContain("three");
    });
  });

  describe("each", function(){
    it("returns itself for chaining", function() {
      expect(selectResults.each(function(){})).toEqual(selectResults);
    });

    it("calls the callback once per result, passing in the result", function() {
      const callback = jasmine.createSpy();

      selectResults.each(callback);

      expect(callback.callCount).toEqual(3);
      expect(callback).toHaveBeenCalledWith("one");
      expect(callback).toHaveBeenCalledWith("two");
      expect(callback).toHaveBeenCalledWith("three");
    });

    it("throws an error if a callback is not passed", function() {
      function callWithoutCallback() {
        selectResults.each();
      }

      expect(callWithoutCallback).toThrow("You must pass a callback to each()");
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the select results", function() {
      expect(selectResults.inspect()).toEqual('[SelectResults size=3]');
    });
  });
});
