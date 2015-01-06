const async = require('async');
const cache = require("./support/factories.js").getCache();
const errorMatchers = require("./support/error_matchers.js");

describe("SelectResults", function() {
  var selectResults;

  beforeEach(function(done) {
    jasmine.addMatchers(errorMatchers);
    const region = cache.getRegion('exampleRegion');

    async.series([
      function(next) { region.clear(next); },
      function(next) {
        region.putAll({
          "1": "one",
          "2": "two",
          "3": "three"
        }, next);
      },
      function(next) {
        cache.executeQuery("SELECT * FROM /exampleRegion", {poolName: "myPool"}, function(error, response){
          expect(error).not.toBeError();
          selectResults = response;
          next();
        });
      }
    ], done);
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

      expect(callback.calls.count()).toEqual(3);
      expect(callback).toHaveBeenCalledWith("one");
      expect(callback).toHaveBeenCalledWith("two");
      expect(callback).toHaveBeenCalledWith("three");
    });

    it("throws an error if a callback is not passed", function() {
      function callWithoutCallback() {
        selectResults.each();
      }

      expect(callWithoutCallback).toThrow(new Error("You must pass a callback to each()"));
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the select results", function() {
      expect(selectResults.inspect()).toEqual('[SelectResults size=3]');
    });
  });
});
