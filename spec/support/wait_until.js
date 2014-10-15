module.exports = function waitUntil(test, next) {
  if(test()) {
    next();
  } else {
    setImmediate(function(){
      waitUntil(test, next);
    });
  }
};
