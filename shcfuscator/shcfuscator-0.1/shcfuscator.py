#!/usr/bin/python

#
# shcfuscator.py, Assembly (shellcode) obfuscator
# - Itzik Kotler <ik@ikotler.org>
#

import sys
import random
import string
import optparse

#

g_comment_char = '#'
g_regs_32 = [ "%eax" , "%ebx", "%ecx", "%edx", "%esi", "%edi", "%ebp" ]
g_regs_locked = [] 
g_verbose = False
g_zerodups = False
g_maxdepth = 0
g_obfuscating_hash = {}
g_obfuscating_stack = []
g_instructions_db = {}

#

def push_obfuscator(fcn):
	"""
	Push obfuscator to the calling stack (check for dups, maxdepth)
	"""

	global g_maxdepth
	global g_obfuscating_hash
	global g_obfuscating_stack
	global g_zerodups

	skip = False
	abort = False

	if (g_zerodups == True):
		if (str(fcn).find("default") == -1 and g_obfuscating_hash.has_key(fcn) == True):
			skip = True
		else:
			g_obfuscating_hash[fcn] = True
		
	g_obfuscating_stack.append(fcn)

	if (len(g_obfuscating_stack) >= int(g_maxdepth)):
		abort = True

	return (skip, abort)

def pop_obfuscator():
	"""
	Pop obfuscator from the calling stack
	"""

	global g_zerodups
	global g_obfuscating_hash
	global g_obfuscating_stack

	g_obfuscating_stack.pop()

	return True

def zero_obfuscating_stack():
	"""
	Reset the obfuscating stack 
	"""

	global g_zerodups
	global g_obfuscating_hash
	global g_obfuscating_stack

	if (g_zerodups == True):
		g_obfuscating_hash.clear()

	g_obfuscating_stack = []

	return True

#

def r8_to_r16(reg):
	"""
	Return the 16bit register for a 8bit register
	"""

	return reg[:2] + 'x'

def r8_to_r32(reg):
	"""
	Return the 32bit register for a 8bit register
	"""

	return "%e" + r8_to_r16(reg)[1:]

def r16_to_r8(reg):
	"""
	Return the 8bit registers off a 16bit register
	"""

	return [ reg[:2] + 'l' , reg[:2] + 'h' ]

def r16_to_32(reg):
	"""
	Return the 32bit reigster off a 16bit register
	"""

	return r8_to_r32(r16_to_r8(reg)[0])

def r32_to_r8(reg):
	"""
	Return the 8bit registers off a 32bit register
	"""

	return r16_to_r8(r32_to_r16(reg))

def r32_to_r16(reg):
	"""
	Return the 16bit register off a 32bit register
	"""

	return "%" + reg[2:]

def get_rand_r32():
	"""
	Shuffles the available registers (32bit) array and returns a random register
	"""

	retval = None

	global g_regs_32

	try:

		random.shuffle(g_regs_32)

		retval = random.choice(g_regs_32)

		if (g_verbose):
			print "%%% random register = " + retval

	except Exception, e:
		pass

	return retval

def lock_reg(reg):
	"""
	Lock a register for an exclusive access (remove from available pool)
	"""

	global g_regs_32
	global g_regs_locked

	retval = False

	try:

		g_regs_locked.append(reg)

		g_regs_32.remove(reg)

		retval = True

	except Exception, e:
		pass

	return retval

def unlock_reg(reg):
	"""
	Unlock a register (re-insert to available pool)
	"""

	global g_regs_32
	global g_regs_locked

	retval = False

	try:

		g_regs_locked.remove(reg)

		g_regs_32.append(reg)

		retval = True

	except Exception, e:
		pass

	return retval

def available_regs():
	"""
	Return the number of available registers
	"""

	global g_regs_32

	return len(g_regs_32)

#

def is_reg(operand):
	"""
	Is given operand is a register? [e.g. %eax]
	"""

	if (operand[0] == '%'):
		return True

	return False

