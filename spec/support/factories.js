const gemfire = require("./gemfire.js");
const cache = new gemfire.Cache("benchmark/xml/BenchmarkClient.xml");

exports.getCache = function getCache(){
  return cache;
};
