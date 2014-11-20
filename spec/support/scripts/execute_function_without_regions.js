const Cache = require("../gemfire.js").Cache;
const cache = new Cache("spec/support/scripts/no_regions.xml");
cache.executeFunction("io.pivotal.node_gemfire.TestFunction");
