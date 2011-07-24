dnl
dnl autoconf input file for ``rrep''
dnl
dnl Copyright (C) 2011 Arno Onken <asnelt@asnelt.org>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
dnl 02110-1301, USA.

AC_INIT([rrep], [1.3.2], [asnelt@asnelt.org])

AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR(src/rrep.c)
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_CANONICAL_HOST
AC_PROG_INSTALL
AC_PROG_CC
dnl For gnulib.
gl_EARLY
gl_INIT

AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([
 Makefile
 lib/Makefile
 src/Makefile
 po/Makefile.in
 doc/Makefile
])
AC_OUTPUT
