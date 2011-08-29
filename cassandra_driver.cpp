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

#include "php_pdo_cassandra.hpp"
#include "php_pdo_cassandra_int.hpp"

/* Defined in pdo_cassandra_statement.cpp */
extern struct pdo_stmt_methods cassandra_stmt_methods;

/* {{{ pdo_cassandra_functions[] */
const zend_function_entry pdo_cassandra_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* Declarations */
static int  pdo_cassandra_handle_close(pdo_dbh_t *dbh TSRMLS_DC);
static int  pdo_cassandra_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC);
static int  pdo_cassandra_handle_prepare(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC);
static int  pdo_cassandra_handle_quote(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype  TSRMLS_DC);
static long pdo_cassandra_handle_execute(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC);
static int  pdo_cassandra_handle_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC);
static int  pdo_cassandra_handle_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC);
static int  pdo_cassandra_get_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC);

static struct pdo_dbh_methods cassandra_methods = {
	pdo_cassandra_handle_close,
	pdo_cassandra_handle_prepare,
	pdo_cassandra_handle_execute,
	pdo_cassandra_handle_quote,
	NULL,
	NULL,
	NULL,
	pdo_cassandra_handle_set_attribute,
	NULL,
	pdo_cassandra_get_error,
	pdo_cassandra_handle_get_attribute,
	NULL,
	NULL,
	NULL
};

/** {{{ static int pdo_cassandra_get_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC)
*/
static int pdo_cassandra_get_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast<pdo_cassandra_db_handle *> (dbh->driver_data);
	pdo_cassandra_einfo *einfo = &H->einfo;

	if (einfo->errmsg) {
		add_next_index_long(info, einfo->errcode);
		add_next_index_string(info, einfo->errmsg, 1);
	}
	return 1;
}
/* }}} */

/** {{{ static void pdo_cassandra_set_cqlstate(pdo_error_type buffer, pdo_cassandra_error code)
*/
static void pdo_cassandra_set_cqlstate(pdo_error_type buffer, pdo_cassandra_error code)
{
	switch (code) {
		case PDO_CASSANDRA_TRANSPORT_ERROR:
			strlcpy(buffer, "08006", sizeof(pdo_error_type));
		break;

		case PDO_CASSANDRA_INTEGER_CONVERSION_ERROR:
			strlcpy(buffer, "22003", sizeof(pdo_error_type));
		break;

		default:
			strlcpy(buffer, "HY000", sizeof(pdo_error_type));
		break;
	}
}
/* }}} */

/** {{{ void pdo_cassandra_error_ex(pdo_dbh_t *dbh TSRMLS_DC, pdo_cassandra_error code, const char *file, int line, const char *message, ...)
*/
void pdo_cassandra_error_ex(pdo_dbh_t *dbh TSRMLS_DC, pdo_cassandra_error code, const char *file, int line, zend_bool force_exception, const char *message, ...)
{
	pdo_cassandra_db_handle *H = static_cast<pdo_cassandra_db_handle *> (dbh->driver_data);
	pdo_cassandra_einfo *einfo = &H->einfo;

	va_list args;
	char buffer[256];
	size_t buffer_len;

	if (einfo->errmsg) {
		/* Free previous error message */
		pefree(einfo->errmsg, dbh->is_persistent);
		einfo->errmsg = NULL;
	}

	einfo->errcode = code;
	einfo->file    = file;
	einfo->line    = line;

	/* Set the correct CQLSTATE */
	pdo_cassandra_set_cqlstate(dbh->error_code, code);

	va_start(args, message);
	buffer_len = vsnprintf(buffer, sizeof(buffer), message, args);
	va_end(args);

	einfo->errmsg = (char *) pemalloc(buffer_len + 1, dbh->is_persistent);
	memcpy(einfo->errmsg, buffer, buffer_len + 1);

	if (dbh->error_mode == PDO_ERRMODE_EXCEPTION || force_exception) {
		zend_throw_exception_ex(php_pdo_get_exception(), code TSRMLS_CC, "CQLSTATE[%s] [%d] %s", dbh->error_code, code, buffer);
	} else if (dbh->error_mode == PDO_ERRMODE_WARNING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "CQLSTATE[%s] [%d] %s", dbh->error_code, code, buffer);
	}
}
/* }}} */

