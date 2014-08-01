var nodePreGyp = require('node-pre-gyp');
var path = require('path')
var pivotalGemfirePath = nodePreGyp.find(
  path.resolve(path.join(__dirname,'../package.json'))
);

describe("pivotal-gemfire", function() {
  var pivotalGemfire;

  beforeEach(function() {
    pivotalGemfire = require(pivotalGemfirePath);
    pivotalGemfire.clear();
  });

  describe(".version", function() {
    it("returns the version string", function() {
      expect(pivotalGemfire.version()).toEqual("7.0.2.0");
    });
  });

  describe(".put", function() {
    it("returns the value", function() {
      expect(pivotalGemfire.put("foo", "42")).toEqual("42");
    });
  });

  describe(".get", function() {
    it("returns the values from the cache", function() {
      pivotalGemfire.put("my key", "foo")
      expect(pivotalGemfire.get("my key")).toEqual("foo");

      pivotalGemfire.put("my key", "bar")
      expect(pivotalGemfire.get("my key")).toEqual("bar");
    });
  });

  describe("cleanup", function(){
    it("is not actually a test", function(){
      pivotalGemfire.close();
    });
  });
});
