--TEST--
Test execute prepared statement multiple times
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);

pdo_cassandra_init ($db, $keyspace);

$result = $db->query ("SELECT my_key, full_name FROM users;");
var_dump($result->fetchAll (PDO::FETCH_GROUP));
$result->closeCursor ();

$result->execute ();
var_dump($result->fetchAll (PDO::FETCH_GROUP));
$result->closeCursor ();

pdo_cassandra_done ($db, $keyspace);

echo "OK";

?>
--EXPECT--
array(2) {
  ["mikko"]=>
  array(1) {
    [0]=>
    array(2) {
      ["full_name"]=>
      string(7) "Mikko K"
      [0]=>
      string(7) "Mikko K"
    }
  }
  ["john"]=>
  array(1) {
    [0]=>
    array(2) {
      ["full_name"]=>
      string(8) "John Doe"
      [0]=>
      string(8) "John Doe"
    }
  }
}
array(2) {
  ["mikko"]=>
  array(1) {
    [0]=>
    array(2) {
      ["full_name"]=>
      string(7) "Mikko K"
      [0]=>
      string(7) "Mikko K"
    }
  }
  ["john"]=>
  array(1) {
    [0]=>
    array(2) {
      ["full_name"]=>
      string(8) "John Doe"
      [0]=>
      string(8) "John Doe"
    }
  }
}
OK