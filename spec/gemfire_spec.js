describe("gemfire", function() {
  describe(".version", function() {
    it("returns the version string", function() {
      var gemfire = require("../gemfire.js");
      expect(gemfire.version()).toEqual("7.0.2.0");
    });
  });
});
