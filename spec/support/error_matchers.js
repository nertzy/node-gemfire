const inspect = require('util').inspect;

// For now, we don't care if it's actually an instance of Error.
function isError(error) {
  return !!error;
}

function isErrorWithName(error, expectedName) {
  return isError(error) && (expectedName === error.name);
}

function isErrorWithNameAndMessage(error, expectedName, expectedMessage) {
  if (!isErrorWithName(error, expectedName)) {
    return false;
  }

  if (expectedMessage.constructor === RegExp) {
    return expectedMessage.test(error.message);
  } else {
    return error.message === expectedMessage;
  }
}

exports.toBeError = function toBeError(util) {
  return {
    compare: function compare(actual, expectedName, expectedMessage) {
      const isNot = this.isNot;

      if(isNot && (expectedName || expectedMessage)) {
        throw("not.toBeError() matcher received unexpected argument");
      }

      var result = {};

      result.message = function message() {
        if(isNot) {
          return "Expected " + inspect(actual) + " not to be an error";
        }

        var messageString = "Expected " + inspect(actual) + " to be an error";

        if(expectedName) {
          messageString = messageString + " with name " + inspect(expectedName);
        }

        if(expectedMessage) {
          if(expectedMessage.constructor === RegExp) {
            return messageString + " with message matching " + inspect(expectedMessage);
          } else {
            return messageString + " with message " + inspect(expectedMessage);
          }
        } else {
          return messageString;
        }
      };

      if(expectedName) {
        if(expectedMessage) {
          result.pass = isErrorWithNameAndMessage(actual, expectedName, expectedMessage);
        } else {
          result.pass = isErrorWithName(actual, expectedName);
        }
      } else {
        result.pass = isError(actual);
      }

      return result;
    }
  };
};

exports.toThrowNamedError = function toThrowNamedError(util) {
  return {
    compare: function(actual, expectedName, expectedMessage) {
      var result = { pass: false },
        threw = false,
        thrown;

      if (typeof actual != 'function') {
        throw new Error('Actual is not a Function');
      }

      try {
        actual();
      } catch (e) {
        threw = true;
        thrown = e;
      }

      if (!threw) {
        result.message = 'Expected function to throw an exception.';
        return result;
      }

      const expectedError = new Error(expectedMessage);
      expectedError.name = expectedName;

      if (thrown.message == expectedMessage && thrown.name == expectedName) {
        result.pass = true;
        result.message = function() {
          return 'Expected function not to throw ' + inspect(expectedError);
        };
      } else {
        result.message = function() {
          return 'Expected function to throw ' + inspect(expectedError) +
            ', but it threw ' +  inspect(thrown) + '.';
        };
      }

      return result;
    }
  };
};
