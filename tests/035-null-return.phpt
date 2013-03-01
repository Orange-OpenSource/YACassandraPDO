--TEST--
NULL return test
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>

--FILE--

<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password);
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
    $db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'CLASS' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY test_null (id int primary key,
					   my_int int);");

$db->exec ("INSERT INTO test_null(id) values(1);");
$stmt = $db->prepare("SELECT my_int FROM test_null WHERE id=1;");
$stmt->execute();
var_dump($stmt->fetch(PDO::FETCH_ASSOC));


--EXPECT--