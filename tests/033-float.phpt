--TEST--
FLOAT implementation test
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

$db->exec ("CREATE KEYSPACE {$keyspace}  WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor': 1}");
$db->exec ("USE {$keyspace}");
$db->exec ("CREATE COLUMNFAMILY cf (my_key int PRIMARY KEY,
				    my_float float,
				    my_double double)");

function insert_in_base($db, $key, $value, $type = 0) {
    $stmt = '';
    if ($type == 0) {
	$stmt = $db->prepare ("UPDATE cf SET my_float=:my_float WHERE my_key=:my_key;");
	$stmt->bindValue (':my_float', $value, PDO::PARAM_INT);
    }
    else {
	$stmt = $db->prepare ("UPDATE cf SET my_double=:my_double WHERE my_key=:my_key;");
	$stmt->bindValue (':my_double', $value, PDO::PARAM_INT);
    }
    $stmt->bindValue (':my_key', $key, PDO::PARAM_INT);
    $stmt->execute();
}

function dump_value($db, $key, $type = 0) {
    $stmt = $db->prepare("SELECT * FROM cf WHERE my_key=:my_key");
    $stmt->bindValue (':my_key', $key, PDO::PARAM_INT);
    $stmt->execute();
    $res = $stmt->fetchAll();
    if ($type == 0)
	var_dump($res[0]['my_float']);
    else
	var_dump($res[0]['my_double']);
}

// Float insertions
insert_in_base($db, 21, 42.42);
insert_in_base($db, 22, -42.42);
insert_in_base($db, 23, -4242);
insert_in_base($db, 24, +4242);
insert_in_base($db, 25, -4.39518E+7);
insert_in_base($db, 26, 4.39518E+7);

dump_value($db, 21);
dump_value($db, 22);
dump_value($db, 23);
dump_value($db, 24);
dump_value($db, 25);
dump_value($db, 26);


// Double insertions
insert_in_base($db, 26, 4.395181234567E+73, 1);
insert_in_base($db, 27, -4.395181234567E+73, 1);

dump_value($db, 26, 1);
dump_value($db, 27, 1);

--EXPECT--
float(42.419998168945)
float(-42.419998168945)
float(-4242)
float(4242)
float(-43951800)
float(43951800)
float(4.395181234567E+73)
float(-4.395181234567E+73)
