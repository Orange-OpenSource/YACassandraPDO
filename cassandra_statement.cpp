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

/** {{{ static int pdo_cassandra_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
*/
static int pdo_cassandra_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	try {
		if (!H->transport->isOpen()) {
			H->transport->open();
		}

		std::string query(stmt->active_query_string);

		S->result.reset(new CqlResult);
		H->client->execute_cql_query(*S->result.get(), query, (H->compression ? Compression::GZIP : Compression::NONE));
		S->has_iterator = 0;
		stmt->row_count = S->result.get()->rows.size();
		pdo_cassandra_set_active_keyspace(H, query);

		return 1;
	} catch (NotFoundException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_NOT_FOUND, "%s", e.what());
	} catch (InvalidRequestException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_INVALID_REQUEST, "%s", e.why.c_str());
	} catch (UnavailableException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_UNAVAILABLE, "%s", e.what());
	} catch (TimedOutException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_TIMED_OUT, "%s", e.what());
	} catch (AuthenticationException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_AUTHENTICATION_ERROR, "%s", e.why.c_str());
	} catch (AuthorizationException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_AUTHORIZATION_ERROR, "%s", e.why.c_str());
	} catch (SchemaDisagreementException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_SCHEMA_DISAGREEMENT, "%s", e.what());
	} catch (TTransportException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_TRANSPORT_ERROR, "%s", e.what());
	} catch (TException &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	} catch (std::exception &e) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_GENERAL_ERROR, "%s", e.what());
	}
	return 0;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
*/
static int pdo_cassandra_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);

	if (!S->result.get()->rows.size ())
		return 0;

	if (!S->has_iterator) {
		S->it = S->result.get()->rows.begin();
		S->has_iterator = 1;

		// Find the largets number of columns
		stmt->column_count = 0;
		for (size_t i = 0; i < S->result.get()->rows.size(); i++) {
			if (S->result.get()->rows [i].columns.size() > stmt->column_count) {
				stmt->column_count = S->result.get()->rows [i].columns.size();
			}
		}
	} else {
		S->it++;
	}

	if (S->it == S->result.get()->rows.end()) {
		// Iterated all rows, reset the iterator
		S->has_iterator = 0;
		S->it = S->result.get()->rows.begin();
		return 0;
	}
	return 1;
}
/* }}} */

/** {{{ pdo_cassandra_type pdo_cassandra_get_type(const std::string &type)
*/
pdo_cassandra_type pdo_cassandra_get_type(const std::string &type)
{
	if (!type.compare("org.apache.cassandra.db.marshal.BytesType")) {
		return PDO_CASSANDRA_TYPE_BYTES;
	} else if (!type.compare("org.apache.cassandra.db.marshal.AsciiType")) {
		return PDO_CASSANDRA_TYPE_ASCII;
	} else if (!type.compare("org.apache.cassandra.db.marshal.UTF8Type")) {
		return PDO_CASSANDRA_TYPE_UTF8;
	} else if (!type.compare("org.apache.cassandra.db.marshal.IntegerType")) {
		return PDO_CASSANDRA_TYPE_INTEGER;
	} else if (!type.compare("org.apache.cassandra.db.marshal.LongType")) {
		return PDO_CASSANDRA_TYPE_LONG;
	} else if (!type.compare("org.apache.cassandra.db.marshal.UUIDType")) {
		return PDO_CASSANDRA_TYPE_UUID;
	} else if (!type.compare("org.apache.cassandra.db.marshal.LexicalType")) {
		return PDO_CASSANDRA_TYPE_LEXICAL;
	} else if (!type.compare("org.apache.cassandra.db.marshal.TimeUUIDType")) {
		return PDO_CASSANDRA_TYPE_TIMEUUID;
	} else {
		return PDO_CASSANDRA_TYPE_UNKNOWN;
	}
}
/* }}} */

/** {{{ static int64_t pdo_cassandra_marshal_numeric(const std::string &test)
*/
static int64_t pdo_cassandra_marshal_numeric(const std::string &test) 
{
	const unsigned char *bytes = reinterpret_cast <const unsigned char *>(test.c_str());

	int64_t val = 0;
	size_t siz = test.size ();
	for (size_t i = 0; i < siz; i++)
		val = val << 8 | bytes[i];

	return val;
}
/* }}} */

static void pdo_cassandra_empty_column(pdo_stmt_t *stmt, int colno)
{
	if (stmt->columns[colno].namelen)
		efree(stmt->columns[colno].name);

	stmt->columns[colno].namelen    = spprintf(&(stmt->columns[colno].name), 0, "__column_not_set_%d", colno);
	stmt->columns[colno].precision  = 0;
	stmt->columns[colno].maxlen     = -1;
	stmt->columns[colno].param_type = PDO_PARAM_STR;
}

