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
    describe("with objects", function() {
      it("returns the value and stores the object in the cache", function() {
        expect(pivotalGemfire.put("foo", { foo: "bar" })).toEqual({ foo: "bar" });
        expect(pivotalGemfire.get("foo")).toEqual({ foo: "bar" });
      });
    });
  });

  describe(".get", function() {
    describe("with objects", function() {
      describe("one level deep, made of strings", function() {
        it("returns the object from the cache", function() {
          pivotalGemfire.put("key", { foo: "bar" });
          expect(pivotalGemfire.get("key")).toEqual({ foo: "bar" });
        });
      });
    });
  });

  describe("cleanup", function(){
    it("is not actually a test", function(){
      pivotalGemfire.close();
    });
  });
});
