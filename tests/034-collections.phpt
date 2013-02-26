--TEST--
Test collection types
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
$db->exec ("CREATE COLUMNFAMILY cf (my_key int PRIMARY KEY,
				    set_test_vc set<varchar>,
				    set_test_int set<int>,
				    map_test_vc_int map<varchar, int>,
				    map_test_int_float map<int, float>)");

// Map varchar int test
$db->exec("INSERT INTO cf (my_key, map_test_vc_int)
		VALUES (400, {'key1': 1, 'key2': 2, 'key3': 3});");
print_r($db->query('SELECT map_test_vc_int FROM cf WHERE my_key=400;')->fetchObject());


// Map int float test
$db->exec("INSERT INTO cf (my_key, map_test_int_float)
		VALUES (400, {12: 24.24, 15: 30.30, 32: 64.64});");
print_r($db->query('SELECT map_test_int_float FROM cf WHERE my_key=400;')->fetchObject());


// Set varchar test
$db->exec("INSERT INTO cf (my_key, set_test_vc)
		VALUES (8, {'key1', 'llowo', 'madafuck', 'yukulele'});");
print_r($db->query('SELECT set_test_vc FROM cf WHERE my_key=8;')->fetchObject());


// Set int test
$db->exec("INSERT INTO cf (my_key, set_test_int)
		VALUES (900, {21, 42, 84, 45, 456});");
print_r($db->query('SELECT set_test_int FROM cf WHERE my_key=900;')->fetchObject());


--EXPECT--
stdClass Object
(
    [map_test_vc_int] => Array
        (
            [key1] => 1
            [key2] => 2
            [key3] => 3
        )

)
stdClass Object
(
    [map_test_int_float] => Array
        (
            [12] => 24.239999771118
            [15] => 30.299999237061
            [32] => 64.639999389648
        )

)
stdClass Object
(
    [set_test_vc] => Array
        (
            [0] => key1
            [1] => llowo
            [2] => madafuck
            [3] => yukulele
        )

)
stdClass Object
(
    [set_test_int] => Array
        (
            [0] => 21
            [1] => 42
            [2] => 45
            [3] => 84
            [4] => 456
        )

)
