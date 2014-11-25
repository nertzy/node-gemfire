const gemfire = require("../gemfire.js");
gemfire.configure("spec/support/scripts/NotPdxReadSerialized.xml");
gemfire.getCache();
