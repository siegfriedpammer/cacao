#!/usr/bin/env python
# src/vm/jit/compiler2/instruction_table.csv - instruction table
#
# Copyright (C) 2013
# CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
#
# This file is part of CACAO.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
#

##
# This file contains the generator for the instruction table
##

import sys
import argparse
import re
import string

cpp_header = """/* src/vm/jit/compiler2/{filename} - {file_desc}

   Copyright (C) 2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

"""

cpp_generation_disclaimer = """/*
WARNING: THIS FILE IS AUTO-GENERATED! DO NOT ALTER!
Instead have a look at the generator ({generator})
and the input file ({input_file}).
*/

"""

cpp_footer = """
/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
"""

class Formatter(string.Formatter):
	def convert_field(self, value, conversion):
		if 'U' == conversion:
			return value.upper()
		elif 'l' == conversion:
			return value.lower()
		else:
			return super(Formatter, self).convert_field(value, conversion)

class Template:
	def __init__(self,filename,file_desc, line, header='',footer=''):
		self.file = open(filename,'w')
		self.filename = filename
		self.line = line
		self.file_desc = file_desc
		self.header = header
		self.footer = footer
	def print_header(self, entry):
		try:
			self.file.write(Formatter().format(self.header, filename = self.filename, file_desc = self.file_desc, **entry))
		except KeyError as e:
			print 'Unknown template key: ', e
			sys.exit(1)

	def print_line(self, entry):
		try:
			self.file.write(Formatter().format(self.line, filename = self.filename, file_desc = self.file_desc, **entry))
		except KeyError as e:
			print 'Unknown template key: ', e
			sys.exit(1)

	def print_footer(self, entry):
		try:
			self.file.write(Formatter().format(self.footer, filename = self.filename, file_desc = self.file_desc, **entry))
		except KeyError as e:
			print 'Unknown template key: ', e
			sys.exit(1)

def main(temps):
	default_file = './instruction_table.csv'
	comment_pattern = re.compile(r'^[^#]*')
	#entry_pattern = re.compile(r'(?P<name>\w+),(?P<desc>"\w+")')
	entry_pattern = re.compile(r'(?P<name>\w+)')

	# command line arguments
	parser = argparse.ArgumentParser(description='Generate instruction table includes.')
	parser.add_argument('table', nargs='?', type=file,
	  help='instruction table file [default='+default_file+']', default=default_file)
	try:
		args = parser.parse_args()
	except IOError as e:
		print str(e)
		sys.exit(1)

	keys = {
	  'generator' : sys.argv[0],
	  'input_file': args.table.name,
	}

	# print headers
	for t in temps:
		t.print_header(keys)
	# iterate entries
	line_nr = 0
	for line in args.table:
		line_nr += 1
		# clean lines
		sline = comment_pattern.match(line).group(0)
		sline = sline.strip()
		if not sline:
		  continue
		entry = entry_pattern.match(sline)
		if not entry:
			print 'Error in line: ' + str(line_nr) + ' :' + line.strip()
			continue

		entry = entry.groupdict()
		# print line
		for t in temps:
			t.print_line(dict(keys.items() + entry.items()))

	# print footer
	for t in temps:
		t.print_footer(keys)

if __name__ == '__main__':
	temps = [
		Template('InstructionDeclGen.inc','Instruction Declarations','class {name};\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
		Template('InstructionIDGen.inc','Instruction IDs','{name}ID,\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
		Template('InstructionToInstGen.inc','Instruction conversion methods',
			'virtual {name}* to_{name}() {{ return NULL; }}\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
		Template('InstructionNameSwitchGen.inc','Instruction name switch',
			'case {name}ID: return "{name}";\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
		Template('InstructionVisitorGen.inc','Instruction Visitor',
			'virtual void visit({name}* I);\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
		Template('InstructionVisitorImplGen.inc','Instruction Visitor',
			'void InstructionVisitor::visit({name}* I) {{visit_default(I);}}\n',
			cpp_header+cpp_generation_disclaimer,cpp_footer),
	]
	main(temps)


#
# These are local overrides for various environment variables in Emacs.
# Please do not remove this and leave it at the end of the file, where
# Emacs will automagically detect them.
# ---------------------------------------------------------------------
# Local variables:
# mode: python
# indent-tabs-mode: t
# c-basic-offset: 4
# tab-width: 4
# End:
# vim:noexpandtab:sw=4:ts=4:
#
