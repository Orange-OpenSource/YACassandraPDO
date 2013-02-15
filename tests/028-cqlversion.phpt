--TEST--
Test cql versions
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

// Test default value
$db = new PDO($dsn, $username, $password);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace} WITH REPLICATION = {'CLASS' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace};");

$db->exec ("CREATE COLUMNFAMILY int_test (my_key text PRIMARY KEY, my_int bigint)");
$db->exec ("CREATE TABLE comparator_test (my_key text PRIMARY KEY)");



// Test 3.0.0 (supported version)
$dsn2 = str_replace('3.0.0','2.0.0',$dsn) . ';dbname='. $keyspace;
try {
    $db = new PDO($dsn2, $username, $password);
} catch (Exception $e) {
    echo $e->getMessage() . PHP_EOL;
}

echo "OK";
--EXPECT--
CQLSTATE[HY000] [9] Invalid connection string attribute
OK