--TEST--
Test quoting values
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);
var_dump ($db->quote ("'hello' 'world'"));
var_dump ($db->quote ("Co'mpl''ex \"st'\"ring"));
var_dump ($db->quote ("'''''''''", PDO::PARAM_LOB));
var_dump ($db->quote ("test " . chr(0) . " value"));
var_dump ($db->quote ("test \\"));

echo "OK";
?>
--EXPECT--
string(21) "'''hello'' ''world'''"
string(26) "'Co''mpl''''ex "st''"ring'"
string(20) "''''''''''''''''''''"
string(7) "'test '"
string(8) "'test \'"
OK
