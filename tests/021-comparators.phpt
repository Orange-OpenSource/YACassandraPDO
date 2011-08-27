--TEST--
Test integer keys
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace} with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE TABLE comparator_test (my_key text PRIMARY KEY)
            WITH comparator = bigint AND default_validation = ascii;");

$stmt = $db->prepare ("UPDATE comparator_test SET 10 = 'hello', 20 = 'hi there' WHERE my_key = 'aa'");
$stmt->execute ();

$stmt = $db->prepare ("UPDATE comparator_test SET 30 = 'more data', 40 = 'more more', 50 = 'more' WHERE my_key = 'bb'");
$stmt->execute ();

$stmt = $db->query ("SELECT * FROM comparator_test");
var_dump ($stmt->fetchAll (PDO::FETCH_ASSOC));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(2) {
  [0]=>
  array(6) {
    ["my_key"]=>
    string(2) "bb"
    [30]=>
    string(9) "more data"
    [40]=>
    string(9) "more more"
    [50]=>
    string(4) "more"
    [10]=>
    NULL
    [20]=>
    NULL
  }
  [1]=>
  array(6) {
    ["my_key"]=>
    string(2) "aa"
    [30]=>
    NULL
    [40]=>
    NULL
    [50]=>
    NULL
    [10]=>
    string(5) "hello"
    [20]=>
    string(8) "hi there"
  }
}
OK