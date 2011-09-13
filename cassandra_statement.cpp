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

static pdo_param_type pdo_cassandra_get_type(const std::string &type);
static long pdo_cassandra_marshal_numeric(pdo_stmt_t *stmt, const std::string &binary);

static zend_bool pdo_cassandra_describe_keyspace(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	if (H->has_description) {
		return 1;
	}

	try {
		H->client->describe_keyspace(H->description, H->active_keyspace);
		H->has_description = 1;
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

/** {{{ static void pdo_cassandra_stmt_undescribe(pdo_stmt_t *stmt TSRMLS_DC)
*/
static void pdo_cassandra_stmt_undescribe(pdo_stmt_t *stmt TSRMLS_DC)
{
	if (stmt->columns) {
		int i;
		struct pdo_column_data *cols = stmt->columns;

		for (i = 0; i < stmt->column_count; i++) {
			efree(cols[i].name);
		}
		efree(stmt->columns);
		stmt->columns = NULL;
		stmt->column_count = 0;
	}

	// Clear data
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	S->original_column_names.clear();
	S->column_name_labels.clear();

	stmt->executed = 0;
}
/* }}} */

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
		pdo_cassandra_set_active_keyspace(H, query TSRMLS_CC);
		pdo_cassandra_set_active_columnfamily(H, query TSRMLS_CC);

		// Undescribe the result set because next time there might be different amount of columns
		pdo_cassandra_stmt_undescribe(stmt TSRMLS_CC);
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

static zend_bool pdo_cassandra_add_column(pdo_stmt_t *stmt, const std::string &name, int order)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);

	try {
		S->original_column_names.left.at(name);
		return 0;
	} catch (std::out_of_range &ex) {
		S->original_column_names.insert(ColumnMap::value_type(name, order));

        pdo_param_type name_type;
		if (S->result.get ()->schema.name_types[name].size () == 0) {
            name_type = pdo_cassandra_get_type(S->result.get ()->schema.default_name_type);
		} else {
		    name_type = pdo_cassandra_get_type(S->result.get ()->schema.name_types [name]);
		}

		if (name_type == PDO_PARAM_INT && name.size() <= 8) {
			char label[96];
			size_t len;
			long namel = (long) pdo_cassandra_marshal_numeric(stmt, name);
			len = snprintf(label, 96, "%ld", namel);
			S->column_name_labels.insert(ColumnMap::value_type(std::string(label, len), order));
		}
		return 1;
	}
}

/** {{{ static int pdo_cassandra_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
*/
static int pdo_cassandra_stmt_fetch(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	int order = 0;

	if (!stmt->executed || !S->result.get()->rows.size()) {
		return 0;
	}

	if (!S->has_iterator) {
		S->it = S->result.get()->rows.begin();
		S->has_iterator = 1;

		// Set column names and labels if we don't have them already
		if (!S->original_column_names.size()) {

			// Get unique column names
			for (std::vector<CqlRow>::iterator it = S->result.get()->rows.begin(); it < S->result.get()->rows.end(); it++) {
				for (std::vector<Column>::iterator col_it = (*it).columns.begin(); col_it < (*it).columns.end(); col_it++) {
					if (pdo_cassandra_add_column(stmt, (*col_it).name, order)) {
						order++;
					}
				}
			}
			stmt->column_count = order;
		}
		stmt->column_count = S->original_column_names.size();
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
static pdo_param_type pdo_cassandra_get_type(const std::string &type)
{
    std::string real_type;

    if (type.find("org.apache.cassandra.db.marshal.") != std::string::npos) {
        real_type = type.substr(::strlen("org.apache.cassandra.db.marshal."));
    } else {
        real_type = type;
    }
    if (!real_type.compare("IntegerType") ||
        !real_type.compare("Int32Type") ||
        !real_type.compare("LongType") ||
        !real_type.compare("CounterColumnType")) {
        return PDO_PARAM_INT;
    }
    return PDO_PARAM_STR;
}
/* }}} */

/** {{{ static long pdo_cassandra_marshal_numeric(const std::string &binary)
*/
static long pdo_cassandra_marshal_numeric(pdo_stmt_t *stmt, const std::string &binary) 
{
	if (sizeof(long) < binary.size()) {
		pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_INTEGER_CONVERSION_ERROR, "The value is too large for integer datatype", "");
		return LONG_MAX;
	}

	const unsigned char *bytes = reinterpret_cast <const unsigned char *>(binary.c_str());

	long val = 0;
	size_t siz = binary.size ();
	for (size_t i = 0; i < siz; i++)
		val = val << 8 | bytes[i];

	return val;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
*/
static int pdo_cassandra_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	std::string current_column;
	try {
		current_column = S->original_column_names.right.at(colno);
	} catch (std::out_of_range &ex) {
		return 0;
	}

	try {
	    std::string column_label = S->column_name_labels.right.at(colno);

	    stmt->columns[colno].name    = estrndup(column_label.c_str(), column_label.size());
    	stmt->columns[colno].namelen = column_label.size();

	} catch (std::exception &e) {
        stmt->columns[colno].name    = estrndup(current_column.c_str(), current_column.size());
    	stmt->columns[colno].namelen = current_column.size();
	}

    if (H->preserve_values) {
        stmt->columns[colno].param_type = PDO_PARAM_STR;
    } else {
    	// Do we have schema?
        if (S->result.get ()->schema.value_types [current_column].size() == 0) {
            stmt->columns[colno].param_type = pdo_cassandra_get_type(S->result.get ()->schema.default_value_type);
        } else {
            stmt->columns[colno].param_type = pdo_cassandra_get_type(S->result.get ()->schema.value_types [current_column]);
        }
    }

	stmt->columns[colno].precision  = 0;
	stmt->columns[colno].maxlen     = -1;
	return 1;
}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
*/
static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);

	std::string current_column;
	try {
		current_column = S->original_column_names.right.at(colno);
	} catch (std::out_of_range &ex) {
		return 0;
	}

	*ptr          = NULL;
	*len          = 0;
	*caller_frees = 0;

	// Do we have data for this column? 
	for (std::vector<Column>::iterator col_it = (*S->it).columns.begin(); col_it < (*S->it).columns.end(); col_it++) {
		if (!current_column.compare(0, current_column.size(), (*col_it).name.c_str(), (*col_it).name.size())) {
			switch (stmt->columns[colno].param_type)
			{
				case PDO_PARAM_INT:
				{
					long value = (long) pdo_cassandra_marshal_numeric(stmt, (*col_it).value);
					long *p    = (long *) emalloc(sizeof(long));
					memcpy(p, &value, sizeof(long));

					*ptr          = (char *)p;
					*len          = sizeof(long);
					*caller_frees = 1;
				}
				break;

				default:
					*ptr          = const_cast <char *> ((*col_it).value.c_str());
					*len          = (*col_it).value.size();
					*caller_frees = 0;
				break;
			}
			return 1;
		}
	}
	return 0;

}
/* }}} */

