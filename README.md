Node GemFire
====================

NodeJS client for Pivotal GemFire

## Supported platforms

* CentOS 6.5
* Other 64-bit Linux platforms may work.

## Installation

### Prerequisites

1. Download and install the GemFire 8.0.0 Native Client for your platform from [Pivotal Network](https://network.pivotal.io/products/pivotal-gemfire).
2. Set the environment variables described by the [GemFire Native Client installation instructions](http://gemfire.docs.pivotal.io/latest/userguide/index.html#gemfire_nativeclient/introduction/install-overview.html) for your platform.

### Installing the NPM package

Note that for the time being, if you want to be able to use the precompiled binary, you'll need to set `NODE_TLS_REJECT_UNAUTHORIZED=0` when running `npm install`. Otherwise, `npm install` will fallback to compiling from source, which may only work on certain platforms.

```
$ cd /my/node/project
$ NODE_TLS_REJECT_UNAUTHORIZED=0 npm install --save pivotal/node-gemfire
```

## Usage examples:

For an example cache.xml, see [here](https://github.com/pivotal/node-gemfire/blob/master/benchmark/xml/BenchmarkClient.xml).

```javascript
var gemfire = require('gemfire');

//
//  Cache
//
var cache = new gemfire.Cache('config/cache.xml');

//  executeQuery
cache.executeQuery("SELECT DISTINCT * FROM /exampleRegion", function(error, results){
  console.log("executeQuery: ", results.toArray());

  results.each(function(result) {
    console.log(result);
  });
});

//
// Region
//
var region = cache.getRegion('myRegion');

// put & get
region.put('foo', { bar: ['baz', 'qux'] }, function(error) { 
  region.get('foo', function(error, value) {
    console.log(value); // => { bar: ['baz', 'qux'] }
  });
});

//  putAll & getAll
region.putAll({ key1: 'value1', key2: 'value2' }, function(error) {
  region.getAll(['key1', 'key2'], function(error, response) {
    console.log(response); // => { key1: 'value1', key2: 'value2' }
  });
});

//  putAll & query
region.putAll({ key1: 'value1', key2: 'value2' }, function(error) {
  region.query("this like 'value%'", function(error, response) {
    console.log(response.toArray()); // => [ 'value1', 'value2' ]
  });
});

//  putAll & existsValue
region.putAll({ key1: 'value1', key2: 'value2' }, function(error) {

  // exists
  region.existsValue("this = 'value1'", function(error, response) {
    console.log(response); // => true
  });

  // does not exist
  region.existsValue("this = 'something that does not exist'", function(error, response) {
    console.log(response); // => false
  });
});

//  executeFunction
region.executeFunction("com.example.MyJavaFunction", ["some", "arguments"], function(error, results){
  console.log("executeFunction MyJavaFunction: ", results);
});

//  clear
region.clear();

```

## Development
### Prerequisites 

* [Vagrant 1.6.x or later](http://www.vagrantup.com/)

### Setup

To build the VM:

    $ vagrant up

### Re-provision VM to install missing dependencies

After pulling down updated code, you may need to re-provision your VM to get any new dependencies.

    $ vagrant provision

### Developer workflow

This directory is mounted on the VM as `/project`. You can make edits here or on the VM.

    $ vagrant ssh
    $ cd /project

### Rebuild the node module and run Jasmine tests

    $ vagrant ssh
    $ cd /project
    $ grunt

### Run every benchmark

    $ vagrant ssh
    $ cd /project
    $ grunt benchmark

### Run benchmarks individually

    $ vagrant ssh
    $ cd /project
    $ grunt benchmark:node
    $ grunt benchmark:node:async
    $ grunt benchmark:java

### Server Management

The GemFire server should be automatically started for you as part of the above tasks. If you
need to restart it manually, use the following:

    $ vagrant ssh
    $ grunt server:restart # or server:start or server:stop

## License

See LICENSE file for details
