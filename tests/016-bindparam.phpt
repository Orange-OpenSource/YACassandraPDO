--TEST--
Test binding params
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$full_name = "test 1";

$stmt = $db->prepare ("UPDATE users SET full_name = :value WHERE my_key = 'mikko'");
$stmt->bindParam (":value", $full_name, PDO::PARAM_STR);
$stmt->execute ();

$full_name = "test 2";
$stmt->execute ();

$full_name = "test 3";
$stmt->execute ();

$stmt = $db->query ("SELECT my_key, full_name FROM users");
var_dump ($stmt->fetchAll ());

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(2) {
  [0]=>
  array(4) {
    ["my_key"]=>
    string(5) "mikko"
    [0]=>
    string(5) "mikko"
    ["full_name"]=>
    string(6) "test 3"
    [1]=>
    string(6) "test 3"
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
OK