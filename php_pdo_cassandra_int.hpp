/*
 *  Copyright 2011 DataStax
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.                 
 */

#ifndef _PHP_PDO_CASSANDRA_PRIVATE_H_
# define _PHP_PDO_CASSANDRA_PRIVATE_H_

extern "C" {
/* Need to undefine these so that thrift config doesn't complain */
#ifdef HAVE_CONFIG_H
# include "config.h"
# undef PACKAGE_NAME
# undef PACKAGE_STRING
# undef PACKAGE_TARNAME
# undef PACKAGE_VERSION
#endif

#include "ext/standard/info.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"
#include "main/php_ini.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"
#include "ext/standard/url.h"
#include "ext/standard/php_string.h"
#include "ext/pcre/php_pcre.h"
#include "ext/standard/php_math.h"
}

#define HAVE_ZLIB_CP HAVE_ZLIB
#undef HAVE_ZLIB

#include "gen-cpp/Cassandra.h"
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocketPool.h>
#include <transport/TTransportUtils.h>

#undef HAVE_ZLIB
#define HAVE_ZLIB HAVE_ZLIB_CP

#include <boost/bimap.hpp>
#include <iostream>
#include <string>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace org::apache::cassandra;

enum pdo_cassandra_type {
	PDO_CASSANDRA_TYPE_BYTES,
	PDO_CASSANDRA_TYPE_ASCII,
	PDO_CASSANDRA_TYPE_UTF8,
	PDO_CASSANDRA_TYPE_INTEGER,
	PDO_CASSANDRA_TYPE_LONG,
	PDO_CASSANDRA_TYPE_UUID,
	PDO_CASSANDRA_TYPE_LEXICAL,
	PDO_CASSANDRA_TYPE_TIMEUUID,
	PDO_CASSANDRA_TYPE_UNKNOWN
};

/** {{{
*/
typedef struct {
	const char *file;
	char *errmsg;
	int line;
	unsigned int errcode;
} pdo_cassandra_einfo;
/* }}} */

/* {{{ typedef struct php_cassandra_db_handle 
*/
typedef struct {
	zend_object zo;
	zend_bool compression;
	boost::shared_ptr<TSocketPool> socket;
	boost::shared_ptr<TFramedTransport> transport;
	boost::shared_ptr<TProtocol> protocol;
	boost::shared_ptr<CassandraClient> client;
	pdo_cassandra_einfo einfo;
	std::string active_keyspace;
	std::string active_columnfamily;
	KsDef description;
	zend_bool has_description;
	zend_bool preserve_values;
} pdo_cassandra_db_handle;
/* }}} */

typedef boost::bimap<std::string, int> ColumnMap;

/* {{{ typedef struct pdo_cassandra_stmt 
*/
typedef struct {
	pdo_cassandra_db_handle *H;
	zend_bool has_iterator;
	boost::shared_ptr<CqlResult> result;
	std::vector<CqlRow>::iterator it;

	ColumnMap original_column_names;
	ColumnMap column_name_labels;
} pdo_cassandra_stmt;
/* }}} */

/* {{{ enum pdo_cassandra_constant
*/
enum pdo_cassandra_constant {
	PDO_CASSANDRA_ATTR_MIN = PDO_ATTR_DRIVER_SPECIFIC,
	PDO_CASSANDRA_ATTR_NUM_RETRIES,
	PDO_CASSANDRA_ATTR_RETRY_INTERVAL,
	PDO_CASSANDRA_ATTR_MAX_CONSECUTIVE_FAILURES,
	PDO_CASSANDRA_ATTR_RANDOMIZE,
	PDO_CASSANDRA_ATTR_ALWAYS_TRY_LAST,
	PDO_CASSANDRA_ATTR_LINGER,
	PDO_CASSANDRA_ATTR_NO_DELAY,
	PDO_CASSANDRA_ATTR_CONN_TIMEOUT,
	PDO_CASSANDRA_ATTR_RECV_TIMEOUT,
	PDO_CASSANDRA_ATTR_SEND_TIMEOUT,
	PDO_CASSANDRA_ATTR_COMPRESSION,
	PDO_CASSANDRA_ATTR_THRIFT_DEBUG,
	PDO_CASSANDRA_ATTR_PRESERVE_VALUES,
	PDO_CASSANDRA_ATTR_MAX
};
/* }}} */

enum pdo_cassandra_error {
	PDO_CASSANDRA_GENERAL_ERROR,
	PDO_CASSANDRA_NOT_FOUND,
	PDO_CASSANDRA_INVALID_REQUEST,
	PDO_CASSANDRA_UNAVAILABLE,
	PDO_CASSANDRA_TIMED_OUT,
	PDO_CASSANDRA_AUTHENTICATION_ERROR,
	PDO_CASSANDRA_AUTHORIZATION_ERROR,
	PDO_CASSANDRA_SCHEMA_DISAGREEMENT,
	PDO_CASSANDRA_TRANSPORT_ERROR,
	PDO_CASSANDRA_INVALID_CONNECTION_STRING,
	PDO_CASSANDRA_INTEGER_CONVERSION_ERROR
};

void pdo_cassandra_error_ex(pdo_dbh_t *dbh TSRMLS_DC, pdo_cassandra_error code, const char *file, int line, zend_bool force_exception, const char *message, ...);
#define pdo_cassandra_error(dbh, code, message, ...) pdo_cassandra_error_ex(dbh TSRMLS_CC, code, __FILE__, __LINE__, 0, message, __VA_ARGS__)
#define pdo_cassandra_error_exception(dbh, code, message, ...) pdo_cassandra_error_ex(dbh TSRMLS_CC, code, __FILE__, __LINE__, 1, message, __VA_ARGS__)

void pdo_cassandra_set_active_keyspace(pdo_cassandra_db_handle *H, const std::string &sql TSRMLS_DC);
void pdo_cassandra_set_active_columnfamily(pdo_cassandra_db_handle *H, const std::string &query TSRMLS_DC);
std::string pdo_cassandra_get_first_sub_pattern(const std::string &subject, const std::string &pattern TSRMLS_DC);

#endif /* _PHP_PDO_CASSANDRA_PRIVATE_H_ */
