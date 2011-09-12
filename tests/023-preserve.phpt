--TEST--
Test preserving column keys and values
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

$db->exec ("CREATE TABLE test_preserve (my_key text PRIMARY KEY)
            WITH comparator = int AND default_validation = int;");

$db->exec ("UPDATE test_preserve SET 10 = 100, 20 = 200 WHERE my_key = 'aa'");
$stmt = $db->prepare ("SELECT * FROM test_preserve WHERE my_key = 'aa'");

echo "-- preserve_values=no " . PHP_EOL;
$stmt->execute ();
var_dump ($stmt->fetchAll (PDO::FETCH_ASSOC));

echo "-- preserve_values=yes " . PHP_EOL;
$db->setAttribute(PDO::CASSANDRA_ATTR_PRESERVE_VALUES, true);
$stmt->execute ();
var_dump ($stmt->fetchAll (PDO::FETCH_ASSOC));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECTF--
-- preserve_values=no 
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    [10]=>
    int(100)
    [20]=>
    int(200)
  }
}
-- preserve_values=yes 
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    [10]=>
    string(4) "%s"
    [20]=>
    string(4) "%s"
  }
}
OK