--TEST--
Test multiple keyspaces
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

$db->exec ("CREATE KEYSPACE {$keyspace}_int with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");
$db->exec ("CREATE KEYSPACE {$keyspace}_text with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");

$db->exec ("USE {$keyspace}_int");
$db->exec ("CREATE TABLE my_cf (my_key text PRIMARY KEY)
            WITH comparator = int AND default_validation = int;");

$db->exec ("UPDATE my_cf SET 10 = 100, 20 = 200 WHERE my_key = 'aa'");

$db->exec ("USE {$keyspace}_text");
$db->exec ("CREATE TABLE my_cf (my_key text PRIMARY KEY)");
$db->exec ("UPDATE my_cf SET hello = 'world', test = 'column' WHERE my_key = 'aa'");

$db->exec ("USE {$keyspace}_int");
$results = $db->query ("SELECT * FROM my_cf WHERE my_key = 'aa'");
var_dump ($results->fetchAll(PDO::FETCH_ASSOC));

$db->exec ("USE {$keyspace}_text");
$results = $db->query ("SELECT * FROM my_cf WHERE my_key = 'aa'");
var_dump ($results->fetchAll(PDO::FETCH_ASSOC));

$db->exec ("DROP KEYSPACE {$keyspace}_int");
$db->exec ("DROP KEYSPACE {$keyspace}_text");

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    [10]=>
    int(100)
    [20]=>
    int(200)
  }
}
array(1) {
  [0]=>
  array(3) {
    ["my_key"]=>
    string(2) "aa"
    ["hello"]=>
    string(5) "world"
    ["test"]=>
    string(6) "column"
  }
}
OK