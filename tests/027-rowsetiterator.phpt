--TEST--
Test sparse columns with rowset iterator
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$db->exec ("CREATE COLUMNFAMILY extended_users (
			my_key varchar PRIMARY KEY);");

$db->exec ("INSERT INTO extended_users(my_key, third) VALUES('two columns', 'aaa')");
$db->exec ("INSERT INTO extended_users(my_key, secondrowdata, thirdcolumn) VALUES('three columns', 'Flat 2, Street 2', 'metadata')");
$db->exec ("INSERT INTO extended_users(my_key, fourth, large, row) VALUES('four columns', 'large', 'row', 'data')");

$stmt = $db->prepare ("SELECT * FROM extended_users");
$stmt->setAttribute(PDO::CASSANDRA_ATTR_ROWSET_ITERATOR, true);
$stmt->execute ();

do {
	$row = $stmt->fetch(PDO::FETCH_ASSOC);
	var_dump ($row);
} while ($stmt->nextRowSet ());

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
array(2) {
  ["my_key"]=>
  string(11) "two columns"
  ["third"]=>
  string(3) "aaa"
}
array(4) {
  ["my_key"]=>
  string(12) "four columns"
  ["fourth"]=>
  string(5) "large"
  ["large"]=>
  string(3) "row"
  ["row"]=>
  string(4) "data"
}
array(3) {
  ["my_key"]=>
  string(13) "three columns"
  ["secondrowdata"]=>
  string(16) "Flat 2, Street 2"
  ["thirdcolumn"]=>
  string(8) "metadata"
}
OK