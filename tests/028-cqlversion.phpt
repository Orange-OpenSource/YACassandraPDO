--TEST--
Test cql versions
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

// Test default 2.0.0
$db = new PDO($dsn, $username, $password);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace} with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");
$db->exec ("USE {$keyspace};");

$db->exec ("CREATE COLUMNFAMILY int_test (my_key text PRIMARY KEY, my_int bigint)");
$db->exec ("CREATE TABLE comparator_test (my_key text PRIMARY KEY)
            WITH comparator = bigint AND default_validation = ascii;");


// Test 3.0.0

$dsn2 = $dsn . ';cqlversion=3.0.0;dbname='. $keyspace;
$db = new PDO($dsn2, $username, $password);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$db->exec ("CREATE TABLE preserve_test (my_composite_key text, my_second_key text, values int, PRIMARY KEY(my_composite_key, my_second_key))");


echo "OK";
--EXPECT--
OK