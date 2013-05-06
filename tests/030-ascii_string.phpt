--TEST--
Test special characters in ascii strings
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>

--FILE--
<?php
require_once(dirname(__FILE__) . '/config.inc');

$db = new PDO($dsn, $username, $password);
pdo_cassandra_init($db, $keyspace);

function display($db, $field) {
    $stmt = $db->prepare ("SELECT * from users where my_key=:key;");
    $stmt->execute(array (':key' => 'ascii_key'));
    $rows = $stmt->fetchAll();
    var_dump($rows[0][$field]);
};

function inject($db, $keys) {
    $stmt = $db->prepare ("UPDATE users SET field_ascii=:field_ascii WHERE my_key=:key ");
    $stmt->execute($keys);
};

// Regular test
$chains  = array('four',
                 'I am an ascii chain',
                 '^^ \\',
                 'Good\'Morning\'\'England!!');


foreach($chains as $chain) {
    inject($db, array (':key' => 'ascii_key',
		       ':field_ascii' => $chain));
    display($db, 'field_ascii');
}
pdo_cassandra_done($db, $keyspace);
?>

--EXPECT--
string(4) "four"
string(19) "I am an ascii chain"
string(4) "^^ \"
string(23) "Good'Morning''England!!"