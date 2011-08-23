--TEST--
Test column metadata
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$stmt = $db->query ("SELECT my_key, full_name FROM users;");
$stmt->fetch ();
var_dump ($stmt->getColumnMeta (1));
var_dump ($stmt->getColumnMeta (3));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(9) {
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["comparator"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["default_validation_class"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["key_validation_class"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["key_alias"]=>
  string(6) "my_key"
  ["name"]=>
  string(9) "full_name"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
  ["pdo_type"]=>
  int(2)
}
bool(false)
OK