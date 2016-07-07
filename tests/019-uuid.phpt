--TEST--
Test UUIDs
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

$uuid = '5bafc990-ceb7-11e0-bd10-aa2e4924019b';

$db->exec ("CREATE KEYSPACE {$keyspace} WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");

$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY uuid_test(
                my_key text PRIMARY KEY,
                uuid uuid)");

$stmt = $db->prepare ("UPDATE uuid_test SET uuid=:uuid WHERE my_key='uuidtest'");
$stmt->bindValue (':uuid', $uuid, PDO::CASSANDRA_UUID);
$stmt->execute ();

$stmt = $db->query ("SELECT * FROM uuid_test WHERE my_key='uuidtest'");
$data = $stmt->fetchAll ();

var_dump($data[0]['uuid'] == $uuid);

pdo_cassandra_done ($db, $keyspace);
echo 'OK';

--EXPECT--
bool(true)
OK
