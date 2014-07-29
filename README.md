pivotal-gemfire
===============

Proof-of-concept node module connecting to Pivotal Gemfire

## Prerequisites 

* node 0.10.x http://nodejs.org/
* vagrant 1.6.x http://www.vagrantup.com/

## Setup
Build out the virtualbox and install nodejs, Gemfire, etc.

    $ vagrant up

## Developer workflow

This directory is mounted on the VM as `/project`. You can make edits here or on the VM. Then to run the tests, SSH into the VM and run grunt.

    $ vagrant ssh
    $ cd /project
    $ grunt

## License

See LICENSE file for details
