const gemfire = require("./gemfire.js");
const cache = new gemfire.Cache("xml/ExampleClient.xml");

exports.getCache = function getCache(){
  return cache;
};
