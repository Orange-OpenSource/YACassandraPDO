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

#include <map>
#include <sstream>

template <class T>
T pdo_cassandra_marshal_numeric(pdo_stmt_t *stmt, const std::string &binary);
static pdo_cassandra_type pdo_cassandra_get_cassandra_type(const std::string &type);

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
        H->client->execute_cql3_query(*S->result.get(), query, (H->compression ? Compression::GZIP : Compression::NONE), H->consistency);
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

static std::vector<pdo_cassandra_type> pdo_cassandra_get_element_type_in_collection(pdo_cassandra_type col_type, const std::string &type)
{
    std::vector<pdo_cassandra_type> ret;
    std::string real_type;
    int st = type.find('(') + 1;
    std::string rtype = type.substr(st, type.size() - st - 1);
    if (col_type == PDO_CASSANDRA_TYPE_MAP) {
        st = rtype.find(',');
        ret.push_back(pdo_cassandra_get_cassandra_type(rtype.substr(0, st)));
        ret.push_back(pdo_cassandra_get_cassandra_type(rtype.substr(st + 1)));
    }
    else
        ret.push_back(pdo_cassandra_get_cassandra_type(rtype));
    return ret;
}

/** {{{ pdo_cassandra_type pdo_cassandra_get_cassandra_type(const std::string &type)
*/
static pdo_cassandra_type pdo_cassandra_get_cassandra_type(const std::string &type)
{
    std::string real_type;
    if (type.find("org.apache.cassandra.db.marshal.") != std::string::npos)
        real_type = type.substr(::strlen("org.apache.cassandra.db.marshal."));
    else
        real_type = type;

    if (!real_type.compare("IntegerType"))
        return PDO_CASSANDRA_TYPE_VARINT;
    if (!real_type.compare("Int32Type"))
        return PDO_CASSANDRA_TYPE_INTEGER;
    if (!real_type.compare("LongType") ||
        !real_type.compare("DateType") ||
        !real_type.compare("CounterColumnType") ||
	!real_type.compare("TimestampType"))
        return PDO_CASSANDRA_TYPE_LONG;
    if (!real_type.compare("BooleanType"))
        return PDO_CASSANDRA_TYPE_BOOLEAN;
    if (!real_type.compare("DecimalType"))
        return PDO_CASSANDRA_TYPE_DECIMAL;
    if (!real_type.compare("FloatType"))
        return PDO_CASSANDRA_TYPE_FLOAT;
    if (!real_type.compare("DoubleType"))
        return PDO_CASSANDRA_TYPE_DOUBLE;
    if (!real_type.compare("UUIDType"))
        return PDO_CASSANDRA_TYPE_UUID;
    if (!real_type.compare("TimeUUIDType"))
        return PDO_CASSANDRA_TYPE_TIMEUUID;
    if (!real_type.compare(0, 7, "SetType"))
        return PDO_CASSANDRA_TYPE_SET;
    if (!real_type.compare(0, 7, "MapType"))
        return PDO_CASSANDRA_TYPE_MAP;
    if (!real_type.compare(0, 8, "ListType"))
        return PDO_CASSANDRA_TYPE_LIST;
    return PDO_CASSANDRA_TYPE_UTF8;
}
/* }}} */

namespace StreamExtraction {

    /**
     * Creates a zval reading bina
     */
    zval* extract_zval(const unsigned char *binary, pdo_cassandra_type type, int size);


    /**
     * Read n bytes to a primitive type
     */
    template <class T>
    T extract(const unsigned char *bytes, unsigned int stream_size = sizeof(T))
    {
        T val = T();
        unsigned char *pval = reinterpret_cast<unsigned char *>(&val) + stream_size;
        for (size_t i = 0; i < stream_size; i++) {
        --pval;
        *pval = bytes[i];
        }
        return val;
    }

    // Zval creators signature
    typedef zval *(*EvaluatorType)(const unsigned char *binary, int size);

