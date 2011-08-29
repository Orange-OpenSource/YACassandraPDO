PHP_ARG_WITH(pdo-cassandra,     whether to enable PDO cassandra support,
[  --with-pdo-cassandra[=FILE]  Enable PDO cassandra support. FILE is optional path to thrift interface file.], yes)

PHP_ARG_WITH(thrift-dir,    optional thrift install prefix,
[  --with-thrift-dir[=DIR]  Optional path to thrift installation.], no, no)

PHP_ARG_WITH(boost-dir,    optional boost install prefix,
[  --with-boost-dir[=DIR]  Optional path to boost installation.], no, no)

if test "x${PHP_PDO_CASSANDRA}" != "xno"; then
  
  PHP_REQUIRE_CXX()
  
  if test "x${PHP_PDO}" = "xno" && test "x${ext_shared}" = "xno"; then
    AC_MSG_ERROR([PDO is not enabled. Add --enable-pdo to your configure line.])
  fi

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "x${PKG_CONFIG}" = "xno"; then
    AC_MSG_RESULT([pkg-config not found])
    AC_MSG_ERROR([Please reinstall the pkg-config distribution])
  fi

  ORIG_PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"

  AC_MSG_CHECKING(thrift installation)
  if test "x${PHP_THRIFT_DIR}" = "xyes" -o "x${PHP_THRIFT_DIR}" = "xno"; then
    if test "x${PKG_CONFIG_PATH}" != "x"; then
      export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:/usr/local/lib/pkgconfig:/opt/lib/pkgconfig:/opt/local/lib/pkgconfig"
    else
      export PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/local/lib/pkgconfig:/opt/lib/pkgconfig:/opt/local/lib/pkgconfig"
    fi
  else
    export PKG_CONFIG_PATH="${PHP_THRIFT_DIR}:${PHP_THRIFT_DIR}/lib/pkgconfig"
  fi
  
  if ${PKG_CONFIG} --exists thrift; then
    PHP_THRIFT_VERSION=`${PKG_CONFIG} thrift --modversion`

    AC_MSG_RESULT([found version ${PHP_THRIFT_VERSION}])
    PHP_THRIFT_LIBS=`${PKG_CONFIG} thrift --libs`
    PHP_THRIFT_INCS=`${PKG_CONFIG} thrift --cflags`

    PHP_EVAL_LIBLINE(${PHP_THRIFT_LIBS}, PDO_CASSANDRA_SHARED_LIBADD)
    PHP_EVAL_INCLINE(${PHP_THRIFT_INCS})
  else
    AC_MSG_ERROR(Unable to find thrift installation)
  fi
  
  THRIFT_BIN=`${PKG_CONFIG} thrift --variable=prefix`"/bin/thrift"

  if test ! -x "${THRIFT_BIN}"; then
    AC_MSG_ERROR([${THRIFT_BIN} does not exist or is not executable])
  fi
  
  if test "x${PHP_PDO_CASSANDRA}" = "xyes" -o "x${PHP_PDO_CASSANDRA}" = "xno"; then
    INTERFACE_FILE="interface/cassandra.thrift"
  else
    INTERFACE_FILE="${PHP_PDO_CASSANDRA}"
  fi

  # Regenerate the cpp
  "${THRIFT_BIN}" -o . -gen cpp "${INTERFACE_FILE}"
  if test $? != 0; then
    AC_MSG_ERROR([failed to regenerate thrift interfaces])
  fi
  
  # Add boost includes
  AC_MSG_CHECKING([boost installation])
  if test "x${PHP_BOOST_DIR}" != "xyes" -a "x${PHP_BOOST_DIR}" != "xno"; then
    if test ! -r ${PHP_BOOST_DIR}/include/boost/shared_ptr.hpp; then
      AC_MSG_ERROR([${PHP_BOOST_DIR}/include/boost/shared_ptr.hpp not found])
    fi
  else
    for dir in /usr /usr/local /opt/local; do
      test -r "${dir}/include/boost/shared_ptr.hpp" && PHP_BOOST_DIR="${dir}" && break
    done
    if test "x${PHP_BOOST_DIR}" = "x"; then
      AC_MSG_ERROR([boost installation not found])
    fi
  fi
  AC_MSG_RESULT([found in ${PHP_BOOST_DIR}])
  PHP_ADD_INCLUDE(${PHP_BOOST_DIR}/include)
  
  PHP_ADD_EXTENSION_DEP(pdo_cassandra, pdo)
  PHP_ADD_EXTENSION_DEP(pdo_cassandra, pcre)
  
  PHP_ADD_LIBRARY(stdc++, PDO_CASSANDRA_SHARED_LIBADD)
  PHP_SUBST(PDO_CASSANDRA_SHARED_LIBADD)
  PHP_NEW_EXTENSION(pdo_cassandra, cassandra_driver.cpp cassandra_statement.cpp gen-cpp/Cassandra.cpp gen-cpp/cassandra_types.cpp, $ext_shared,,-Wall -Wno-write-strings)
fi

