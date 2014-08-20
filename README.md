GemFire NodeJS addon
====================

Proof-of-concept NodeJS addon for connecting to Pivotal GemFire

## Supported platforms

* CentOS 6.5
* Other 64-bit Linux platforms probably work.

## Installation

### From Precompiled Binary

This is currently unsupported, but we're working on it.

### From Source

1. Clone this repo to a local directory.
2. Download and install the GemFire 7.0.2 Native Client for your platform from [Pivotal Network](https://network.pivotal.io/products/pivotal-gemfire).
3. Set the environment variables described by the [GemFire Native Client installation instructions](http://gemfire.docs.pivotal.io/latest/userguide/index.html#gemfire_nativeclient/introduction/install-overview.html) for your platform.
4. Install our package into your NodeJS project: `cd /my/node/project && npm install --save-dev /path/to/our/repo`

## Usage example

```javascript
var gemfire = require('gemfire');
var cache = new gemfire.Cache('config/cache.xml');
var region = cache.getRegion('myRegion');

region.put('foo', { bar: ['baz', 'qux'] }, function(error, value) { console.log("put: ", value); });

region.get('foo', function(error, value) { console.log("get: ", value); });

cache.executeQuery("SELECT DISTINCT * FROM /exampleRegion", function(error, results){
  console.log("executeQuery: ", results);
});

region.clear();
```

## Development
### Prerequisites 

* node 0.10.x http://nodejs.org/
* vagrant 1.6.x http://www.vagrantup.com/

### Setup

Build out the virtualbox and install nodejs, GemFire, etc.

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