def is_imm(operand):
	"""
	Is given operand is an immediate? [e.g. $0xdeadbeef]
	"""

	if (operand[0] == '$'):
		return True

	return False

def is_pharse(operand):
	"""
	Is given operand is a phrase? [e.g. (%esi,%ecx)]
	"""

	if (operand[0] == '('):
		return True

	return False

def is_displ(operand):
	"""
	Is given operand is a displacement? [e.g. -4(%ebp)]
	"""

	if (operand[0] != '(' and operand.find('(') != -1):
		return True

	return False

def get_imm_value(operand):
	"""
	Return the immediate operand value
	"""

	imm = operand[1:]
	val = None
	base = 10

	if (imm.startswith('0x')):
		base = 16
	
	try:

		val = string.atoi(imm, base)
	
	except Exception, e:
		
		pass
	
	return val

def sizeof_imm(operand):
	"""
	Return the size of immediate operand (8,16,32 bits)
	"""

	val = get_imm_value(operand)

	if (val == None):
		return val

	if (val < 0):
		val = val * -1
	
	if (val < 256):
		return 8

	if (val < 655357):
		return 16

	return 32

def sizeof_reg(operand):
	"""
	Return the size of a register
	"""

	if (len(operand) == 4):
		return 32

	if (operand[-1] == 'l' or operand[-1] == 'h'):
		return 8

	return 16

def sizeof_operand(operand):
	"""
	Return the size of an operand
	"""

	if (is_reg(operand)):
		return sizeof_reg(operand)

	if (is_imm(operand)):
		return sizeof_imm(operand)

	return 32	

def sizeof_inst(inst):
	"""
	Return the instruction postfix size
	"""

	val = 32

	if (inst[:1] == 'w'):
		val = 16
	elif (inst[:1] == 'b'):
		val = 8

	return val

def strip_inst(inst):
	"""
	Strip instruction from it's postfix character
	"""

	if (inst[-1:] == 'w' or inst[-1:] == 'l' or inst[-1:] == 'b'):
		return inst[:-1]

	return inst

def typeof_operand(operand):
	"""
	Return operand type (Debugging)
	"""

	if (is_reg(operand)):
		return "Register [size: " + str(sizeof_reg(operand)) + "]" # %eax

	if (is_imm(operand)):
		return "Immediate [size: " + str(sizeof_imm(operand)) + "]" # $0xdeadbeef
	
	if (is_displ(operand)):
		return "Displacement" # 4(%ebp)

	if (is_pharse(operand)): # (%ebp,%esi)
		return "Pharse"

	return "Unknown"

#

def out_comment(comment):
	"""
	Format an assembly comment
	"""

	return g_comment_char + " " + comment + '\n'

def out_operands(operands, comment=None):
	"""
	Format operands to an instruction
	"""

	buf = operands[0] + '\t'

	if (len(operands) > 1):

		buf += string.replace(string.join(operands[1:]), " ", ", ")

	if (comment):

		buf += '\t' + out_comment(comment)
	else:
		buf += '\n'

	return buf

def out_inst_size(inst_size=32):
	"""
	Format a size postfix character to an instruction
	"""

	postfix_char = 'l'

	if (inst_size == 16):
		postfix_char = 'w'
	elif (inst_size == 8):
		postfix_char = 'b'
		
	return postfix_char

def out_imm_value(value, new_size=32):
	"""
	Format immediate value to new size
	"""

	bitmask = 0xFFFFFFFF

	if (new_size == 8):
		bitmask = 0x000000FF

	if (new_size == 16):
		bitmask = 0x0000FFFF

	value &= bitmask

	buf = "$" + hex(value)

	if (buf[-1:] == 'L'):
		buf = buf[:-1]

	return buf

def out_inst(inst, inst_size=32):
	"""
	Format instruction with specified size
	"""
	
	return inst + out_inst_size(inst_size)

#

def imp_default(operands):
	"""
	Default implemementation
	"""

	return out_operands(operands)

