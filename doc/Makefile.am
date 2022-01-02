# Process this file with automake to create Makefile.in
#
# Copyright 2011 Arno Onken <asnelt@asnelt.org>
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
info_TEXINFOS = rrep.texi
rrep_TEXINFOS = fdl.texi

man_MANS = rrep.1

EXTRA_DIST = rrep.in.1
CLEANFILES = rrep.1

rrep.1: rrep.in.1
	$(AM_V_GEN)rm -f $@ $@-t
	$(AM_V_at)sed 's/@''VERSION@/$(VERSION)/' \
	  < $(srcdir)/rrep.in.1 > $@-t && chmod 444 $@-t && mv $@-t $@
