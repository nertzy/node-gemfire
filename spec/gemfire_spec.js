var nodePreGyp = require('node-pre-gyp');
var path = require('path')
var pivotalGemfirePath = nodePreGyp.find(
  path.resolve(path.join(__dirname,'../package.json'))
);

describe("pivotal-gemfire", function() {
  var pivotalGemfire;

  beforeEach(function() {
    pivotalGemfire = require(pivotalGemfirePath);
  });

  describe(".hello", function() {
    it("says hello", function() {
      expect(pivotalGemfire.hello()).toEqual("hello");
    });
  });

  describe(".version", function() {
    it("returns the version string", function() {
      expect(pivotalGemfire.version()).toEqual("7.0.2.0");
    });
  });
});