def imp_push_alef(operands):
	"""
	Implements PUSH as 

	SUB $0x4, %ESP
	MOV <SOURCE>, (%ESP)

	[or]

	PUSH %ESP
	XOR  %ESP, (%ESP)
	MOV  <SOURCE>, (%ESP)
	"""

	if (is_pharse(operands[1]) == True or is_displ(operands[1]) == True):
		return None

	if (is_reg(operands[1]) == True and operands[1] == "%esp"):
		return None

	src_size = sizeof_operand(operands[1])

	if (src_size == 32):

		# No padding needed

		return gen_alternative_chunk([
			[ "sub" , "$0x4" , "%esp" ],
			[ "mov" , operands[1] , "(%esp)" ]
		])

	# Padding for < 32

	return gen_alternative_chunk([
		[ "push" , "%esp" ],
		[ "xor" , "%esp" , "(%esp)" ],
		[ out_inst("mov", src_size) , operands[1] , "(%esp)" ]
	])

def imp_push_bet(operands):
	"""
	Implements PUSH as

	PUSH <RANDOM_REG32>
	PUSH <RANDOM_REG32>
	XOR <SOURCE>, (%ESP)
	XOR <RANDOM_REG32>, (%ESP)
	POP <RANDOM_REG32>
	XOR (%ESP), <RANDOM_REG32>
	XOR <RANDOM_REG32>, (%ESP)
	XOR (%ESP), <RANDOM_REG32>
	"""	

	retval = None

	if (is_pharse(operands[1]) == True or is_displ(operands[1]) == True):
		return None
	
	r1 = get_rand_r32()
	
	if (r1 == None):
		return None

	lock_reg(r1)

	retval = gen_alternative_chunk([
		[ "push" , r1 ],
		[ "push" , r1 ],
		[ out_inst("xor", sizeof_operand(operands[1])) , operands[1] , "(%esp)" ],
		[ "xor" , r1 , "(%esp)" ],
		[ "pop" , r1 ],
		[ "xor" , "(%esp)", r1 ],
		[ "xor" , r1, "(%esp)" ],
		[ "xor" , "(%esp)", r1 ]
	])
	
	unlock_reg(r1)

	return retval

def imp_pop_alef(operands):
	"""
	Implements POP as

	MOV (%ESP), <DESTINATION>
	ADD $0x4, %ESP
	"""
	
	if (sizeof_operand(operands[1]) != 32):
		return None
	
	if (is_pharse(operands[1]) == True or is_displ(operands[1]) == True):
		return None

	return gen_alternative_chunk([
		[ "mov" , "(%esp)" , operands[1] ],
		[ "add" , "$0x4" , "%esp" ]
	])

def imp_pop_bet(operands):
	"""
	Implements POP as

	XOR <DESTINATION>, (%ESP)
	XOR (%ESP), <DESTINATION>
	ADD $0x4, %ESP
	"""

	if (sizeof_operand(operands[1]) != 32):
		return None
	
	if (is_pharse(operands[1]) == True or is_displ(operands[1]) == True):
		return None

	return gen_alternative_chunk([
		[ "xor" , operands[1] , "(%esp)" ],
		[ "xor" , "(%esp)" , operands[1] ],
		[ "add" , "$0x4" , "%esp" ]
	])
	
def imp_mov_alef(operands):
	"""
	Implements MOV as

	PUSH <SOURCE>
	POP  <DESTINATION>
	"""

	if (sizeof_operand(operands[2]) != 32):
		return None

	return gen_alternative_chunk([
		[ "push" , operands[1] ],
		[ "pop"  , operands[2] ]
	])
	
def imp_mov_bet(operands):
	"""
	Implements MOV as

	PUSH <SOURCE>
	XCHG <SOURCE>, <DESTINATION>
	POP  <SOURCE>
	"""

	if (is_reg(operands[1]) == False or is_reg(operands[2]) == False):
		return None

	if (sizeof_operand(operands[1]) != sizeof_operand(operands[2])):
		return None

	if (operands[1] == "%esp" or operands[2] == "%esp"):
		return None

	return gen_alternative_chunk([
		[ "push" , operands[1] ],
		[ "xchg" , operands[1] , operands[2] ],
		[ "pop"  , operands[1] ]
	])