    /**
     * Evaluator: Ascii extraction
     */
    zval *evaluate_string(const unsigned char *binary, int size) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_STRING;
        char *str = (char *) emalloc(sizeof(*str) * (size + 1));
        memcpy(str, binary, size);
        str[size] = 0;
        Z_STRVAL_P(ret) = str;
        Z_STRLEN_P(ret) = size;
        return ret;
    }

    /**
     * Evaluator: UUID extraction
     */
    char to_hex(int x)
    {
        return x < 10 ? x + '0' : (x - 10) + 'a';
    }

    char *raw_uuid_to_str(const unsigned char *binary, int size) {

        int size_str = size * 2 + 4;

        char *str = (char *) emalloc(sizeof(*str) * (size_str + 1));

        int writepos = 0;
        int readpos = 0;
        for (int i = 0; i < size + 4; ++i) {
            if (i == 4 || i == 7 || i == 10 || i == 13) {
                str[writepos++] = '-';
            } else {
                str[writepos++] = to_hex((binary[readpos] >> 4) & 0x0F);
                str[writepos++] = to_hex(binary[readpos] & 0x0F);
                ++readpos;
            }
        }

        str[size_str] = 0;
		return str;
    }

    zval *evaluate_uuid(const unsigned char *binary, int size) {

        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_STRING;

		char *str = StreamExtraction::raw_uuid_to_str(binary, size);

        Z_STRVAL_P(ret) = str;
        Z_STRLEN_P(ret) = strlen(str);
        return ret;
    }

    /**
     * Evaluator: Binary to char *
     */
    zval *evaluate_bytes_to_zval(const unsigned char *binary, int size) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_RESOURCE;
        char *str = (char *) emalloc(sizeof(*str) * (size));
        memcpy(str, binary, size);
        Z_STRVAL_P(ret) = str;
        Z_STRLEN_P(ret) = size;
        return ret;
    }

    /**
     * Evaluator: integer and long extractions
     */
    template <class T>
    zval *evaluate_integer_type(const unsigned char *binary, int stream_size = sizeof(T)) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_LONG;
        Z_LVAL_P(ret) = 0L;
        Z_LVAL_P(ret) = StreamExtraction::extract<T>(binary, stream_size);
        return ret;
    }

    template <>
    zval *evaluate_integer_type<bool>(const unsigned char *binary, int stream_size) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_BOOL;
        Z_LVAL_P(ret) = 0L;
        Z_LVAL_P(ret) = StreamExtraction::extract<bool>(binary, sizeof(bool));
        return ret;
    }


    /**
     * Decimal number extraction
     * Php lacks such a type therefore we return an array containing the unscaled value and the scale
     */
    zval *evaluate_decimal_type(const unsigned char *binary, int size) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        array_init(ret);
        // Scale extraction
        zval *scale = extract_zval(binary, PDO_CASSANDRA_TYPE_INTEGER, sizeof(int));
        add_next_index_zval(ret, scale);

        // Unscaled value extraction
        zval *unscaled_value;
        int unscaled_val_size = size - sizeof(int);
        do {
            int natural_unscaled_val_size = ((unscaled_val_size > 0) ? unscaled_val_size : 0);
            if (unscaled_val_size  <= (int) sizeof(int)) {
                unscaled_value = evaluate_integer_type<int>(binary + sizeof(int), natural_unscaled_val_size);
                unscaled_val_size -= 4;
            }
            else {
                unscaled_value = evaluate_integer_type<long>(binary + sizeof(int), natural_unscaled_val_size);
                unscaled_val_size -= 8;
            }
            add_next_index_zval(ret, unscaled_value);
        } while (unscaled_val_size > 0);
        return ret;
    }

    /**
     * Evaluator: float and double extraction
     */
    template <class T>
    zval *evaluate_float_type(const unsigned char *binary, int size) {
        zval *ret;
        MAKE_STD_ZVAL(ret);
        Z_TYPE_P(ret) = IS_DOUBLE;
        Z_DVAL_P(ret) = (double)0.0;
        Z_DVAL_P(ret) = StreamExtraction::extract<T>(binary);
        return ret;
    }



        /**
     * Pointers on evaluators indexed by the pdo type
     */
    static std::map<pdo_cassandra_type, EvaluatorType> m_evaluation_map;

    static EvaluatorType evaluate(pdo_cassandra_type type) {
        static bool init = false;
        if (!init) {
            m_evaluation_map[PDO_CASSANDRA_TYPE_BYTES]          = evaluate_bytes_to_zval;
            m_evaluation_map[PDO_CASSANDRA_TYPE_ASCII]          = evaluate_string;
            m_evaluation_map[PDO_CASSANDRA_TYPE_UTF8]           = evaluate_string;
            m_evaluation_map[PDO_CASSANDRA_TYPE_INTEGER]        = evaluate_integer_type<int>;
            m_evaluation_map[PDO_CASSANDRA_TYPE_LONG]           = evaluate_integer_type<long>;
            m_evaluation_map[PDO_CASSANDRA_TYPE_DECIMAL]        = evaluate_decimal_type;
            m_evaluation_map[PDO_CASSANDRA_TYPE_UUID]           = evaluate_uuid;
            m_evaluation_map[PDO_CASSANDRA_TYPE_LEXICAL]        = 0;
            m_evaluation_map[PDO_CASSANDRA_TYPE_TIMEUUID]       = evaluate_uuid;
            m_evaluation_map[PDO_CASSANDRA_TYPE_BOOLEAN]        = evaluate_integer_type<bool>;
            m_evaluation_map[PDO_CASSANDRA_TYPE_VARINT]         = evaluate_bytes_to_zval;
            m_evaluation_map[PDO_CASSANDRA_TYPE_FLOAT]          = evaluate_float_type<float>;
            m_evaluation_map[PDO_CASSANDRA_TYPE_DOUBLE]         = evaluate_float_type<double>;
            m_evaluation_map[PDO_CASSANDRA_TYPE_SET]            = 0;
            m_evaluation_map[PDO_CASSANDRA_TYPE_MAP]            = 0;
            m_evaluation_map[PDO_CASSANDRA_TYPE_LIST]           = 0;
            m_evaluation_map[PDO_CASSANDRA_TYPE_UNKNOWN]        = 0;
            init = true;
        }
        return m_evaluation_map[type];
    }


    /**
     * Allocates a zval, and reads a value according to the specified type
     */
    zval* extract_zval(const unsigned char *binary, pdo_cassandra_type type, int size) {
        if (!size) {
            // We create a null zvalue in return
            zval *ret;
            MAKE_STD_ZVAL(ret);
            ret->type = IS_NULL;
            ret->value.lval = 0;
            return ret;
        }
        EvaluatorType pe = evaluate(type);
        if (!pe) {
            std::cout << "Zval extraction for type: " << (int)type << " not handled yet" << std::endl;
            //TODO raise exception
        }
        return (*pe)(binary, size);
    }

};

