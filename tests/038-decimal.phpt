--TEST--
Test decimal type

--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>

--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
    $db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'CLASS' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY test (my_key text PRIMARY KEY,
                                          my_decimal decimal,
					)");

$stmt = $db->prepare ("UPDATE test SET my_decimal=:my_decimal WHERE my_key=:key");
// Positive value insertion
$stmt->bindValue (':my_decimal', "252154689.000", PDO::PARAM_INT);
$stmt->bindValue (':key', 'aa', PDO::PARAM_STR);
$stmt->execute ();

$stmt = $db->query ("SELECT my_decimal FROM test WHERE my_key='aa'");
var_dump ($stmt->fetchAll ());

--EXPECT--
