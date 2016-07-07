--TEST--
Test null returns on select queries

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
$db->exec ("CREATE COLUMNFAMILY cf (my_key int PRIMARY KEY,
				    my_ts timestamp,
				    my_int int,
				    my_int2 int,
				    my_ascii ascii)");

$stmt = $db->prepare ("UPDATE cf SET my_int=:my_int WHERE my_key=:my_key;");
$stmt->bindValue (':my_int', 42, PDO::PARAM_INT);
$stmt->bindValue (':my_key', 21, PDO::PARAM_INT);
$stmt->execute();

$stmt = $db->prepare("SELECT * FROM cf WHERE my_key=:my_key");
$stmt->bindValue (':my_key', 21, PDO::PARAM_INT);
$stmt->execute();

var_dump($stmt->fetchAll());

--EXPECT--
array(1) {
  [0]=>
  array(10) {
    ["my_key"]=>
    int(21)
    [0]=>
    int(21)
    ["my_ascii"]=>
    NULL
    [1]=>
    NULL
    ["my_int"]=>
    int(42)
    [2]=>
    int(42)
    ["my_int2"]=>
    NULL
    [3]=>
    NULL
    ["my_ts"]=>
    NULL
    [4]=>
    NULL
  }
}
