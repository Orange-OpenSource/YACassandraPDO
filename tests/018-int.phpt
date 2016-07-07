--TEST--
Test integers
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
$db->exec ("CREATE COLUMNFAMILY int_test (my_key text PRIMARY KEY,
					  my_int bigint,
					  my_reg_int int
					)");

$stmt = $db->prepare ("UPDATE int_test SET my_int = :test, my_reg_int=:my_reg_int WHERE my_key = :key");
// Positive value insertion
$stmt->bindValue (':test', 12345, PDO::PARAM_INT);
$stmt->bindValue (':my_reg_int', 12345, PDO::PARAM_INT);
$stmt->bindValue (':key', 'aa', PDO::PARAM_STR);
$stmt->execute ();

// Negative value insertion
$stmt->bindValue (':test', -54321, PDO::PARAM_INT);
$stmt->bindValue (':my_reg_int', -54321, PDO::PARAM_INT);
$stmt->bindValue (':key', 'bb', PDO::PARAM_STR);
$stmt->execute ();

// INT MAX value
$stmt->bindValue (':test', PHP_INT_MAX, PDO::PARAM_INT);
$stmt->bindValue (':my_reg_int', -12345, PDO::PARAM_INT);
$stmt->bindValue (':key', 'cc', PDO::PARAM_STR);
$stmt->execute ();


$stmt = $db->query ("SELECT my_int, my_reg_int FROM int_test WHERE my_key = 'aa'");
var_dump ($stmt->fetchAll ());

$stmt = $db->query ("SELECT my_int, my_reg_int FROM int_test WHERE my_key = 'bb'");
var_dump ($stmt->fetchAll ());

$stmt = $db->query ("SELECT my_int, my_reg_int FROM int_test WHERE my_key = 'cc'");
var_dump ($stmt->fetchAll ());

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(4) {
    ["my_int"]=>
    int(12345)
    [0]=>
    int(12345)
    ["my_reg_int"]=>
    int(12345)
    [1]=>
    int(12345)
  }
}
array(1) {
  [0]=>
  array(4) {
    ["my_int"]=>
    int(-54321)
    [0]=>
    int(-54321)
    ["my_reg_int"]=>
    int(-54321)
    [1]=>
    int(-54321)
  }
}
array(1) {
  [0]=>
  array(4) {
    ["my_int"]=>
    int(9223372036854775807)
    [0]=>
    int(9223372036854775807)
    ["my_reg_int"]=>
    int(-12345)
    [1]=>
    int(-12345)
  }
}
OK