/** {{{ static int pdo_cassandra_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
*/
static int pdo_cassandra_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	// This could be sparse column scenario
	if (static_cast <size_t>(colno) >= (*S->it).columns.size()) {
		pdo_cassandra_empty_column(stmt, colno);
		return 1;
	}

	if (!H->has_description) {
		H->client->describe_keyspace(H->description, H->active_keyspace);
		H->has_description = 1;
	}

	stmt->columns[colno].param_type = PDO_PARAM_STR;
	stmt->columns[colno].namelen    = 0;

	for (size_t i = 0; i < H->description.cf_defs.size(); i++) {
		for (size_t j = 0; j < H->description.cf_defs [i].column_metadata.size(); j++) {
			if ((*S->it).columns[colno].name.size () == H->description.cf_defs [i].column_metadata [j].name.size () &&
				!memcmp ((*S->it).columns[colno].name.c_str (), H->description.cf_defs [i].column_metadata [j].name.c_str (), (*S->it).columns[colno].name.size ())) {

				pdo_cassandra_type value_type = pdo_cassandra_get_type(H->description.cf_defs [i].column_metadata [j].validation_class);

				switch (value_type) {
					case PDO_CASSANDRA_TYPE_LONG:
					case PDO_CASSANDRA_TYPE_INTEGER:
						stmt->columns[colno].param_type = PDO_PARAM_INT;
						break;

					default:
						stmt->columns[colno].param_type = PDO_PARAM_STR;
						break;
				}
			}			
		}
	}

	stmt->columns[colno].name      = estrndup (const_cast <char *> ((*S->it).columns[colno].name.c_str()), (*S->it).columns[colno].name.size());
	stmt->columns[colno].namelen   = (*S->it).columns[colno].name.size();
	stmt->columns[colno].precision = 0;
	stmt->columns[colno].maxlen    = -1;
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
*/
static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	// This could be sparse column scenario, column is not set for this row
	if (static_cast <size_t>(colno) >= (*S->it).columns.size()) {
		pdo_cassandra_empty_column(stmt, colno);
		return 1;
	}

	switch (stmt->columns[colno].param_type)
	{
		case PDO_PARAM_INT:
		{
			long value = (long) pdo_cassandra_marshal_numeric((*S->it).columns[colno].value);
			long *p    = (long *) emalloc(sizeof (long));
			memcpy (p, &value, sizeof(long));

			*ptr          = (char *)p;
			*len          = sizeof(long);
			*caller_frees = 1;
		}
		break;

		default:
			*ptr          = const_cast <char *> ((*S->it).columns[colno].value.c_str());
			*len          = (*S->it).columns[colno].value.size();
			*caller_frees = 0;
		break;
	}

	// Key is always key 
	if (!H->has_description) {
		H->client->describe_keyspace(H->description, H->active_keyspace);
		H->has_description = 1;
	}

	if (!(*S->it).columns[colno].name.compare(H->description.cf_defs [0].key_alias)) {
		return 1;
	}

	pdo_cassandra_type name_type = pdo_cassandra_get_type(H->description.cf_defs [0].comparator_type);

	if (name_type == PDO_CASSANDRA_TYPE_LONG || name_type == PDO_CASSANDRA_TYPE_INTEGER) {
		char col [96];
		size_t len;
		long name = (long) pdo_cassandra_marshal_numeric((*S->it).columns[colno].name);
		len = snprintf(col, 96, "%ld", name);

		if (strcmp (col, stmt->columns[colno].name)) {
			if (stmt->columns[colno].namelen) {
				efree (stmt->columns[colno].name);
			}
			stmt->columns[colno].name    = estrdup (col);
			stmt->columns[colno].namelen = len;
		}
	} else {
		if ((*S->it).columns[colno].name.size() != stmt->columns[colno].namelen ||
			memcmp ((*S->it).columns[colno].name.c_str(), stmt->columns[colno].name, stmt->columns[colno].namelen)) {
			if (stmt->columns[colno].namelen) {
				efree (stmt->columns[colno].name);
			}
			stmt->columns[colno].name    = estrndup (const_cast <char *> ((*S->it).columns[colno].name.c_str()), (*S->it).columns[colno].name.size());
			stmt->columns[colno].namelen = (*S->it).columns[colno].name.size();
		}
	}
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_get_column_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
*/
static int pdo_cassandra_stmt_get_column_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	if (!stmt->row_count) {
		return FAILURE;
	}
	array_init(return_value);

	if (!H->has_description) {
		H->client->describe_keyspace(H->description, H->active_keyspace);
		H->has_description = 1;
	}

	bool found = false;
	for (size_t i = 0; i < H->description.cf_defs.size(); i++) {
		for (size_t j = 0; j < H->description.cf_defs [i].column_metadata.size(); j++) {
			if (!(*S->it).columns[colno].name.compare(H->description.cf_defs[i].column_metadata[j].name))
			{
				found = true;
				add_assoc_string(return_value,
				                 "native_type",
								 const_cast <char *> (H->description.cf_defs[i].column_metadata[j].validation_class.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "comparator",
								 const_cast <char *> (H->description.cf_defs[i].comparator_type.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "default_validation_class",
								 const_cast <char *> (H->description.cf_defs[i].default_validation_class.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "key_validation_class",
								 const_cast <char *> (H->description.cf_defs[i].key_validation_class.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "key_alias",
								 const_cast <char *> (H->description.cf_defs[i].key_alias.c_str()),
								 1);
			} else if (!H->description.cf_defs [i].key_alias.compare((*S->it).columns[colno].name)) {
				add_assoc_string(return_value,
				                 "native_type",
								 "key_alias",
								 1);
				found = true;
			}
		}
	}

	if (!found) {
		add_assoc_string(return_value,
		                 "native_type",
						 "unknown",
						 1);
	}
	return SUCCESS;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_cursor_closer(pdo_stmt_t *stmt TSRMLS_DC)
*/
static int pdo_cassandra_stmt_cursor_close(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	S->has_iterator       = 0;
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
*/
static int pdo_cassandra_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
{
	if (stmt->driver_data) {
		pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
		S->result.reset();
		delete S;
		stmt->driver_data = NULL;
	}
	return 1;
}
/* }}} */


struct pdo_stmt_methods cassandra_stmt_methods = {
	pdo_cassandra_stmt_dtor,
	pdo_cassandra_stmt_execute,
	pdo_cassandra_stmt_fetch,
	pdo_cassandra_stmt_describe,
	pdo_cassandra_stmt_get_column,
	NULL, /* param_hook */
	NULL, /* set_attr */
	NULL, /* get_attr */
	pdo_cassandra_stmt_get_column_meta,
	NULL, /* next_rowset */
	pdo_cassandra_stmt_cursor_close
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