def imp_inc_alef(operands):
	"""
	Implements INC as

	ADD $0x1, <DESTINATION>
	"""

	return gen_alternative_inst(
		[ "add" , "$0x1" , operands[1] ]
	)

def imp_inc_bet(operands):
	"""
	Implements INC as
	
	SUB $0xFF[FF][FFFF], <DESTINATION>
	"""
	
	return gen_alternative_inst(
		[ "sub" , out_imm_value(0xffffffff, sizeof_operand(operands[1])) , operands[1] ]
	)

def imp_sub_alef(operands):
	"""
	Implements SUB as

	DEC <DESTINATION> multiplied by <SOURCE>
	"""

	if (is_imm(operands[1]) != True):
		return None
	
	return gen_repetitive_inst(
		[ "dec" , operands[2] ], get_imm_value(operands[1])
	)

def imp_dec_alef(operands):
	"""
	Implements DEC as

	SUB $0x1, <DESTINATION>
	"""

	return gen_alternative_inst(
		[ "sub" , "$0x1" , operands[1] ]
	)

def imp_dec_bet(operands):
	"""
	Implements DEC as

	ADD $0xFF/FFFF/FFFFFFFF, <DESTINATION>
	"""

	return gen_alternative_inst(
		[ "add" , out_imm_value(0xffffffff, sizeof_operand(operands[1])) , operands[1] ]
	)

def imp_xchg_alef(operands):
	"""
	Implements XCHG as
	
	PUSH <SOURCE>
	PUSH <DESTINATION>
	POP  <SOURCE>
	POP  <DESTINATION>
	"""

	if (is_reg(operands[1]) == False or is_reg(operands[2]) == False):
		return None

	if (sizeof_operand(operands[1]) != sizeof_operand(operands[2])):
		return None

	return gen_alternative_chunk([
		[ "push" , operands[1] ], 
		[ "push" , operands[2] ],
		[ "pop"  , operands[1] ], 
		[ "pop"  , operands[2] ]
	])
	
def imp_xchg_bet(operands):
	"""
	Implements XCHG as

	PUSH <SOURCE>
	MOV <SOURCE>, <DESTINATION>
	POP <DESTINATION>
	"""

	if (is_reg(operands[1]) == False or is_reg(operands[2]) == False):
		return None

	if (sizeof_operand(operands[1]) != sizeof_operand(operands[2]) or sizeof_operand(operands[1]) != 32):
		return None

	return gen_alternative_chunk([
		[ "push" , operands[1] ] ,
		[ "mov"  , operands[2] , operands[1] ] ,
		[ "pop"  , operands[2] ]
	])

def imp_xchg_gimel(operands):
	"""
	Implements XCHG as

	ADD <SOURCE>, <DESTINATION>
	SUB <DESTINATION>, <SOURCE>
	NEG <SOURCE>
	SUB <SOURCE>, <DESTINATION>
	"""
	
	if (is_reg(operands[1]) == False or is_reg(operands[2])):
		return None

	if (sizeof_operand(operands[1]) != sizeof_operand(operands[2])):
		return None

	return gen_alternative_chunk([
		[ "add" , operands[1] , operands[2] ] ,
		[ "sub" , operands[2] , operands[1] ] ,
		[ "neg" , operands[1] ] ,
		[ "sub" , operands[1] , operands[2] ]
	])

def imp_xchg_dalet(operands):
	"""
	Implements XCHG as

	XOR <SOURCE>, <DESTINATION>
	XOR <DESTINATION>, <SOURCE>
	XOR <SOURCE>, <DESTINATION>
	"""

	if (is_reg(operands[1]) == False or is_reg(operands[2])):
		return None

	if (sizeof_operand(operands[1]) != sizeof_operand(operands[2])):
		return None

	return gen_alternative_chunk([
		[ "xor" , operands[1] , operands[2] ],
		[ "xor" , operands[2] , operands[1] ],
		[ "xor" , operands[1] , operands[2] ]
	])


