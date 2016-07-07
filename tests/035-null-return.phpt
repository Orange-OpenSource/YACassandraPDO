--TEST--
NULL return test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>

--FILE--

<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
    $db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY test_null (id int primary key,
					   my_int int,
					   my_ascii ascii);");

$db->exec ("INSERT INTO test_null(id) values(1);");
$stmt = $db->prepare("SELECT my_int, my_ascii FROM test_null WHERE id=1;");
$stmt->execute();
var_dump($stmt->fetch(PDO::FETCH_ASSOC));


--EXPECT--
array(2) {
  ["my_int"]=>
  NULL
  ["my_ascii"]=>
  NULL
}
