# Introduction

PDO driver implementation for Cassandra CQL.

This repository is a fork of: https://code.google.com/a/apache-extras.org/p/cassandra-pdo/
We cloned it on GitHub because the original project seemed to be dead.

This version is developped for the CQL3 target only. We do not provide any support for former versions of CQL.

# Cassandra versions support

NEW: The driver is compatible with versions of Cassandra up to 2.0.x

The support of Cassandra 2.0 is very new. So if you experience bugs, incorrect behavior or anything suspect, don't hesitate to fill a bug report.

# What is different from the Datastax version?
 - Added support for float numbers, decimals
 - Added support for collections (map, set, list)
 - Fixed a lot of bugs on integer convertions
 - Fixed parameters binding
 - Some other minor fixes
 - Timeout in requests support

# Dependencies

  - PHP and PDO
  - Thrift
  - Boost (shared_ptr)

# How to build?

First install thrift from http://thrift.apache.org/download/. Thrift should depend on
boost shared_ptr so while installing thrift you are installing rest of the dependencies
for pdo_cassandra (apart from PHP and PDO of course).

pdo_cassandra ./configure script takes the following options:

 - --with-pdo-cassandra[=FILE] where file is optional path to Cassandra Thrift definitions file. The file is
    bundled with the package so it is unlikely that alternative file is needed unless testing new versions.

 - --with-thrift-dir[=DIR] can be used to specify 'non-standard' thrift installation prefix.

 - --with-boost-dir[=DIR] can be used to specify 'non-standard' boost installation prefix.

# Running tests

After a successful build tests can be executed using an instance of Cassandra. Default config
for Cassandra should be fine.

Before make test:

    $ cp tests/config.inc-dist tests/config.inc
    $ $EDITOR tests/config.inc

This is to prevent accidentally dropping keyspaces that might in use.

For running tests with SSL configuration, use $params in config.inc for setting certificate path, private/public key etc.

# Create a debian package from the sources

    add "deb http://debian.datastax.com/community stable main" in your apt sources list
    apt-get install libthrift0 libthrift-dev thrift-compiler

    ln -s packaging/debian debian
    dpkg-buildpackage -rfakeroot -us -uc

# How to use this driver?

[Visit our wiki page](https://github.com/Orange-OpenSource/YACassandraPDO/wiki)

# Contributing

Pull requests containing fixes and/or additional tests are highly appreciated.
Documentation is also far from being complete, feel free to report the difficulties you face.
