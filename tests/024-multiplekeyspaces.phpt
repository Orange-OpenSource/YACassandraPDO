--TEST--
Test multiple keyspaces
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);

pdo_cassandra_init($db);

try {
    $db->exec ("DROP KEYSPACE {$keyspace}_int");
    $db->exec ("DROP KEYSPACE {$keyspace}_text");
} catch (Exception $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace}_int WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("CREATE KEYSPACE {$keyspace}_text WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");

$db->exec ("USE {$keyspace}_int");
$db->exec ("CREATE TABLE my_cf (my_key text PRIMARY KEY, my_int1 int, my_int2 int)");


$db->exec ("UPDATE my_cf SET my_int1 = 100, my_int2 = 200 WHERE my_key = 'aa'");

$db->exec ("USE {$keyspace}_text");
$db->exec ("CREATE TABLE my_cf (my_key text PRIMARY KEY, my_msg1 text, my_msg2 text)");
$db->exec ("UPDATE my_cf SET my_msg1 = 'world', my_msg2 = 'column' WHERE my_key = 'aa'");

$db->exec ("USE {$keyspace}_int");
$results1 = $db->query ("SELECT * FROM my_cf WHERE my_key = 'aa'");
var_dump ($results1->fetchAll(PDO::FETCH_ASSOC));

$db->exec ("USE {$keyspace}_text");
$results2 = $db->query ("SELECT * FROM my_cf WHERE my_key = 'aa'");
var_dump ($results2->fetchAll(PDO::FETCH_ASSOC));

$db->exec ("DROP KEYSPACE {$keyspace}_int");
$db->exec ("DROP KEYSPACE {$keyspace}_text");

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    ["my_int1"]=>
    int(100)
    ["my_int2"]=>
    int(200)
  }
}
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    ["my_msg1"]=>
    string(5) "world"
    ["my_msg2"]=>
    string(6) "column"
  }
}
OK
