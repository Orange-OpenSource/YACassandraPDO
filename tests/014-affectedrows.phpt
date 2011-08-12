--TEST--
Test number of affected rows
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

var_dump ($db->exec ("UPDATE users SET full_name = 'K okkiM' WHERE my_key = 'mikko'"));

$stmt = $db->query ("SELECT my_key, full_name FROM users");
var_dump ($stmt->fetchAll ());

var_dump ($db->exec ("DELETE FROM users WHERE my_key = 'mikko'"));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
int(0)
array(2) {
  [0]=>
  array(4) {
    ["my_key"]=>
    string(5) "mikko"
    [0]=>
    string(5) "mikko"
    ["full_name"]=>
    string(7) "K okkiM"
    [1]=>
    string(7) "K okkiM"
  }
  [1]=>
  array(4) {
    ["my_key"]=>
    string(4) "john"
    [0]=>
    string(4) "john"
    ["full_name"]=>
    string(8) "John Doe"
    [1]=>
    string(8) "John Doe"
  }
}
int(0)
OK