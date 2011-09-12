--TEST--
Test counters
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$stmt = $db->query ("CREATE TABLE test_counter (my_key text PRIMARY KEY, count_me counter)
					  WITH comparator = ascii AND default_validation = counter;");
$data = $stmt->fetchAll (PDO::FETCH_ASSOC);

$db->query("UPDATE test_counter SET count_me = count_me + 2 WHERE my_key = 'counter1'");

$stmt = $db->query ("SELECT * FROM test_counter");
var_dump ($stmt->fetchAll (PDO::FETCH_ASSOC));

$db->query("UPDATE test_counter SET count_me = count_me + 2 WHERE my_key = 'counter1'");

$stmt = $db->query ("SELECT * FROM test_counter");
var_dump ($stmt->fetchAll (PDO::FETCH_ASSOC));

$db->query ("DROP TABLE test_counter");

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(1) {
  [0]=>
  array(2) {
    ["my_key"]=>
    string(8) "counter1"
    ["count_me"]=>
    int(2)
  }
}
array(1) {
  [0]=>
  array(2) {
    ["my_key"]=>
    string(8) "counter1"
    ["count_me"]=>
    int(4)
  }
}
OK