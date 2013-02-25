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
				    set_test_int set<int>)");

/* echo "INSERTING 1 element" . PHP_EOL; */
/* $db->exec("INSERT INTO cf (my_key, set_test_int) */
/* 		VALUES (1, {21});"); */
/* // Display cf content */
/* print_r($db->query('SELECT set_test_int FROM cf WHERE my_key=1;')->fetchObject()); */



/* echo "INSERTING 2 elements" . PHP_EOL; */
/* $db->exec("INSERT INTO cf (my_key, set_test_int) */
/* 		VALUES (2, {21, 42});"); */
/* // Display cf content */
/* print_r($db->query('SELECT set_test_int FROM cf WHERE my_key=2;')->fetchObject()); */



echo "INSERTING 3 elements" . PHP_EOL;
$db->exec("INSERT INTO cf (my_key, set_test_int)
		VALUES (3, {21, 42, 84, 45, 456});");
// Display cf content
print_r($db->query('SELECT set_test_int FROM cf WHERE my_key=3;')->fetchObject());

/* echo "INSERTING 1 string elems (size 2)" . PHP_EOL; */
/* $db->exec("INSERT INTO cf (my_key, set_test_vc) */
/* 		VALUES (4, {'he'});"); */
/* // Display cf content */
/* print_r($db->query('SELECT set_test_vc FROM cf WHERE my_key=4;')->fetchObject()); */


/* echo "INSERTING 2 string elems (size 2 and 5)" . PHP_EOL; */
/* $db->exec("INSERT INTO cf (my_key, set_test_vc) */
/* 		VALUES (4, {'he', 'llowo'});"); */
/* // Display cf content */
/* print_r($db->query('SELECT set_test_vc FROM cf WHERE my_key=4;')->fetchObject()); */


echo "INSERTING 3 string elems (size 2 and 5 and 8)" . PHP_EOL;
$db->exec("INSERT INTO cf (my_key, set_test_vc)
		VALUES (4, {'he', 'llowo', 'madafuck'});");
// Display cf content
print_r($db->query('SELECT set_test_vc FROM cf WHERE my_key=4;')->fetchObject());

// Insertion test
/* $db->exec("INSERT INTO cf (my_key, set_test_vc) */
/* 		VALUES (1, {'key1', 'key2', 'key3'});"); */

/* // Display cf content */
/* print_r($db->query('SELECT set_test_vc FROM cf;')->fetchObject()); */

--EXPECT--