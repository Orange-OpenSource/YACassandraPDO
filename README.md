# Introduction

PDO driver for Cassandra CQL.

This repository is a fork of: https://code.google.com/a/apache-extras.org/p/cassandra-pdo/
We cloned it on GitHub because the original project seemed to be dead.

This version is developped for the CQL3 target only. We do not provide any support for former versions of CQL.

# Cassandra versions support

The driver runs well with cassandra 1.2.x
We plan to support Cassandra 2.0 versions in the next few months.
The support of Cassandra 2.0 is experimental. Checkout the project to the 2.0_experimental branch to enhance compatibility with 2.0 versions

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

# Create a debian package from the sources

    ln -s packaging/debian debian
    dpkg-buildpackage -rfakeroot -us -uc

# How to use this driver?

[Visit our wiki page](https://github.com/Orange-OpenSource/YACassandraPDO/wiki)

# Contributing

Pull requests containing fixes and/or additional tests are highly appreciated.
Documentation is also far from being complete, feel free to report the difficulties you face.
