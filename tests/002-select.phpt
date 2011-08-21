--TEST--
Test prepared statement emulation
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$stmt = $db->prepare ("SELECT my_key, full_name FROM users WHERE my_key = :key;");
$stmt->bindValue (':key', 'mikko');
$stmt->execute ();

var_dump ($stmt->fetchAll ());

pdo_cassandra_done ($db, $keyspace);

echo "OK";

--EXPECT--
array(1) {
  [0]=>
  array(4) {
    ["my_key"]=>
    string(5) "mikko"
    [0]=>
    string(5) "mikko"
    ["full_name"]=>
    string(7) "Mikko K"
    [1]=>
    string(7) "Mikko K"
  }
}
OK