--TEST--
Test sparse columns
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$db->exec ("CREATE COLUMNFAMILY extended_users (
			my_key varchar PRIMARY KEY);");


$db->exec ("INSERT INTO extended_users(my_key, firstrowdata) VALUES('test user', 'Flat 1, Street 1')");
$db->exec ("INSERT INTO extended_users(my_key, secondrowdata) VALUES('jane', 'Flat 2, Street 2')");

$stmt = $db->prepare ("SELECT * FROM extended_users");
$stmt->execute ();
var_dump ($stmt->fetchAll ());			

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(2) {
  [0]=>
  array(4) {
    ["my_key"]=>
    string(9) "test user"
    [0]=>
    string(9) "test user"
    ["firstrowdata"]=>
    string(16) "Flat 1, Street 1"
    [1]=>
    string(16) "Flat 1, Street 1"
  }
  [1]=>
  array(4) {
    ["my_key"]=>
    string(4) "jane"
    [0]=>
    string(4) "jane"
    ["firstrowdata"]=>
    string(16) "Flat 2, Street 2"
    [1]=>
    string(16) "Flat 2, Street 2"
  }
}
OK