/** {{{ static int pdo_cassandra_stmt_get_column_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
*/
static int pdo_cassandra_stmt_get_column_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
{
	pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
	pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

	// If the stmt is not in executed state, don't even try
	if (!stmt->executed) {
		return FAILURE;
	}

	std::string current_column;
	try {
		current_column = S->original_column_names.right.at(colno);
	} catch (std::out_of_range &ex) {
		return FAILURE;
	}
	array_init(return_value);

	if (!pdo_cassandra_describe_keyspace(stmt TSRMLS_CC)) {
		return 0;
	}

	add_assoc_stringl(return_value,
	                 "keyspace",
					 const_cast <char *> (H->active_keyspace.c_str()),
					 H->active_keyspace.size(),
					 1);

	add_assoc_stringl(return_value,
	                 "columnfamily",
					 const_cast <char *> (H->active_columnfamily.c_str()),
					 H->active_columnfamily.size(),
					 1);

	bool found = false;
	for (std::vector<CfDef>::iterator cfdef_it = H->description.cf_defs.begin(); cfdef_it < H->description.cf_defs.end(); cfdef_it++) {
		for (std::vector<ColumnDef>::iterator columndef_it = (*cfdef_it).column_metadata.begin(); columndef_it < (*cfdef_it).column_metadata.end(); columndef_it++) {

			// Only interested in the currently active CF
			if ((*cfdef_it).name.compare(H->active_columnfamily)) {
				continue;
			}
			else if (!(*cfdef_it).key_alias.compare(current_column)) {
				found = true;
				add_assoc_string(return_value,
				                 "native_type",
								 "key_alias",
								 1);
				break;
			}
			else if (!current_column.compare(0, current_column.size(), (*columndef_it).name.c_str(), (*columndef_it).name.size())) {
				found = true;
				add_assoc_string(return_value,
				                 "native_type",
								 const_cast <char *> ((*columndef_it).validation_class.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "comparator",
								 const_cast <char *> ((*cfdef_it).comparator_type.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "default_validation_class",
								 const_cast <char *> ((*cfdef_it).default_validation_class.c_str()),
								 1);
				add_assoc_string(return_value,
				                 "key_validation_class",
								 const_cast <char *> ((*cfdef_it).key_validation_class.c_str()),
								 1);
				add_assoc_stringl(return_value,
				                 "key_alias",
								 const_cast <char *> ((*cfdef_it).key_alias.c_str()),
								 (*cfdef_it).key_alias.size(),
								 1);
				add_assoc_stringl(return_value,
				                 "original_column_name",
								 const_cast <char *> (current_column.c_str()),
								 current_column.size(),
								 1);
				break;
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
	NULL, /* next rowset */
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