def imp_not_alef(operands):
	"""
	Implements NOT as

	AND <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	"""

	if (is_reg(operands[1]) == False):
		return None

	return gen_alternative_chunk([
		[ "and" , operands[1] , operands[1] ],
		[ "not" , operands[1] ]
	])

def imp_not_bet(operands):
	"""
	Implements NOT as
	
	OR <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	"""

	if (is_reg(operands[1]) == False):
		return None

	return gen_alternative_chunk([
		[ "or" , operands[1] , operands[1] ],
		[ "not" , operands[1] ]
	])

def imp_and_alef(operands):
	"""
	Implements AND as

	AND <SOURCE>, <DESTINATION>
	NOT <DESTINATION>
	AND <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	"""

	return gen_alternative_chunk([
		[ "and" , operands[1] , operands[2] ],
		[ "not" , operands[2] ],
		[ "and" , operands[2] , operands[2] ],
		[ "not" , operands[2] ]
	])

def imp_and_bet(operands):
	"""
	Implements AND as

	OR <SOURCE>, <SOURCE>
	NOT <SOURCE>
	OR <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	OR <SOURCE>, <DESTINATION>
	NOT <DESTINATION>
	"""
	
	return gen_alternative_chunk([
		[ "or" , operands[1] , operands[2] ],
		[ "not" , operands[1] ],
		[ "or" , operands[2] , operands[2] ],
		[ "not" , operands[2] ],
		[ "or" , operands[1] , operands[2] ],
		[ "not" , operands[2] ]
	])
	
def imp_or_alef(operands):
	"""
	Implements OR as

	AND <SOURCE>, <SOURCE>
	NOT <SOURCE>
	AND <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	AND <SOURCE>, <DESTINATION>
	NOT <DESTINATION>
	"""

	return gen_alternative_chunk([
		[ "and" , operands[1] , operands[2] ],
		[ "not" , operands[1] ],
		[ "and" , operands[2] , operands[2] ],
		[ "not" , operands[2] ],
		[ "and" , operands[1] , operands[2] ],
		[ "not" , operands[2] ]
	])

def imp_or_bet(operands):
	"""
	Implements OR as

	OR <SOURCE>, <DESTINATION>
	NOT <DESTINATION>
	OR <DESTINATION>, <DESTINATION>
	NOT <DESTINATION>
	"""

	return gen_alternative_chunk([
		[ "or" , operands[1] , operands[2] ],
		[ "not" , operands[2] ],
		[ "or" , operands[2] , operands[2] ],
		[ "not" , operands[2] ]
	])

def imp_xor_alef(operands):
	"""
	Implements XOR as

	MOV <SOURCE>, <RAND_REG_32>
	AND <DESTINATION>, <SOURCE>
	NOT <SOURCE>
	XCHG <RAND_REG_32>, <SOURCE>
	AND <RAND_REG_32>, <SOURCE>
	NOT <SOURCE>
	AND <RAND_REG_32>, <DESTINATION>
	NOT <DESTINATION>
	AND <SOURCE>, <DESTINATION>
	NOT <DESTINATION>
	"""

	return None

def imp_bswap_alef(operands):
	"""
	Implements BSWAP as

	XCHG <SOURCE/8_LOWER>, <SOURCE/8_HIGHER>
	ROL 16, <SOURCE>
	XCHG <SOURCE/8_LOWER>, <SOURCE/8_HIGHER>	
	"""

	if (sizeof_operand(operands[1]) != 32):
		return None

	if (operands[1] == "%ebp" or operands[1] == "%esp" or operands[1] == "%edi" or operands[1] == "%edi"):
		return None

	(r1,r2) = r32_to_r8(operands[1])

	return gen_alternative_chunk([
		[ "xchg" , r1, r2 ],
		[ "rol" , "$16" , operands[1] ],
		[ "xchg" , r1 , r2 ]
	])
	
