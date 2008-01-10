## doc/Makefile.am
##
## Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
## C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
## E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
## J. Wenninger, Institut f. Computersprachen - TU Wien
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
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.
##
## Contact: cacao@cacaojvm.org
##
## Authors: Christian Thalinger
##
## Changes:

## Process this file with automake to produce Makefile.in

SUBDIRS = handbook

dist_noinst_DATA = assertions.tex annotations.tex jsr.bib

EXTRA_DIST = inlining_stacktrace.txt native_threads.txt stack.txt

CLEANFILES = \
	annotations.aux \
	annotations.bbl \
	annotations.blg \
	annotations.dvi \
	annotations.lof \
	annotations.log \
	annotations.lot \
	annotations.toc \
	annotations.idx \
	annotations.out \
	annotations.tex~ \
	assertions.aux \
	assertions.dvi \
	assertions.log \
	assertions.toc

annotations:
	latex annotations
	bibtex annotations
	latex annotations
	latex annotations

assertions:
	latex assertions
	latex assertions

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
