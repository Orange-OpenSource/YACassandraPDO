--TEST--
Test column metadata
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$keyspace = 'phptests';

try {
    $db->exec ("DROP KEYSPACE {$keyspace}");
} catch (PDOException $e) {}

$uuid = '5bafc990-ceb7-11e0-bd10-aa2e4924019b';

$db->exec ("CREATE KEYSPACE {$keyspace} WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY types_test(
                my_key text PRIMARY KEY,
                my_blob blob,
                my_ascii ascii,
                my_text text,
                my_varchar varchar,
                my_uuid uuid,
                my_int int,
                my_varint varint,
                my_bigint bigint)");



$stmt = $db->prepare ("INSERT INTO types_test(my_key, my_blob, my_ascii, my_text, my_varchar, my_uuid, my_int, my_varint, my_bigint)
                                    VALUES   (:key,   :blob,   :ascii,   :text,   :varchar,   :uuid,   :int,   :varint,   :bigint)");


$stmt->bindValue (':key', "hello key");
$stmt->bindValue (':blob', "0x74686520616e73776572206973203432", PDO::CASSANDRA_BLOB);
$stmt->bindValue (':ascii', "hello ascii");
$stmt->bindValue (':text', "what else than lorem ipsum? well, ∆∆∆");
$stmt->bindValue (':varchar', "what else than more lorem ipsum?");
$stmt->bindValue (':uuid', '5bafc990-ceb7-11e0-bd10-aa2e4924019b', PDO::CASSANDRA_UUID);
$stmt->bindValue (':int', -5555, PDO::PARAM_INT);
$stmt->bindValue (':varint', 123, PDO::PARAM_INT);
$stmt->bindValue (':bigint', 891011, PDO::PARAM_INT);
$stmt->execute ();

$stmt = $db->query ("SELECT my_key, my_blob, my_ascii, my_text, my_varchar, my_uuid, my_varint, my_int, my_bigint FROM types_test");
$data = $stmt->fetchAll ();

for ($i = 0; $i < 9; $i++)
{
    var_dump ($stmt->getColumnMeta ($i));
}

pdo_cassandra_done ($db, $keyspace);

echo "OK";
--EXPECTF--
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["name"]=>
  string(6) "my_key"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(41) "org.apache.cassandra.db.marshal.BytesType"
  ["name"]=>
  string(7) "my_blob"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(41) "org.apache.cassandra.db.marshal.AsciiType"
  ["name"]=>
  string(8) "my_ascii"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["name"]=>
  string(7) "my_text"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.UTF8Type"
  ["name"]=>
  string(10) "my_varchar"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.UUIDType"
  ["name"]=>
  string(7) "my_uuid"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(43) "org.apache.cassandra.db.marshal.IntegerType"
  ["name"]=>
  string(9) "my_varint"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(41) "org.apache.cassandra.db.marshal.Int32Type"
  ["name"]=>
  string(6) "my_int"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
array(6) {
  ["keyspace"]=>
  string(8) "phptests"
  ["columnfamily"]=>
  string(10) "types_test"
  ["native_type"]=>
  string(40) "org.apache.cassandra.db.marshal.LongType"
  ["name"]=>
  string(9) "my_bigint"
  ["len"]=>
  int(-1)
  ["precision"]=>
  int(0)
}
OK