def imp_test_alef(operands):
	"""
	Implements TEST as

	PUSH <DESTINATION>
	AND  <SOURCE>, <DESTINATION>
	POP  <DESTINATION>
	"""

	return gen_alternative_chunk([
		[ "push" , operands[2] ],
		[ "and" , operands[1] , operands[2] ],
		[ "pop" , operands[2] ] 
	])

#

def gen_instructions_db():
	"""
	Compile instructions database from imp_* functions
	"""

	global g_instructions_db

	globals_dict = globals()

	for key in globals_dict.keys():

		if (key.startswith("imp_") == True and key.count("_") >= 2):
	
			fcn = globals_dict.get(key)

			inst = key[4:].split('_')[0]

			if (g_verbose):
				print "! registrying " + key + " for " + inst

			if (g_instructions_db.has_key(inst) == False):
				g_instructions_db[inst] = []
			
			g_instructions_db[inst].append(fcn)

	return True

def gen_alternative_chunk(instructions):
	"""
	Generate an alternative instructions for given code chunk
	"""

	out = ""
	retval = None

	for inst in instructions:

		retval = gen_alternative_inst(inst)

		if (retval == None):
			return None

		out += retval
	
	return out

def gen_alternative_inst(operands):
	"""
	Generate an alternative instruction for given operands
	"""

	global g_instrucions_db

	insts = g_instructions_db
	retval = None
	gen_fcn = imp_default

	# Echo back instruction

	if (insts.has_key(operands[0]) == False):

		retval = gen_fcn(operands)

	else:

		gen_fcns = list(insts[operands[0]])

		gen_fcns.append(imp_default)

	while (retval == None):

		retval = None

		random.shuffle(gen_fcns)

		gen_fcn = random.choice(gen_fcns)

		(skip, abort) = push_obfuscator(gen_fcn)

		if (skip != True):

			if (abort == True):

				if (g_verbose):
					print "! reached soft maximum depth/obfuscating / invoking imp_default()"
				
				gen_fcn = imp_default

			if (g_verbose):
				print "+++ " + str(gen_fcn)
		
			retval = gen_fcn(operands)

			if (g_verbose):
				print "--- " + str(gen_fcn)
		
		elif (g_verbose == True):

			print "! skipping over " + str(gen_fcn) + " to avoid duplicate in the chain!"
			
		pop_obfuscator()

		gen_fcns.remove(gen_fcn)

	if (g_verbose):
		print "=== " + str(gen_fcn) + "\n... " + str(operands) + " = \n... " + str(retval)[:-1].replace('\n', "\n... ")

	return retval

def gen_repetitive_inst(operands, times):
	"""
	Generate an alternative instruction for given operands (multiply by X times)
	"""

	retval = ""

	try:
		for seq in range(0, times):
			retval += gen_alternative_inst(operands)
	
	except Exception, e:
	
		retval = None

	return retval

#

def split_to_operands(line):
	"""
	Split source code line to array of operands
	"""

	operands = []

	buf = line.replace('\t', ' ')

	# Single operand or multiply operands?

	if (buf.find(' ') == -1):

		operands.append(line)

	else:

		for operand in buf.split(' '):	
			operands.append(operand.rstrip(','))

		while (operands.count('')):
			operands.remove('')

	return operands

#

