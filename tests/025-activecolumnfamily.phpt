--TEST--
Test that active columnfamily is tracked correctly
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$db->exec ("CREATE TABLE my_cf1 (my_key1 text PRIMARY KEY)");
$db->exec ("CREATE TABLE my_cf2 (my_key2 text PRIMARY KEY)");
$db->exec ("CREATE TABLE my_cf3 (my_key3 text PRIMARY KEY)");

$db->exec ("UPDATE my_cf1 SET first_cf = 'first1' WHERE my_key1 = 'aa1'");
$db->exec ("UPDATE my_cf1 SET first_cf = 'first2' WHERE my_key1 = 'aa2'");
$db->exec ("UPDATE my_cf2 SET second_cf = 'second1' WHERE my_key2 = 'aa1'");
$db->exec ("UPDATE my_cf2 SET second_cf = 'second2' WHERE my_key2 = 'aa2'");
$db->exec ("UPDATE my_cf3 SET third_cf = 'third1' WHERE my_key3 = 'aa1'");
$db->exec ("UPDATE my_cf3 SET third_cf = 'third2' WHERE my_key3 = 'aa2'");

$stmt2 = $db->prepare("SELECT * FROM my_cf2");
$stmt2->execute();

$stmt3 = $db->prepare("SELECT * FROM my_cf3");
$stmt3->execute();

$stmt1 = $db->prepare("SELECT * FROM my_cf1");
$stmt1->execute();

var_dump ($stmt2->fetch (PDO::FETCH_ASSOC));
var_dump ($stmt1->fetch (PDO::FETCH_ASSOC));
var_dump ($stmt3->fetch (PDO::FETCH_ASSOC));
var_dump ($stmt3->fetch (PDO::FETCH_ASSOC));
var_dump ($stmt1->fetch (PDO::FETCH_ASSOC));
$db->exec ("SELECT * FROM my_cf1");
var_dump ($stmt2->fetch (PDO::FETCH_ASSOC));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(2) {
  ["my_key2"]=>
  string(3) "aa2"
  ["second_cf"]=>
  string(7) "second2"
}
array(2) {
  ["my_key1"]=>
  string(3) "aa2"
  ["first_cf"]=>
  string(6) "first2"
}
array(2) {
  ["my_key3"]=>
  string(3) "aa2"
  ["third_cf"]=>
  string(6) "third2"
}
array(2) {
  ["my_key3"]=>
  string(3) "aa1"
  ["third_cf"]=>
  string(6) "third1"
}
array(2) {
  ["my_key1"]=>
  string(3) "aa1"
  ["first_cf"]=>
  string(6) "first1"
}
array(2) {
  ["my_key2"]=>
  string(3) "aa1"
  ["second_cf"]=>
  string(7) "second1"
}
OK