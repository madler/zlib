#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([minizip], [@ZLIB_VER@], [bugzilla.redhat.com])
AC_CONFIG_SRCDIR([minigzip.c])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

AC_SUBST([HAVE_UNISTD_H], [0])
AC_CHECK_HEADER([unistd.h], [HAVE_UNISTD_H=1], [])
AC_CONFIG_FILES([Makefile minizip.pc])
AC_OUTPUT
