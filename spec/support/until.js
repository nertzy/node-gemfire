const async = require("async");
const _ = require("lodash");

module.exports = function until(fn, test, done) {
  var args;

  async.doUntil(
    function(next) {
      _.delay(function(){
        fn(function(){
          args = arguments;
          next();
        });
      }, 50);
    },
    function() {
      return test.apply(this, args);
    },
    done
  );
};
