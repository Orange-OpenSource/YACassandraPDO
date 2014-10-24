--TEST--
Test consistency level set in db handle
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

require_once(dirname(__FILE__) . '/config.inc');

// pass consistency param in constructor
$params = array_merge($params, array(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL => PDO::CASSANDRA_CONSISTENCYLEVEL_ONE));
$db = new PDO($dsn, $username, $password, $params);

pdo_cassandra_init ($db, $keyspace);

// set consistency as an attribute
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_ONE);

// set consistency in the prepare statement 
$stmt = $db->prepare("SELECT full_name FROM users WHERE my_key='john'", $params);
$stmt->execute();

// test consistency types
// ONE
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_ONE);
// TWO
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_TWO);
// THREE
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_THREE);
// ANY
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_ANY);
// ALL
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_ALL);
// QUORUM
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_QUORUM);
// LOCAL_QUORUM
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_LOCAL_QUORUM);
// EACH_QUORUM
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_EACH_QUORUM);
// LOCAL_ONE
$db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, PDO::CASSANDRA_CONSISTENCYLEVEL_LOCAL_ONE);

// INVALID CONSISTENCY
try {
    $db->setAttribute(PDO::CASSANDRA_ATTR_CONSISTENCYLEVEL, 9999);
} catch (Exception $e) {
    echo $e->getMessage() . PHP_EOL;
}

pdo_cassandra_done ($db, $keyspace);

echo "OK";

?>
--EXPECT--
CQLSTATE[HY000] [0] Invalid consistency level value.
OK
