## tests/jvm98/Makefile.am
##
## Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
## R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
## C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
## TU Wien
##
## This file is part of CACAO.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.
##
## Contact: cacao@complang.tuwien.ac.at
##
## Authors: Christian Thalinger
##
## Changes:
##
## $Id: Makefile.am 1676 2004-12-03 16:52:15Z twisti $

## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	check.diff \
	compress.diff compress.output \
	db.diff db.output \
	javac.diff javac.output \
	mpegaudio.diff mpegaudio.output \
	raytrace.diff raytrace.output

JAVA = $(top_srcdir)/src/cacao/cacao
JAVAC = jikes

checkjvm: compress db javac mpegaudio raytrace 

check compress db javac mpegaudio raytrace: force
	rm -rf $@
	cp -r ../jvm98/spec/benchmarks/_*_$@ $@
	patch -p0 < $@.diff
	(cd $@; \
	 chmod 644 *; \
	 chmod 755 input; \
	 JAVA="$(JAVA)" JAVAC="$(JAVAC)" sh setup.sh || true; \
	 $(JAVAC) -d . *.java && \
	 $(JAVA) Main > $@.output; \
	 JAVA="$(JAVA)" JAVAC="$(JAVAC)" sh postoutput.sh || true; \
	)
	diff --brief $@/$@.output $@.output
	rm -rf $@

force:


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
