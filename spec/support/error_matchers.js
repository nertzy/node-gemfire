const inspect = require('util').inspect;

// For now, we don't care if it's actually an instance of Error.
function isError(error) {
  return !!error;
}

function isErrorWithMessage(error, expectedMessage) {
  if (!isError(error)) {
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
    compare: function compare(actual, expectedMessage) {
      const isNot = this.isNot;

      if(isNot && expectedMessage) {
        throw("not.toBeError() matcher received unexpected argument");
      }

      this.message = function message() {
        if(isNot) {
          return "Expected " + inspect(actual) + " not to be an error";
        }

        if(expectedMessage) {
          if(expectedMessage.constructor === RegExp) {
            return "Expected " + inspect(actual) + " to be an error with message matching " + inspect(expectedMessage);
          } else {
            return "Expected " + inspect(actual) + " to be an error with message " + inspect(expectedMessage);
          }
        } else {
          return "Expected " + inspect(actual) + " to be an error";
        }
      };

      var result = {};

      if(expectedMessage) {
        result.pass = isErrorWithMessage(actual, expectedMessage);
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
