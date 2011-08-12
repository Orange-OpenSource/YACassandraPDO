--TEST--
Test iterating prepared statement
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

pdo_cassandra_init ($db, $keyspace);

$result = $db->query ("SELECT my_key, full_name FROM users;");

foreach ($result as $row) {
	var_dump ($row);
}

foreach ($result as $row) {
	var_dump ($row);
}

pdo_cassandra_done ($db, $keyspace);

echo "OK";

--EXPECT--
array(4) {
  ["my_key"]=>
  string(5) "mikko"
  [0]=>
  string(5) "mikko"
  ["full_name"]=>
  string(7) "Mikko K"
  [1]=>
  string(7) "Mikko K"
}
array(4) {
  ["my_key"]=>
  string(4) "john"
  [0]=>
  string(4) "john"
  ["full_name"]=>
  string(8) "John Doe"
  [1]=>
  string(8) "John Doe"
}
array(4) {
  ["my_key"]=>
  string(5) "mikko"
  [0]=>
  string(5) "mikko"
  ["full_name"]=>
  string(7) "Mikko K"
  [1]=>
  string(7) "Mikko K"
}
array(4) {
  ["my_key"]=>
  string(4) "john"
  [0]=>
  string(4) "john"
  ["full_name"]=>
  string(8) "John Doe"
  [1]=>
  string(8) "John Doe"
}
OK