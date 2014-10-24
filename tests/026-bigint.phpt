--TEST--
Test very large integers
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

if (!extension_loaded ('gmp'))
    die ('skip The test requires gmp extension');

require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password, $params);

pdo_cassandra_init($db, $keyspace);

$db->exec ("CREATE COLUMNFAMILY verylargeint_test (my_key text PRIMARY KEY, testval varint)");

$db->exec ("UPDATE verylargeint_test SET testval = 50000000000000000000000 WHERE my_key = 'aa'");
$stmt = $db->query ("SELECT testval FROM verylargeint_test WHERE my_key = 'aa'");

$row = $stmt->fetch (PDO::FETCH_ASSOC);

$g = gmp_init(bin2hex($row['testval']), 16);
echo gmp_strval ($g) . PHP_EOL;

echo "OK";
--EXPECT--
50000000000000000000000
OK
