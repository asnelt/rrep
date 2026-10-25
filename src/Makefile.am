# Process this file with automake to create Makefile.in
#
# Copyright 2011, 2019, 2026 Arno Onken <asnelt@asnelt.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
bin_PROGRAMS = rrep
rrep_SOURCES = rrep.c messages.c bufferio.c pattern.c
noinst_HEADERS = bufferio.h messages.h pattern.h rrep.h

AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -I$(top_builddir)/lib \
  -I$(top_srcdir)/lib
LDADD = ../lib/libgnu.a $(CLOCK_TIME_LIB) $(FILE_HAS_ACL_LIB) \
  $(GETLOCALENAME_L_LIB) $(GETRANDOM_LIB) $(HARD_LOCALE_LIB) $(LIBPTHREAD) \
  $(LIBTHREAD) $(LIB_ACL) $(LIBC32CONV) $(LIBINTL) $(LIBUNISTRING) \
  $(MBRTOWC_LIB) $(PTHREAD_ONCE_LIB) $(QCOPY_ACL_LIB) $(SETLOCALE_LIB) \
  $(SETLOCALE_NULL_LIB) @INTL_MACOSX_LIBS@
