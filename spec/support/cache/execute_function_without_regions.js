const Cache = require("../gemfire.js").Cache;
const cache = new Cache("spec/support/cache/no_regions.xml");
cache.executeFunction("io.pivotal.node_gemfire.TestFunction");
