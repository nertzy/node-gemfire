const async = require("async");
const errorMatchers = require("../support/error_matchers.js");

const cache = require("../support/factories.js").getCache();
const region = cache.getRegion("exampleRegion");

describe("OQL integration", function() {
  beforeEach(function() {
    this.addMatchers(errorMatchers);
    region.clear();
  });

  it("maps null JS object values to the OQL null", function(done) {
    async.series([
      function(next) {
        region.putAll({
          valueNull: {foo: null},
          valueUndefined: {foo: undefined},
          valueBar: {foo: "bar"}
        }, next);
      },

      function(next) {
        const query = "SELECT * FROM /exampleRegion WHERE foo = null;";
        cache.executeQuery(query, function(error, response){
          expect(error).not.toBeError();
          const results = response.toArray();
          expect(results).toEqual([ {foo: null} ]);
          next();
        });
      },

      function(next) {
        const query = "SELECT * FROM /exampleRegion WHERE foo != null;";
        cache.executeQuery(query, function(error, response){
          expect(error).not.toBeError();
          const results = response.toArray();
          expect(results.length).toEqual(2);
          expect(results).toContain({foo: undefined});
          expect(results).toContain({foo: "bar"});
          next();
        });
      },
    ], done);
  });
});
