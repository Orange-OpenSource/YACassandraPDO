--TEST--
Test quoting values
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);
var_dump ($db->quote ("'hello' 'world'"));
var_dump ($db->quote ("Co'mpl''ex \"st'\"ring"));
var_dump ($db->quote ("'''''''''", PDO::PARAM_LOB));
var_dump ($db->quote ("test " . chr(0) . " value"));


echo "OK";
?>
--EXPECT--
string(21) "'\'hello\' \'world\''"
string(28) "'Co\'mpl\'\'ex \"st\'\"ring'"
string(20) "'\'\'\'\'\'\'\'\'\''"
string(15) "'test \0 value'"
OK