/** {{{ static zend_bool parse_dsn(pdo_dbh_t *dbh, pdo_cassandra_db_handle *H, const char *dsn TSRMLS_DC)
*/
static zend_bool parse_dsn(pdo_dbh_t *dbh, pdo_cassandra_db_handle *H, const char *dsn TSRMLS_DC)
{
	char *ptr = NULL, *pch = NULL, *last = NULL;

	ptr = estrdup(dsn);
	pch = php_strtok_r(ptr, ",", &last);

	while (pch != NULL) {
		char *host = NULL;
		int port = 0;

		struct pdo_data_src_parser vars[] = {
			{ "host",	"",	0 },
			{ "port",	"",	0 },
		};

		if (php_pdo_parse_data_source(pch, strlen(pch), vars, 2) != 2) {
			efree(ptr);
			return 0;
		}

		host = vars[0].optval;
		port = atoi(vars[1].optval);
#if 0
		std::cerr << "Adding host=[" << host << "] port=[" << port << "]" << std::endl;
#endif
		try {
			H->socket->addServer (host, port);
		} catch (std::exception &re) {
			efree(ptr);
			return 0;
		}

		pch = php_strtok_r(NULL, ",", &last);

		for (size_t i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
			if (vars[i].freeme) {
				efree(vars[i].optval);
			}
		}
	}
	efree(ptr);
	return 1;
}
/* }}} */

/* {{{ static void php_cassandra_thrift_debug_output (const char *s) 
*/
static void php_cassandra_thrift_debug_output (const char *s) 
{
	TSRMLS_FETCH ();
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "PDO Cassandra thrift debug: %s", s);
}
/* }}} */

/* {{{ static void php_cassandra_thrift_no_output (const char *) 
*/
static void php_cassandra_thrift_no_output (const char *) 
{}
/* }}} */

/** {{{ static void pdo_cassandra_toggle_thrift_debug(zend_bool enabled)
*/
static void pdo_cassandra_toggle_thrift_debug(zend_bool enabled)
{
	if (enabled) {
		// Convert thift messages to php warnings
		GlobalOutput.setOutputFunction(&php_cassandra_thrift_debug_output);
	} else {
		// Disable output from thrift library
		GlobalOutput.setOutputFunction(&php_cassandra_thrift_no_output);
	}
}
/* }}} */

