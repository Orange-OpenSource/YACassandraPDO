--TEST--
Boolean related tests

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
				    my_bool boolean)");

$stmt = $db->prepare ("INSERT INTO cf(my_key, my_bool)
                       VALUES (:key, :bool)");


$stmt->bindValue (':key', "42", PDO::PARAM_INT);
$stmt->bindValue (':bool', "true", PDO::PARAM_BOOL);
$stmt->execute ();

$stmt->bindValue (':key', "43", PDO::PARAM_INT);
$stmt->bindValue (':bool', "FALse", PDO::PARAM_BOOL);
$stmt->execute ();

$stmt = $db->query ("SELECT my_bool FROM cf WHERE my_key=42");
$data = $stmt->fetchAll ();
var_dump ($data);
$stmt = $db->query ("SELECT my_bool FROM cf WHERE my_key=43");
$data = $stmt->fetchAll ();
var_dump ($data);

--EXPECTF--
array(1) {
  [0]=>
  array(2) {
    ["my_bool"]=>
    bool(true)
    [0]=>
    bool(true)
  }
}
array(1) {
  [0]=>
  array(2) {
    ["my_bool"]=>
    bool(false)
    [0]=>
    bool(false)
  }
}
