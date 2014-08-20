const gemfire = require("../../index.js");
const cache = new gemfire.Cache("benchmark/xml/BenchmarkClient.xml");

exports.getCache = function getCache(){
  return cache;
};
