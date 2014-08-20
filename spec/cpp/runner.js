#!/usr/bin/env node
var testPath = "../../build/Debug/test.node";
var status = require(testPath).run();

process.exit(status);
