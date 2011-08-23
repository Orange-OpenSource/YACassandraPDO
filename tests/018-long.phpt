--TEST--
Test different data types
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

$db->exec ("CREATE COLUMNFAMILY StandardLongA (KEY text PRIMARY KEY)
			WITH comparator = bigint AND default_validation = ascii;");
			
$stmt = $db->prepare ("
						BEGIN BATCH USING CONSISTENCY ONE
					     UPDATE StandardLongA SET 1='1', 2='2', 3='3', 4='4' WHERE KEY='aa'
					     UPDATE StandardLongA SET 5='5', 6='6', 7='8', 9='9' WHERE KEY='ab'
					     UPDATE StandardLongA SET 9='9', 8='8', 7='7', 6='6' WHERE KEY='ac'
					     UPDATE StandardLongA SET 5='5', 4='4', 3='3', 2='2' WHERE KEY='ad'
					     UPDATE StandardLongA SET 1='1', 2='2', 3='3', 4='4' WHERE KEY='ae'
					     UPDATE StandardLongA SET 1='1', 2='2', 3='3', 4='4' WHERE KEY='af'
					     UPDATE StandardLongA SET 5='5', 6='6', 7='8', 9='9' WHERE KEY='ag'
					    APPLY BATCH
					");

$stmt->execute ();

$stmt = $db->prepare ("SELECT * FROM StandardLongA");
$stmt->execute ();
$datas = $stmt->fetchAll ();

foreach ($datas as $row)
{
	var_dump ($row [0]);
}

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
string(2) "af"
string(2) "ab"
string(2) "ac"
string(2) "aa"
string(2) "ae"
string(2) "ag"
string(2) "ad"
OK