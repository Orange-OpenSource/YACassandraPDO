--TEST--
Test decimal type

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
$db->exec ("CREATE COLUMNFAMILY test (my_key text PRIMARY KEY,
                                          my_decimal decimal,
					)");

$stmt = $db->prepare ("UPDATE test SET my_decimal=:my_decimal WHERE my_key=:key");

// Positive value insertion
$stmt->bindValue (':my_decimal', "2524.1234", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'aa', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "128.123456789", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'bb', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "0", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'cc', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "16.1616161616161616", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'dd', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "1234567890.123456789", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'ee', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "112233445566778899.112233445566778899", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'ff', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-2524.1234", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'naa', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-128.123456789", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'nbb', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-0", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'ncc', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-16.1616161616161616", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'ndd', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-1234567890.123456789", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'nee', PDO::PARAM_STR);
$stmt->execute ();

$stmt->bindValue (':my_decimal', "-712.34", PDO::CASSANDRA_DECIMAL);
$stmt->bindValue (':key', 'ngg', PDO::PARAM_STR);
$stmt->execute ();

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='aa'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='bb'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='cc'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='dd'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='ee'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='ff'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='naa'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='nbb'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='ncc'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='ndd'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='nee'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);

$stmt2 = $db->query ("SELECT my_decimal FROM test WHERE my_key='ngg'");
$res = $stmt2->fetchAll(PDO::FETCH_ASSOC);
print_r($res[0]);


--EXPECT--
Array
(
    [my_decimal] => Array
        (
            [0] => 4
            [1] => 25241234
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 9
            [1] => 128123456789
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 0
            [1] => 0
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 16
            [1] => 161616161616161616
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 9
            [1] => 1234567890123456789
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 18
            [1] => 8234495237290528275
            [2] => 6084187275451764
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 4
            [1] => -25241234
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 9
            [1] => -128123456789
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 0
            [1] => 0
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 16
            [1] => -161616161616161616
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 9
            [1] => -1234567890123456789
        )

)
Array
(
    [my_decimal] => Array
        (
            [0] => 2
            [1] => -71234
        )

)