def build_args_parser():
	"""
	Build args (argv) parser and usage screen
	"""

	args_parser = optparse.OptionParser()

	misc_opts = args_parser.add_option_group('Miscellaneous')

	misc_opts.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False, help="print verbose messages to stdout")

	io_opts = args_parser.add_option_group('Input/Output')

	io_opts.add_option("-i", "--input", dest="input_filename", default=None, help="read assembly instructions from FILE", metavar="FILE")

	io_opts.add_option("-o", "--output", dest="output_filename", default=None, help="write assembly instructions to FILE", metavar="FILE")	

	parser_opts = args_parser.add_option_group('Parser')

	parser_opts.add_option("-s", "--startline", dest="start_line", default=0, help="start obfuscating from LINE in the code", metavar="NUMBER")

	parser_opts.add_option("-e", "--endline", dest="end_line", default=0, help="stop obfuscating at LINE in the code", metavar="NUMBER")

	parser_opts.add_option("-c", "--comment", dest="comment_char", default="#", help="ignore lines which begins with CHAR in the code", metavar="CHAR")

	parser_opts.add_option("-l", "--label", dest="work_label", default=None, help="obfuscat code only if labeled under LABEL in the code", metavar="STRING")

	engine_opts = args_parser.add_option_group('Obfuscating')
	
	engine_opts.add_option("-x", "--maxcalls", default=sys.getrecursionlimit() / 2, dest="maxdepth", help="maximum recursion depth/obfuscating per instruction", metavar="NUMBER")

	engine_opts.add_option("-z", "--zerodups", action="store_true", dest="zerodups", default=False, help="force unique chain of obfuscators per instruction")

	return args_parser

#

def main(argc, argv):

	global g_verbose
	global g_maxdepth
	global g_zerodups

	line_num = -1

	args_parser = build_args_parser()

	(options, args) = args_parser.parse_args()

	# enough params ?

	if (argc < 2 or options.input_filename == None or options.output_filename == None):
		args_parser.print_help()
		return 0

	# adjust settings

	g_verbose = options.verbose
	g_maxdepth = options.maxdepth
	g_zerodups = options.zerodups
	g_comment_char = options.comment_char
	
	start_line = options.start_line
	target_label = options.work_label
	
	if (g_verbose):

		print "! - " + argv[0]

		for opt in str(options).split(','):
			print "! + " + str(opt)
	
	fd_in = open(options.input_filename, 'r')
	fd_out = open(options.output_filename, 'w')

	gen_instructions_db()

	buf = fd_in.read().split('\n')[:-1]

	if (g_verbose):
		print "~ " + str(len(buf)) + " lines"

	if (options.end_line == 0):
		end_line = len(buf) + 1
	else:
		end_line = options.end_line
	
	try:

		# Process source code line

		for line in buf:

			line = line.strip()

			line_num += 1

			raw_line = False

			# Process directives, comments and labels

			if (line == '' or line[0] == g_comment_char or line[0] == '.' or line[-1] == ':'):

				if (line != '' and target_label != None and line[-1] == ':'):

					current_label = line[line.rfind(' ')+1:-1]
					
					if (g_verbose):
						print "! entering label => " + current_label

				raw_line = True
			
			elif (line_num < int(start_line) or line_num > int(end_line)):

				if (g_verbose):
					print "! skipping over line #" + str(line_num) + " [lines ; " + str(start_line) + "..." + str(end_line) + "]"

				raw_line = True

			elif (target_label != None and target_label != current_label):
			
				if (g_verbose):
					print "! skipping over line #" + str(line_num) + " [labels ; " + current_label + " != " + target_label + "]"

				raw_line = True

			# Write line as it is, and move to the next one ...

			if (raw_line == True):

				fd_out.write(line + '\n')

				continue

			# Process assembly source line

			operands = split_to_operands(line)

			if (len(operands) > 1):
				operands[0] = strip_inst(operands[0])

			print "< " + line

			if (g_verbose):
				print "? " + operands[0]

			zero_obfuscating_stack()

			output = gen_alternative_inst(operands).split('\n')

			if (output.count('')):
				output.remove('')

			for output_inst in output:
				if (not g_verbose):
					print "> " + output_inst
				fd_out.write('\t' + output_inst + '\n')
	
	except RuntimeError, e:
		
		print "*** aborted, reached hard recursion depth, try passing -x with smaller value (< " + str(g_maxdepth) + ") !"
		
	fd_out.close()
	fd_in.close()

	if (g_verbose):
		print "~ eof"

	return 1

#

if (__name__ == "__main__"):
	sys.exit(main(len(sys.argv), sys.argv))
