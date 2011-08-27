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

$db->exec ("INSERT INTO extended_users(my_key, third) VALUES('two columns', 'aaa')");
$db->exec ("INSERT INTO extended_users(my_key, secondrowdata, thirdcolumn) VALUES('three columns', 'Flat 2, Street 2', 'metadata')");
$db->exec ("INSERT INTO extended_users(my_key, fourth, large, row) VALUES('four columns', 'large', 'row', 'data')");

$stmt = $db->prepare ("SELECT * FROM extended_users");

$stmt->execute ();
var_dump ($stmt->fetchAll(PDO::FETCH_ASSOC));

$stmt = $db->prepare ("SELECT * FROM extended_users WHERE my_key = :key");
$stmt->bindValue (':key', 'two columns');
$stmt->execute ();
var_dump ($stmt->fetchAll(PDO::FETCH_ASSOC));

$stmt->bindValue (':key', 'four columns');
$stmt->execute ();
var_dump ($stmt->fetchAll(PDO::FETCH_ASSOC));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(3) {
  [0]=>
  array(7) {
    ["my_key"]=>
    string(11) "two columns"
    ["third"]=>
    string(3) "aaa"
    ["fourth"]=>
    NULL
    ["large"]=>
    NULL
    ["row"]=>
    NULL
    ["secondrowdata"]=>
    NULL
    ["thirdcolumn"]=>
    NULL
  }
  [1]=>
  array(7) {
    ["my_key"]=>
    string(12) "four columns"
    ["third"]=>
    NULL
    ["fourth"]=>
    string(5) "large"
    ["large"]=>
    string(3) "row"
    ["row"]=>
    string(4) "data"
    ["secondrowdata"]=>
    NULL
    ["thirdcolumn"]=>
    NULL
  }
  [2]=>
  array(7) {
    ["my_key"]=>
    string(13) "three columns"
    ["third"]=>
    NULL
    ["fourth"]=>
    NULL
    ["large"]=>
    NULL
    ["row"]=>
    NULL
    ["secondrowdata"]=>
    string(16) "Flat 2, Street 2"
    ["thirdcolumn"]=>
    string(8) "metadata"
  }
}
array(1) {
  [0]=>
  array(2) {
    ["my_key"]=>
    string(11) "two columns"
    ["third"]=>
    string(3) "aaa"
  }
}
array(1) {
  [0]=>
  array(4) {
    ["my_key"]=>
    string(12) "four columns"
    ["fourth"]=>
    string(5) "large"
    ["large"]=>
    string(3) "row"
    ["row"]=>
    string(4) "data"
  }
}
OK