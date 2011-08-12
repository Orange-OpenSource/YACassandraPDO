--TEST--
Test setting/getting attributes
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');
$db = new PDO($dsn);

var_dump ($db->getAttribute (PDO::ATTR_SERVER_VERSION));
var_dump ($db->setAttribute (PDO::CASSANDRA_ATTR_NUM_RETRIES, 10));
var_dump ($db->setAttribute (PDO::CASSANDRA_ATTR_NO_DELAY, true));

echo "OK";
--EXPECTF--
string(7) "%d.%d.%d"
bool(true)
bool(true)
OK