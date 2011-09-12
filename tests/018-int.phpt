--TEST--
Test integers
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace} with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY int_test (my_key text PRIMARY KEY, my_int bigint)");

$stmt = $db->prepare ("UPDATE int_test SET my_int = :test WHERE my_key = :key");
$stmt->bindValue (':test', 12345, PDO::PARAM_INT);
$stmt->bindValue (':key', 'aa', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':test', -54321, PDO::PARAM_INT);
$stmt->bindValue (':key', 'bb', PDO::PARAM_STR);
$stmt->execute ();

$stmt = $db->query ("SELECT my_int FROM int_test WHERE my_key = 'aa'");
var_dump ($stmt->fetchAll ());

$stmt = $db->query ("SELECT my_int FROM int_test WHERE my_key = 'bb'");
var_dump ($stmt->fetchAll ());

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