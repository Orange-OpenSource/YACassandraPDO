--TEST--
Test UUIDs
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

try {
	$db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$uuid = '5bafc990-ceb7-11e0-bd10-aa2e4924019b';

$db->exec ("CREATE KEYSPACE {$keyspace} with strategy_class = 'SimpleStrategy' and strategy_options:replication_factor=1;");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY uuid_test(
				my_key text PRIMARY KEY)
			WITH comparator = uuid AND default_validation = ascii;");

$stmt = $db->prepare ("UPDATE uuid_test SET :uuid = 10 WHERE my_key = 'uuidtest'");
$stmt->bindValue (':uuid', $uuid, PDO::PARAM_STR);
$stmt->execute ();

$stmt = $db->query ("SELECT * FROM uuid_test");
$data = $stmt->fetchAll ();
$keys = array_keys ($data [0]);
$array = unpack('H*', $keys [2]);

var_dump(current($array) == str_replace ('-', '', $uuid));

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECT--
bool(true)
OK