/** {{{ static void php_cassandra_handle_auth(pdo_dbh_t *dbh, pdo_cassandra_db_handle *H)
	TODO: this needs to be actually tested with a setup where logins work
*/
static void php_cassandra_handle_auth(pdo_dbh_t *dbh, pdo_cassandra_db_handle *H)
{
	if (dbh->username && strlen(dbh->username) && dbh->password && strlen(dbh->password)) {
		std::string user = dbh->username;
		std::string pass = dbh->password;

		std::map<std::string, std::string> auth_value;
		auth_value.insert(std::pair<std::string, std::string>(user, pass));

		AuthenticationRequest auth_request;
		auth_request.credentials = auth_value;
		H->client->login(auth_request);
	}
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC)
*/
static int pdo_cassandra_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = new pdo_cassandra_db_handle;

	dbh->driver_data   = NULL;
	dbh->methods       = &cassandra_methods;
	H->compression     = 0;
	H->einfo.errcode   = 0;
	H->einfo.errmsg    = NULL;
	H->has_description = 0;
	H->preserve_values = 0;

	H->socket.reset(new TSocketPool);
	H->transport.reset(new TFramedTransport(H->socket));
	H->protocol.reset(new TBinaryProtocol(H->transport));
	H->client.reset(new CassandraClient(H->protocol));
	dbh->driver_data = H;

	/* set possible connection timeout */
	long timeout = 0;

	if (driver_options) {
		timeout = pdo_attr_lval(driver_options, PDO_ATTR_TIMEOUT, timeout TSRMLS_CC);
		H->socket->setConnTimeout(timeout);

		if (pdo_attr_lval(driver_options, static_cast <pdo_attribute_type>(PDO_CASSANDRA_ATTR_THRIFT_DEBUG), 0 TSRMLS_CC)) {
			// Convert thift messages to php warnings
			pdo_cassandra_toggle_thrift_debug(1);
		} else {
			// Disable output from thrift library
			pdo_cassandra_toggle_thrift_debug(0);
		}

		if (pdo_attr_lval(driver_options, static_cast <pdo_attribute_type>(PDO_CASSANDRA_ATTR_PRESERVE_VALUES), 0 TSRMLS_CC)) {
			H->preserve_values = 1;
		}
	}

	/* Break down the values */
	zend_bool rc = 0;
	if (dbh->data_source_len > 0) {
		rc = parse_dsn(dbh, H, dbh->data_source TSRMLS_CC);
	}

	if (!rc) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_INVALID_CONNECTION_STRING, "%s", "Invalid connection string attribute");
		pdo_cassandra_handle_close(dbh TSRMLS_CC);
		return 0;
	}

	try {
		H->transport->open();
		php_cassandra_handle_auth (dbh, H);
		return 1;
	} catch (NotFoundException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_NOT_FOUND, "%s", e.what());
	} catch (InvalidRequestException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_INVALID_REQUEST, "%s", e.why.c_str());
	} catch (UnavailableException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_UNAVAILABLE, "%s", e.what());
	} catch (TimedOutException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_TIMED_OUT, "%s", e.what());
	} catch (AuthenticationException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_AUTHENTICATION_ERROR, "%s", e.why.c_str());
	} catch (AuthorizationException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_AUTHORIZATION_ERROR, "%s", e.why.c_str());
	} catch (SchemaDisagreementException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_SCHEMA_DISAGREEMENT, "%s", e.what());
	} catch (TTransportException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_TRANSPORT_ERROR, "%s", e.what());
	} catch (TException &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	} catch (std::exception &e) {
		pdo_cassandra_error_exception(dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	}
	return 0;
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_prepare(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC)
*/
static int pdo_cassandra_handle_prepare(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast<pdo_cassandra_db_handle *> (dbh->driver_data);
	pdo_cassandra_stmt *S      = new pdo_cassandra_stmt;

	/* transfer handle with the statement */
	S->H = H;
	S->result.reset ();

	/* Fill in necessary driver data */
	stmt->driver_data           = S;
	stmt->methods               = &cassandra_stmt_methods;
	stmt->supports_placeholders = PDO_PLACEHOLDER_NONE;

	return 1;
}
/* }}} */

/** {{{ std::string pdo_cassandra_get_first_sub_pattern(const std::string &subject, const std::string &pattern TSRMLS_DC)
*/
std::string pdo_cassandra_get_first_sub_pattern(const std::string &subject, const std::string &pattern TSRMLS_DC)
{
	std::string ret;
	zval *return_value, *sub_patterns;
	pcre_cache_entry *pce;

	if ((pce = pcre_get_compiled_regex_cache(const_cast<char *>(pattern.c_str()), pattern.size() TSRMLS_CC)) == NULL) {
		return ret;
	}

	MAKE_STD_ZVAL(return_value);
	ALLOC_INIT_ZVAL(sub_patterns);

	php_pcre_match_impl(pce, const_cast<char *>(subject.c_str()), subject.size(), return_value, sub_patterns, 1, 1, 0, 0 TSRMLS_CC);

	if ((Z_LVAL_P(return_value) > 0) && (Z_TYPE_P(sub_patterns) == IS_ARRAY)) {

		if (zend_hash_index_exists(Z_ARRVAL_P(sub_patterns), (ulong) 1)) {
			zval **data = NULL;
			if (zend_hash_index_find(Z_ARRVAL_P(sub_patterns), (ulong) 1, (void**)&data) == SUCCESS) {
				if (Z_TYPE_PP(data) == IS_ARRAY) {
					if (zend_hash_index_exists(Z_ARRVAL_PP(data), (ulong) 0)) {
						zval **match = NULL;
						if (zend_hash_index_find(Z_ARRVAL_PP(data), (ulong) 0, (void**)&match) == SUCCESS) {
							ret = Z_STRVAL_PP(match);
						}
					}
				}
			}
		}
	}
	zval_ptr_dtor(&return_value);
	zval_ptr_dtor(&sub_patterns);
	return ret;
}
/* }}} */

/** {{{ void pdo_cassandra_set_active_keyspace(pdo_cassandra_db_handle *H, const std::string &sql TSRMLS_DC)
*/
void pdo_cassandra_set_active_keyspace(pdo_cassandra_db_handle *H, const std::string &sql TSRMLS_DC)
{
	std::string pattern("~USE\\s+[\\']?(\\w+)~ims");
	std::string match = pdo_cassandra_get_first_sub_pattern(sql, pattern TSRMLS_CC);

	if (!match.empty()) {
		H->active_keyspace = match;
		H->active_columnfamily.clear();
		// USE statement invalidates the current cache
		H->has_description = 0;
	}
}
/* }}} */

/** {{{ void pdo_cassandra_set_active_columnfamily(pdo_cassandra_db_handle *H, const std::string &query TSRMLS_DC)
*/
void pdo_cassandra_set_active_columnfamily(pdo_cassandra_db_handle *H, const std::string &query TSRMLS_DC)
{
	std::string pattern("~\\s*SELECT\\s+.+?\\s+FROM\\s+[\\']?(\\w+)~ims");
	std::string match = pdo_cassandra_get_first_sub_pattern(query, pattern TSRMLS_CC);

	if (!match.empty()) {
		H->active_columnfamily = match;
	}
}
/* }}} */

/** {{{ static long pdo_cassandra_handle_execute(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC)
*/
static long pdo_cassandra_handle_execute(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast<pdo_cassandra_db_handle *> (dbh->driver_data);

	try {
		/* Verify and try to reconnect if necessary */
		if (!H->transport->isOpen()) {
			H->transport->open();
		}

		std::string query(sql);

		CqlResult result;
		H->client->execute_cql_query(result, query, (H->compression ? Compression::GZIP : Compression::NONE));

		if (result.type == CqlResultType::INT) {
			return result.num;
		}
		pdo_cassandra_set_active_keyspace(H, query TSRMLS_CC);
		pdo_cassandra_set_active_columnfamily(H, query TSRMLS_CC);

		return 0;
	} catch (NotFoundException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_NOT_FOUND, "%s", e.what());
	} catch (InvalidRequestException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_INVALID_REQUEST, "%s", e.why.c_str());
	} catch (UnavailableException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_UNAVAILABLE, "%s", e.what());
	} catch (TimedOutException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_TIMED_OUT, "%s", e.what());
	} catch (AuthenticationException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_AUTHENTICATION_ERROR, "%s", e.why.c_str());
	} catch (AuthorizationException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_AUTHORIZATION_ERROR, "%s", e.why.c_str());
	} catch (SchemaDisagreementException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_SCHEMA_DISAGREEMENT, "%s", e.what());
	} catch (TTransportException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_TRANSPORT_ERROR, "%s", e.what());
	} catch (TException &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	} catch (std::exception &e) {
		pdo_cassandra_error(dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	}
	return 0;
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_quote(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype TSRMLS_DC)
*/
static int pdo_cassandra_handle_quote(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype TSRMLS_DC)
{
	char *escaped;
	int new_length;

	// const_cast should be fine here, php_addslashes shouldn't modify the data
	escaped = php_addslashes(const_cast <char *>(unquoted), unquotedlen, &new_length, 0 TSRMLS_CC);

	if (!escaped) {
		return 0;
	}

	*quotedlen = spprintf(quoted, 0, "'%s'", escaped);
	efree(escaped);
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_close(pdo_dbh_t *dbh TSRMLS_DC)
*/
static int pdo_cassandra_handle_close(pdo_dbh_t *dbh TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(dbh->driver_data);

	if (H) {
		pdo_cassandra_einfo *einfo = &H->einfo;

		H->transport->close();
		H->socket.reset();
		H->transport.reset();
		H->protocol.reset();
		H->client.reset();

		if (einfo->errmsg) {
			pefree(einfo->errmsg, dbh->is_persistent);
			einfo->errmsg = NULL;
		}
		delete H;
		dbh->driver_data = NULL;
	}
	return 0;
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC)
*/
static int pdo_cassandra_handle_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(dbh->driver_data);
	pdo_cassandra_constant attribute = static_cast <pdo_cassandra_constant>(attr);

	switch (attribute) {

		case PDO_CASSANDRA_ATTR_NUM_RETRIES:
			convert_to_long(val);
			H->socket->setNumRetries(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_RETRY_INTERVAL:
			convert_to_long(val);
			H->socket->setRetryInterval(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_MAX_CONSECUTIVE_FAILURES:
			convert_to_long(val);
			H->socket->setMaxConsecutiveFailures(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_RANDOMIZE:
			convert_to_boolean(val);
			H->socket->setMaxConsecutiveFailures(Z_BVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_ALWAYS_TRY_LAST:
			convert_to_boolean(val);
			H->socket->setAlwaysTryLast(Z_BVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_LINGER:
			convert_to_long(val);

			if (Z_LVAL_P(val) == 0) {
				H->socket->setLinger(false, 0);
			} else {
				H->socket->setLinger(true, Z_LVAL_P(val));
			}
		break;

		case PDO_CASSANDRA_ATTR_NO_DELAY:
			convert_to_boolean(val);
			H->socket->setNoDelay(Z_BVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_CONN_TIMEOUT:
			convert_to_long(val);
			H->socket->setConnTimeout(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_RECV_TIMEOUT:
			convert_to_long(val);
			H->socket->setRecvTimeout(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_SEND_TIMEOUT:
			convert_to_long(val);
			H->socket->setSendTimeout(Z_LVAL_P(val));
		break;

		case PDO_CASSANDRA_ATTR_COMPRESSION:
			convert_to_boolean(val);
			H->compression = Z_BVAL_P(val);
		break;

		case PDO_CASSANDRA_ATTR_THRIFT_DEBUG:
			convert_to_boolean(val);
			if (Z_BVAL_P(val)) {
				// Convert thift messages to php warnings
				GlobalOutput.setOutputFunction(&php_cassandra_thrift_debug_output);
			} else {
				// Disable output from thrift library
				GlobalOutput.setOutputFunction(&php_cassandra_thrift_no_output);
			}
		break;

		case PDO_CASSANDRA_ATTR_PRESERVE_VALUES:
			convert_to_boolean(val);
			H->preserve_values = Z_BVAL_P(val);
		break;

		default:
			return 0;
	}
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_handle_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)
*/
static int pdo_cassandra_handle_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)
{
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(dbh->driver_data);

	switch (attr) {

		case PDO_ATTR_SERVER_VERSION:
		{
			std::string version;
			H->client->describe_version(version);
			ZVAL_STRING(return_value, version.c_str(), 1);
		}
		break;

		case PDO_ATTR_CLIENT_VERSION:
			ZVAL_STRING(return_value, PHP_PDO_CASSANDRA_EXTVER, 1);
		break;

		default:
			return 0;
	}
	return 1;
}
/* }}} */

pdo_driver_t pdo_cassandra_driver = {
	PDO_DRIVER_HEADER(cassandra),
	pdo_cassandra_handle_factory
};

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(pdo_cassandra)
{
	// Disable debug output from Thrift
	pdo_cassandra_toggle_thrift_debug(0);

	// Class constants
#define PHP_PDO_CASSANDRA_REGISTER_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(php_pdo_get_dbh_ce(), const_name, sizeof(const_name)-1, (long)value TSRMLS_CC);	

	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_NUM_RETRIES",					PDO_CASSANDRA_ATTR_NUM_RETRIES);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_RETRY_INTERVAL",				PDO_CASSANDRA_ATTR_RETRY_INTERVAL);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_MAX_CONSECUTIVE_FAILURES",	PDO_CASSANDRA_ATTR_MAX_CONSECUTIVE_FAILURES);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_RANDOMIZE",					PDO_CASSANDRA_ATTR_RANDOMIZE);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_ALWAYS_TRY_LAST",				PDO_CASSANDRA_ATTR_ALWAYS_TRY_LAST);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_LINGER",						PDO_CASSANDRA_ATTR_LINGER);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_NO_DELAY",					PDO_CASSANDRA_ATTR_NO_DELAY);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_CONN_TIMEOUT",				PDO_CASSANDRA_ATTR_CONN_TIMEOUT);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_RECV_TIMEOUT",				PDO_CASSANDRA_ATTR_RECV_TIMEOUT);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_SEND_TIMEOUT",				PDO_CASSANDRA_ATTR_SEND_TIMEOUT);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_COMPRESSION",					PDO_CASSANDRA_ATTR_COMPRESSION);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_THRIFT_DEBUG",				PDO_CASSANDRA_ATTR_THRIFT_DEBUG);
	PHP_PDO_CASSANDRA_REGISTER_CONST_LONG("CASSANDRA_ATTR_PRESERVE_VALUES",				PDO_CASSANDRA_ATTR_PRESERVE_VALUES);

#undef PHP_PDO_CASSANDRA_REGISTER_CONST_LONG

	return php_pdo_register_driver(&pdo_cassandra_driver);
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(pdo_cassandra)
{
	php_pdo_unregister_driver(&pdo_cassandra_driver);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pdo_cassandra)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "PDO Driver for Cassandra", "enabled");
	php_info_print_table_header(2, "PDO Driver for Cassandra version", PHP_PDO_CASSANDRA_EXTVER);
	php_info_print_table_end();
}
/* }}} */

/* {{{ pdo_cassandra_deps
 */
#if ZEND_MODULE_API_NO >= 20050922
static const zend_module_dep pdo_cassandra_deps[] = {
	ZEND_MOD_REQUIRED("pdo")
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/* {{{ pdo_cassandra_module_entry
 */
zend_module_entry pdo_cassandra_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	pdo_cassandra_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	PHP_PDO_CASSANDRA_EXTNAME,
	pdo_cassandra_functions,
	PHP_MINIT(pdo_cassandra),
	PHP_MSHUTDOWN(pdo_cassandra),
	NULL,
	NULL,
	PHP_MINFO(pdo_cassandra),
	PHP_PDO_CASSANDRA_EXTVER,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#if defined(COMPILE_DL_PDO_CASSANDRA)
ZEND_GET_MODULE(pdo_cassandra)
#endif