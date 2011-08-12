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
var_dump ($stmt->getColumnMeta (0));
var_dump ($stmt->getColumnMeta (3));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(5) {
  ["native_type"]=>
  string(6) "string"
  ["name"]=>
  string(6) "my_key"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
  ["pdo_type"]=>
  int(2)
}
bool(false)
OK