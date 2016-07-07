--TEST--
UUID tests
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

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'class' : 'SimpleStrategy',
           'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY test_bindings (
                id int PRIMARY KEY,
                my_int int,
                my_float double,
                my_uuid uuid,
                my_decimal decimal,
                my_blob blob,
                my_ascii ascii,
                );");


////////////////////////    UUID formating test      ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_uuid=:my_uuid WHERE id=1");
    $stmt->bindValue(':my_uuid', '5bafc990-ceb7-11e0-bd10-aa2e4924019b', PDO::CASSANDRA_UUID);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 1");
}

try {
    // Uncorrectly formatted
    $stmt = $db->prepare("UPDATE test_bindings SET my_uuid=:my_uuid WHERE id=1");
    $stmt->bindValue(':my_uuid', '5baf--bd10-aa2e4924019b', PDO::CASSANDRA_UUID);
    $stmt->execute();
    print_r("Bind Failure 2");
} catch (Exception $e) {
}

// UUID formating test
try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_uuid=:my_uuid WHERE id=1");
    $stmt->bindValue(':my_uuid', '5bafc990-ceb7-11e0-bd10-aa2e4924019b', PDO::CASSANDRA_UUID);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 1");
}

////////////////////////    Blob formating test      ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_blob=:my_blob WHERE id=1");
    $stmt->bindValue(':my_blob', '0xaabbcc001199AAEE', PDO::CASSANDRA_BLOB);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 3" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_blob=:my_blob WHERE id=1");
    $stmt->bindValue(':my_blob', '0xaabbcc001199AAE', PDO::CASSANDRA_BLOB);
    $stmt->execute();
    print_r("Bind Failure 4" . $e);
} catch (Exception $e) {
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_blob=:my_blob WHERE id=1");
    $stmt->bindValue(':my_blob', '0xaab  bcc001199AAE', PDO::CASSANDRA_BLOB);
    $stmt->execute();
    print_r("Bind Failure 5" . $e);
} catch (Exception $e) {
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_blob=:my_blob WHERE id=1");
    $stmt->bindValue(':my_blob', 'aabbgg', PDO::CASSANDRA_BLOB);
    $stmt->execute();
    print_r("Bind Failure 6" . $e);
} catch (Exception $e) {
}


////////////////////////    int formating test      ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_int=:my_int WHERE id=1");
    $stmt->bindValue(':my_int', '12', PDO::CASSANDRA_INT);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 7" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_int=:my_int WHERE id=1");
    $stmt->bindValue(':my_int', '42', PDO::PARAM_INT);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 8" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_int=:my_int WHERE id=1");
    $stmt->bindValue(':my_int', '12.12', PDO::CASSANDRA_INT);
    $stmt->execute();
    print_r("Bind Failure 9" . $e);
} catch (Exception $e) {
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_int=:my_int WHERE id=1");
    $stmt->bindValue(':my_int', '42 56', PDO::PARAM_INT);
    $stmt->execute();
    print_r("Bind Failure 10" . $e);
} catch (Exception $e) {
}

////////////////////////    str formating test      ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_ascii=:my_ascii WHERE id=1");
    $stmt->bindValue(':my_ascii', '42 56', PDO::PARAM_STR);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 11" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_ascii=:my_ascii WHERE id=1");
    $stmt->bindValue(':my_ascii', '42 56 de la mort qui tue', PDO::CASSANDRA_STR);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 12" . $e);
}

////////////////////////    float formating test      ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_float=:my_float WHERE id=1");
    $stmt->bindValue(':my_float', '42.56', PDO::CASSANDRA_FLOAT);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 13" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_float=:my_float WHERE id=1");
    $stmt->bindValue(':my_float', '42.56E02', PDO::CASSANDRA_FLOAT);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 14" . $e);
}

try {
    $stmt = $db->prepare("UPDATE test_bindings SET my_float=:my_float WHERE id=1");
    $stmt->bindValue(':my_float', '42', PDO::CASSANDRA_FLOAT);
    $stmt->execute();
} catch (Exception $e) {
    print_r("Bind Failure 15" . $e);
}


--EXPECT--
