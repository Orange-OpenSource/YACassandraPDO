--TEST--
Test startup keyspace (dbname=...)
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

$dsn2 = $dsn . ';dbname=' . $keyspace;

$db->exec ("USE " . $keyspace . ";");
$db->exec ("CREATE COLUMNFAMILY int_test (my_key text PRIMARY KEY, my_int bigint)");

$stmt = $db->prepare ("UPDATE int_test SET my_int=:test WHERE my_key=:key");
$stmt->bindValue (':test', 12345, PDO::CASSANDRA_INT);
$stmt->bindValue (':key', 'aa', PDO::CASSANDRA_STR);
$stmt->execute();

$stmt->bindValue (':test', -54321, PDO::PARAM_INT);
$stmt->bindValue (':key', 'bb', PDO::PARAM_STR);
$stmt->execute ();

$stmt = $db->prepare("SELECT my_int FROM int_test WHERE my_key = 'aa'");
$stmt->execute();
var_dump($stmt->fetchAll());

$stmt = $db->prepare("SELECT my_int FROM int_test WHERE my_key = 'bb'");
$stmt->execute();
var_dump($stmt->fetchAll());

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(2) {
    ["my_int"]=>
    int(12345)
    [0]=>
    int(12345)
  }
}
array(1) {
  [0]=>
  array(2) {
    ["my_int"]=>
    int(-54321)
    [0]=>
    int(-54321)
  }
}
OK
