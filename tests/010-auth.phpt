--TEST--
Test authentication
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, 'mikko', 'okkim');
echo "OK";

?>
--EXPECT--
OK