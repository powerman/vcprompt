#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([vcprompt], [1.1], [vcprompt-devel@googlegroups.com])
AC_CONFIG_SRCDIR([src/fossil.h])
AC_CONFIG_HEADERS([config.h])

# Extra options
AC_ARG_WITH([sqlite3],
            AS_HELP_STRING([--with-sqlite3=PREFIX],
                           [use sqlite3 in PREFIX (for svn >= 1.7)]),
	    [],
	    [with_sqlite3=check])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_MAKE_SET

# crank up the warnings if building with GCC
if echo $CC | grep -q gcc; then
    CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter"
fi

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stdlib.h string.h sys/time.h unistd.h])

# Checks for third-party libraries.
if test "$with_sqlite3" = "check" -o "$with_sqlite3" = "yes"; then
    AC_CHECK_HEADERS([sqlite3.h])
    AC_CHECK_LIB(sqlite3, sqlite3_open_v2)
elif test "$with_sqlite3" != "no"; then
    CPPFLAGS="$CPPFLAGS -I${with_sqlite3}/include"
    LDFLAGS="$LDFLAGS -L${with_sqlite3}/lib"
    AC_CHECK_HEADERS([sqlite3.h])
    AC_CHECK_LIB(sqlite3, sqlite3_open_v2)
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 select strchr strdup strerror strstr strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