template <class T>
T pdo_cassandra_marshal_numeric(pdo_stmt_t *stmt, const std::string &binary)
{
    if (sizeof(T) != binary.size()) {
        // Binary is null
        if (!binary.size())
            return T();
        pdo_cassandra_error(stmt->dbh, PDO_CASSANDRA_INTEGER_CONVERSION_ERROR,
                            "pdo_cassandra_marshal_numeric: Binary stream and receiver size doesn't match", "");
        return T();
    }
    return StreamExtraction::extract<T>(reinterpret_cast <const unsigned char *>(binary.c_str()));
}


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

    stmt->columns[colno].param_type = PDO_PARAM_ZVAL;

    stmt->columns[colno].precision  = 0;
    stmt->columns[colno].maxlen     = -1;
    return 1;
}
/* }}} */

zval* parse_collection(const std::string &type, const std::string &data) {

    // Extract Collection and data type
    std::vector<pdo_cassandra_type> elt_types = pdo_cassandra_get_element_type_in_collection(PDO_CASSANDRA_TYPE_LIST, type);
    //  assert((elt_types.size() == 1) && "parse_collection elt_types.size() == 1");
    unsigned short nbElements = StreamExtraction::extract<unsigned short>(reinterpret_cast <const unsigned char *>(data.c_str()));

    // ZVAl initialisation
    zval *collection;
    MAKE_STD_ZVAL(collection);
    array_init(collection);

    // Iterating trough the collection
    const unsigned char *datap = reinterpret_cast<const unsigned char *>(data.c_str() + 2);
    while (nbElements--)
        {
            unsigned short elem_size = StreamExtraction::extract<unsigned short>(datap);
            datap += 2;
            zval *zv = StreamExtraction::extract_zval(datap, elt_types[0], elem_size);
            add_next_index_zval(collection, zv);
            datap += elem_size;
        }
    return collection;
}

