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

#ifndef _PHP_PDO_CASSANDRA_H_
# define _PHP_PDO_CASSANDRA_H_

#define PHP_PDO_CASSANDRA_EXTNAME "pdo_cassandra"
#define PHP_PDO_CASSANDRA_EXTVER "0.4.4"

#ifdef __cplusplus
extern "C" {
#ifdef ZTS
# include "TSRM.h"
#endif

#include "php.h"
}
#endif

extern zend_module_entry pdo_cassandra_module_entry;
#define phpext_pdo_cassandra_ptr &pdo_cassandra_module_entry

#endif /* _PHP_PDO_CASSANDRA_H_ */
