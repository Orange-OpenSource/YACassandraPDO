--TEST--
Test timestamp type
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}"); 
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY timestamp_test (my_key text PRIMARY KEY, my_timestamp timestamp)");

$stmt = $db->prepare ("UPDATE timestamp_test SET my_timestamp = :test WHERE my_key = :key");
$stmt->bindValue (':test', 1342453581000, PDO::PARAM_INT);
$stmt->bindValue (':key', 'aa', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':test', '2012-07-16 17:45:03+0200', PDO::PARAM_STR);
$stmt->bindValue (':key', 'bb', PDO::PARAM_STR);
$stmt->execute ();

$stmt = $db->query ("SELECT my_timestamp FROM timestamp_test WHERE my_key = 'aa'");
var_dump ($stmt->fetchAll ());

$stmt = $db->query ("SELECT my_timestamp FROM timestamp_test WHERE my_key = 'bb'");
var_dump ($stmt->fetchAll ());

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(2) {
    ["my_timestamp"]=>
    int(1342453581000)
    [0]=>
    int(1342453581000)
  }
}
array(1) {
  [0]=>
  array(2) {
    ["my_timestamp"]=>
    int(1342453503000)
    [0]=>
    int(1342453503000)
  }
}
OK