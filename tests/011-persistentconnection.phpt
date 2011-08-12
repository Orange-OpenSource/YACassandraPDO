--TEST--
Test initialization of persistent connections
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, null, null, array (
    			PDO::ATTR_PERSISTENT => true
			));

echo "OK";

$db = null;

$db1 = new PDO($dsn, null, null, array (
    			PDO::ATTR_PERSISTENT => true
			));

?>
--EXPECT--
OK