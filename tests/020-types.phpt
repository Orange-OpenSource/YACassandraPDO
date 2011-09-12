--TEST--
Test different data types
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
$db->exec ("CREATE COLUMNFAMILY types_test(
				my_key text PRIMARY KEY,
				my_blob 'blob',
				my_ascii ascii,
				my_text text,
				my_varchar varchar,
				my_uuid uuid,
				my_varint varint,
				my_int int,
				my_bigint bigint)");


$stmt = $db->prepare ("INSERT INTO types_test(my_key, my_blob, my_ascii, my_text, my_varchar, my_uuid, my_varint, my_int, my_bigint)
									VALUES   (:key,   :blob,   :ascii,   :text,   :varchar,   :uuid,   :varint,   :int,   :bigint)");

$stmt->bindValue (':key', "hello key");
$stmt->bindValue (':blob', "74686520616e73776572206973203432");
$stmt->bindValue (':ascii', "hello ascii");
$stmt->bindValue (':text', "what else than lorem ipsum? well, ∆∆∆");
$stmt->bindValue (':varchar', "what else than more lorem ipsum?");
$stmt->bindValue (':uuid', '5bafc990-ceb7-11e0-bd10-aa2e4924019b');
$stmt->bindValue (':varint', 123, PDO::PARAM_INT);
$stmt->bindValue (':int', 4567, PDO::PARAM_INT);
$stmt->bindValue (':bigint', 891011, PDO::PARAM_INT);
$stmt->execute ();

$stmt = $db->query ("SELECT my_key, my_blob, my_ascii, my_text, my_varchar, my_uuid, my_varint, my_int, my_bigint FROM types_test");
$data = $stmt->fetchAll ();

$array = unpack('H*', $data [0]['my_uuid']);

var_dump ($data, $array);

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECTF--
array(1) {
  [0]=>
  array(18) {
    ["my_key"]=>
    string(9) "hello key"
    [0]=>
    string(9) "hello key"
    ["my_blob"]=>
    string(16) "the answer is 42"
    [1]=>
    string(16) "the answer is 42"
    ["my_ascii"]=>
    string(11) "hello ascii"
    [2]=>
    string(11) "hello ascii"
    ["my_text"]=>
    string(43) "what else than lorem ipsum? well, ∆∆∆"
    [3]=>
    string(43) "what else than lorem ipsum? well, ∆∆∆"
    ["my_varchar"]=>
    string(32) "what else than more lorem ipsum?"
    [4]=>
    string(32) "what else than more lorem ipsum?"
    ["my_uuid"]=>
    string(16) "%s"
    [5]=>
    string(16) "%s"
    ["my_varint"]=>
    int(123)
    [6]=>
    int(123)
    ["my_int"]=>
    int(4567)
    [7]=>
    int(4567)
    ["my_bigint"]=>
    int(891011)
    [8]=>
    int(891011)
  }
}
array(1) {
  [1]=>
  string(32) "5bafc990ceb711e0bd10aa2e4924019b"
}
OK