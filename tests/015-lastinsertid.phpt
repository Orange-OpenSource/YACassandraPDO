--TEST--
Test last insert id
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

try {
	$db->lastInsertId ();
	echo 'fail';
} catch (PDOException $e) {
	echo 'success' . PHP_EOL;
}

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
success
OK