zval* parse_collection_map(const std::string &type, const std::string &data) {

    // Extract Collection and data type
    std::vector<pdo_cassandra_type> elt_types = pdo_cassandra_get_element_type_in_collection(PDO_CASSANDRA_TYPE_MAP, type);
    unsigned short nbElements = StreamExtraction::extract<unsigned short>(reinterpret_cast <const unsigned char *>(data.c_str()));

    // ZVAl initialisation
    zval *collection;
    MAKE_STD_ZVAL(collection);
    array_init(collection);
    // Iterating trough the collection
    const unsigned char *datap = reinterpret_cast<const unsigned char *>(data.c_str() + 2);
    while (nbElements--)
        {
            // Reading key size
            unsigned short key_size = StreamExtraction::extract<unsigned short>(datap);
            datap += 2;

            // Extracting value
            const unsigned char *valuep = datap + key_size;
            unsigned short value_size = StreamExtraction::extract<unsigned short>(valuep);
            valuep += 2;
            zval *value = StreamExtraction::extract_zval(valuep, elt_types[1], value_size);

            // Extracting key and pushing the zval in the collection
            if (elt_types[0] == PDO_CASSANDRA_TYPE_ASCII ||
                elt_types[0] == PDO_CASSANDRA_TYPE_UTF8) {
                // String key case
                char *str = (char *) emalloc(sizeof(*str) * (key_size + 1));
                memcpy(str, datap, key_size);
                str[key_size] = 0;
                add_assoc_zval(collection, str, value);
			}
			else if (elt_types[0] == PDO_CASSANDRA_TYPE_UUID ||
                elt_types[0] == PDO_CASSANDRA_TYPE_TIMEUUID) {

                // UUID key case, for now it gets no special treatment
                unsigned char *raw_uuid = (unsigned char *) emalloc(sizeof(*raw_uuid) * key_size);
                memcpy(raw_uuid, datap, key_size);
				char *str_uuid = StreamExtraction::raw_uuid_to_str(raw_uuid, key_size);
                add_assoc_zval(collection, str_uuid, value);
            } else {
                // Numeric keys case
                assert(key_size == sizeof(long) && "parse_collection_map key_size assert");
                long key = StreamExtraction::extract<int>(datap);
                add_index_zval(collection, key, value);
            }
            datap += value_size + 2 + key_size;
        }

    return collection;
}

/** {{{ static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
*/
static int pdo_cassandra_stmt_get_column(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
{
    pdo_cassandra_stmt *S = static_cast <pdo_cassandra_stmt *>(stmt->driver_data);
    pdo_cassandra_db_handle *H = static_cast <pdo_cassandra_db_handle *>(S->H);

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

            pdo_cassandra_type lparam_type;
            if (H->preserve_values) {
                lparam_type = PDO_CASSANDRA_TYPE_BYTES;
            } else {
                lparam_type = pdo_cassandra_get_cassandra_type(S->result.get ()->schema.value_types [current_column]);
            }

            switch (lparam_type)
                {
                case PDO_CASSANDRA_TYPE_LIST:
                case PDO_CASSANDRA_TYPE_SET:
                    {
                        // The return value of the parse collection must be the zval stuff
                        zval **ref = (zval **) emalloc(sizeof(zval));
                        *ref = parse_collection(S->result.get ()->schema.value_types [current_column], (*col_it).value);
                        *ptr = (char *)ref;
                        *len = sizeof(zval);
                        *caller_frees = 1;
                        break;
                    }

                case PDO_CASSANDRA_TYPE_MAP:
                    {
                        // The return value of the parse collection must be the zval stuff
                        zval **ref = (zval **) emalloc(sizeof(*ref));
                        *ref = parse_collection_map(S->result.get ()->schema.value_types [current_column], (*col_it).value);
                        *ptr = (char *)ref;
                        *len = sizeof(zval);
                        *caller_frees = 1;
                        break;
                    }

                default:

                    zval **ref = (zval **) emalloc(sizeof(*ref));
                    *ref = StreamExtraction::extract_zval(reinterpret_cast<const unsigned char*>((*col_it).value.c_str()),
                                                          lparam_type,
                                                          (*col_it).value.size());
                    *ptr = (char *)ref;
                    *len = sizeof(zval);
                    *caller_frees = 1;
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

    add_assoc_string(return_value,
                     "native_type",
                     const_cast <char *>(S->result.get()->schema.value_types[current_column].c_str()),
                     1);

#if 0
// This code is not fonctionnal. However I kept it because this is how this feature should be implemented
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
#endif
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
