Node GemFire
====================

NodeJS client for Pivotal GemFire

## Supported platforms

* CentOS 6.5
* Other 64-bit Linux platforms may work.

## Installation

### Prerequisites

1. Download and install the GemFire 8.1.0.0 Native Client for your platform from [Pivotal Network](https://network.pivotal.io/products/pivotal-gemfire).
2. Set the environment variables described by the [GemFire Native Client installation instructions](http://gemfire.docs.pivotal.io/latest/userguide/index.html#gemfire_nativeclient/introduction/install-overview.html) for your platform.

### Installing the NPM package

Note that for the time being, if you want to be able to use the precompiled binary, you'll need to set `NODE_TLS_REJECT_UNAUTHORIZED=0` when running `npm install`. Otherwise, `npm install` will fallback to compiling from source, which may only work on certain platforms.

```
$ cd /my/node/vagrant
$ NODE_TLS_REJECT_UNAUTHORIZED=0 npm install --save pivotal/node-gemfire
```

### Configuring the GemFire client

It is possible to configure the GemFire C++ Native Client compiled into this node module via a file called `gfcpp.properties`. Place this file in the current working directory of your node application.

Here is an example file that turns off statistics collection, sets the "warning" log level, and redirects the log output to a file.

```
statistic-sampling-enabled=false
log-level=warning
log-file=log/gemfire.log
```

You can see the available options for `gfcpp.properties` in the [GemFire documentation](http://gemfire.docs.pivotal.io/latest/userguide/gemfire_nativeclient/setting-properties/attributes-gfcpp.html).

## Usage

```javascript
var gemfire = require('gemfire');
gemfire.configure('config/cache.xml');

var cache = gemfire.getCache();
var region = cache.getRegion('myRegion');

region.put('foo', { bar: ['baz', 'qux'] }, function(error) { 
  region.get('foo', function(error, value) {
    console.log(value); // => { bar: ['baz', 'qux'] }
  });
});
```

For more information, please see the full [API documentation](doc/api.md).

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

This directory is mounted on the VM as `/vagrant`. You can make edits here or on the VM.

    $ vagrant ssh
    $ cd /vagrant

### Rebuild the node module and run Jasmine tests

    $ vagrant ssh
    $ cd /vagrant
    $ grunt

### Server Management

The GemFire server should be automatically started for you as part of the above tasks. If you
need to restart it manually, use the following:

    $ vagrant ssh
    $ grunt server:restart # or server:start or server:stop

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for information on how to submit a pull request.

## License

BSD, see [LICENSE](LICENSE) for details

For dependency licenses, see [doc/dependencies.md](doc/dependencies.md)
