--TEST--
Test invalid dsn
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
try {
	$db = new PDO('cassandra:asasd:AsdA:DSasd;A:sdasd');
	echo 'fail';
} catch (PDOException $e) {
	echo $e->getMessage () . PHP_EOL;
}

try {
	$db = new PDO('cassandra:');
	echo 'fail';
} catch (PDOException $e) {
	echo $e->getMessage () . PHP_EOL;
}
echo "OK";

?>
--EXPECT--
CQLSTATE[HY000] [9] Invalid connection string attribute
CQLSTATE[HY000] [9] Invalid connection string attribute
OK
