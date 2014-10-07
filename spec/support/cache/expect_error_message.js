const util = require("util");

module.exports = function expectErrorMessage(error, expectedMessage) {
  console.log(error);
  if(!error.message.match(expectedMessage)) {
    throw new Error(
      "Error message didn't match expectation:\n" +
      "Expected: " + util.inspect(expectedMessage) + "\n" +
      "  Actual: " + util.inspect(error.message)
    );
  }
};
