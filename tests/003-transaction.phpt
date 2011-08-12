--TEST--
Test transaction
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

try {
	$db->beginTransaction();
} catch (PDOException $e) {
	echo $e->getMessage () . PHP_EOL;
}

echo "OK";

--EXPECT--
This driver doesn't support transactions
OK