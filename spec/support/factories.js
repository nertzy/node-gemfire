const gemfire = require("./gemfire.js");
gemfire.configure("xml/ExampleClient.xml");
exports.getCache = gemfire.getCache;
