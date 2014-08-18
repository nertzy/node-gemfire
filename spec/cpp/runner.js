#!/usr/bin/env node
var testPath = "../../build/Release/test.node";
var status = require(testPath).run();

process.exit(status);
