--TEST--
Test sparse columns
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$db->exec ("CREATE COLUMNFAMILY extended_users (
			my_key varchar PRIMARY KEY);");

$db->exec ("INSERT INTO extended_users(my_key, secondrowdata, thirdcolumn) VALUES('jane', 'Flat 2, Street 2', 'metadata')");
$db->exec ("INSERT INTO extended_users(my_key, firstrowdata) VALUES('test user', 'Flat 1, Street 1')");
$db->exec ("INSERT INTO extended_users(my_key, third) VALUES('more data', 'aaa')");
$db->exec ("INSERT INTO extended_users(my_key, fourth, large, row) VALUES('xyz', 'large', 'row', 'data')");


$stmt = $db->prepare ("SELECT * FROM extended_users");
$stmt->execute ();
var_dump ($stmt->fetchAll ());			
	
pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(4) {
  [0]=>
  array(8) {
    ["my_key"]=>
    string(9) "test user"
    [0]=>
    string(9) "test user"
    ["firstrowdata"]=>
    string(16) "Flat 1, Street 1"
    [1]=>
    string(16) "Flat 1, Street 1"
    ["__column_not_set_2"]=>
    NULL
    [2]=>
    NULL
    ["__column_not_set_3"]=>
    NULL
    [3]=>
    NULL
  }
  [1]=>
  array(8) {
    ["my_key"]=>
    string(9) "more data"
    [0]=>
    string(9) "more data"
    ["third"]=>
    string(3) "aaa"
    [1]=>
    string(3) "aaa"
    ["__column_not_set_2"]=>
    NULL
    [2]=>
    NULL
    ["__column_not_set_3"]=>
    NULL
    [3]=>
    NULL
  }
  [2]=>
  array(8) {
    ["my_key"]=>
    string(3) "xyz"
    [0]=>
    string(3) "xyz"
    ["fourth"]=>
    string(5) "large"
    [1]=>
    string(5) "large"
    ["large"]=>
    string(3) "row"
    [2]=>
    string(3) "row"
    ["row"]=>
    string(4) "data"
    [3]=>
    string(4) "data"
  }
  [3]=>
  array(8) {
    ["my_key"]=>
    string(4) "jane"
    [0]=>
    string(4) "jane"
    ["secondrowdata"]=>
    string(16) "Flat 2, Street 2"
    [1]=>
    string(16) "Flat 2, Street 2"
    ["thirdcolumn"]=>
    string(8) "metadata"
    [2]=>
    string(8) "metadata"
    ["__column_not_set_3"]=>
    NULL
    [3]=>
    NULL
  }
}
OK