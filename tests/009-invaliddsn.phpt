--TEST--
Test invalid dsn
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');
// dsn is never invalid now
/*try {
	$db = new PDO('cassandra:port=123456', $username, $password);
	echo 'fail';
} catch (PDOException $e) {
	echo $e->getMessage () . PHP_EOL;
}

try {
	$db = new PDO('cassandra:port=123456', $username, $password);
	echo 'fail';
} catch (PDOException $e) {
	echo $e->getMessage () . PHP_EOL;
}*/
echo "OK";

?>
--EXPECT--
OK
