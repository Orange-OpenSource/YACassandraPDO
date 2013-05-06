--TEST--
UUID tests
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
$db->exec ("CREATE COLUMNFAMILY test_uuid (
                id int PRIMARY KEY,
                my_uuid uuid);");


$stmt = $db->prepare("UPDATE test_uuid SET my_uuid=:my_uuid WHERE
                id=1");

$uuid = '5bafc990-ceb7-11e0-bd10-aa2e4924019b';
// UUID must be bound as an int value
$stmt->bindValue('my_uuid', $uuid, PDO::PARAM_INT);
$stmt->execute();

$stmt = $db->prepare("SELECT my_uuid FROM test_uuid WHERE id=1;");
$stmt->execute();
$res = $stmt->fetch(PDO::FETCH_ASSOC);

$uuid_ret = unpack('H*', $res['my_uuid']);
print_r($uuid_ret);


--EXPECT--
Array
(
    [1] => 5bafc990ceb711e0bd10aa2e4924019b
)