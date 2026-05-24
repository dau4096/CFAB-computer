"translate.py"
from typing import Callable, Optional;
import re as regex;
from fabsrc import shared;
from fabsrc.shared import FabricationError;



global SHOULD_ADD_TO_ROM, MARKERS;
SHOULD_ADD_TO_ROM:bool = False;
MARKERS:dict[str, int] = {};


OPCODES:tuple[str] = (
	"nop", "set", "mov", "add",
	"sub", "mul", "div", "not",
	"equ", "grt", "brn", "i_o",
	"shf", "ext", "slp", "mem"
); #Index is opcode number.


#### HELPER FUNCTIONS ####
def getOperatorBinary(opcode:str) -> str:
	if (opcode in OPCODES):
		return f"{OPCODES.index(opcode):04b}";
	else:
		raise FabricationError(f"Unknown opcode: {opcode}");


def to8B(V:int) -> str:
	return str(bin(V & 0xFF)[2:]).zfill(8);
def to16B(V:int) -> str:
	return str(bin(V & 0xFFFF)[2:]).zfill(16);


def convertValues(V:str, convertMarkers:bool=True) -> tuple[int, bool]:
	"Convert lineSplit to actual interger values";
	if V.startswith("r"): #Registers
		return (int(V.replace("r", "")), False);
	elif V.startswith("#x"): #Immediate values (Hex)
		vInt:int = int(V.replace("#x",""), 16);
		if (vInt > 127): vInt -= 256;
		return (vInt, True);
	elif V.startswith("#b"): #Immediate values (Binary)
		vInt:int = int(V.replace("#b",""), 2);
		if (vInt > 127): vInt -= 256;
		return (vInt, True);
	elif V.startswith("#"): #Immediate values (Denary)
		return (int(V.replace("#d", "").replace("#","")), True);
	elif V.startswith(":"): #Markers
		if (convertMarkers): return (MARKERS[V.replace(":", "").upper()], True);
		else: return (0, True); #Filler marker index, ensures binary is upheld even if not correct.
	else:
		try: return (int(V), True);
		except ValueError: return (str(V), True); #Ensure the fallback is a string.
#### HELPER_FUNCTIONS ####



#### COMPLEX OPCODES ####
def OPC_set(ln:shared.Data) -> list[str]:
	if (regex.match(r"(?i)^(?!(((#|#d|#x|#b|r)?[0-9a-f])|(\$[a-z0-9]+))+$).+", ln.preB) is not None): #Operand B is something like "r1 + #4"
		return [
			convertLine(ln.preB, makeHex=False)[0], #The calculation
			f"0000{getOperatorBinary('set')}{ln.A}{shared.REG_RESULT}" #"SET rA rOP"
		];
	else:
		ln.B = shared.toBin(convertValues(ln.preB, convertMarkers=SHOULD_ADD_TO_ROM)[0]); #Convert B only NOW.
		print(ln.immediates, ln.A, ln.B)
		return [f"{ln.immediates}00{getOperatorBinary('set')}{ln.A}{ln.B}",];

def OPC_branch(ln:shared.Data) -> list[str]:
	try: instrIdx:int = to16B(ln.preA); #16-bit.
	except TypeError: instrIdx = preA;
	return [f"{ln.immediates}10{getOperatorBinary('brn')}{instrIdx}",];

def OPC_jump(ln:shared.Data) -> list[str]:
	try: instrIdx:int = to16B(ln.preA); #16-bit.
	except TypeError: instrIdx = ln.preA;
	return [f"{ln.immediates}00{getOperatorBinary('brn')}{instrIdx}",];

def OPC_sleep(ln:shared.Data) -> list[str]:
	sleepMS:int = to16B(ln.preA); #16-bit.
	return [f"{ln.immediates}00{getOperatorBinary('slp')}{sleepMS}",];
#### COMPLEX OPCODES ####




#### ABSTRACTIONS ####
def ABS_increment(ln:shared.Data) -> list[str]:
	return [
		f"{ln.immediates[0]}100{getOperatorBinary('add')}{ln.A}{shared.TRUE}",
		f"0000{getOperatorBinary('mov')}{shared.REG_RESULT}{ln.A}",
	];

def ABS_decrement(ln:shared.Data) -> list[str]:
	return [
		f"{ln.immediates[0]}100{getOperatorBinary('sub')}{ln.A}{shared.TRUE}",
		f"0000{getOperatorBinary('mov')}{shared.REG_RESULT}{ln.A}",
	];

def ABS_sign(ln:shared.Data) -> list[str]:
	return [
		f"{ln.immediates[0]}011{getOperatorBinary('not')}{ln.A}{shared.BLANK}",
		f"{ln.immediates[0]}000{getOperatorBinary('div')}{ln.A}{shared.REG_RESULT}",
	];


def ABS_if(ln:shared.Data) -> list[str]:
	condition:str = convertLine(ln.preA, makeHex=False)[0];
	try: instrIdx:int = to16B(ln.preB); #16-bit.
	except TypeError: instrIdx = ln.preB;
	
	return [
		condition,
		f"{ln.immediates}10{getOperatorBinary('brn')}{instrIdx}",
	];

def ABS_cout(ln:shared.Data) -> list[str]:
	if (ln.preA == "\\n"): #Just a newline char (0x0A)
		return [f"1111{getOperatorBinary('i_o')}{shared.NEW_LN}{shared.BLANK}",]; 

	#Has memory value to show too;
	instructions:list[str] = [f"{ln.immediates[0]}010{getOperatorBinary('i_o')}{ln.A}{shared.BLANK}",];
	if (
		(type(ln.preB) == str) and (
			("$" in ln.preB) or
			("\\n" in ln.preB)
		)
	):
		instructions.append(f"1111{getOperatorBinary('i_o')}{shared.NEW_LN}{shared.BLANK}"); #COUT << NEWLINE instruction

	return instructions;

def ABS_print(ln:shared.Data) -> list[str]:
	#Writes text to console. Uses ROM at the end of the file.
	text:str = ln.text.split('"')[1].replace('"','');
	if (not SHOULD_ADD_TO_ROM): return [f"1011{getOperatorBinary('i_o')}{shared.BLANK}{shared.BLANK}",];

	ROMidx:int = len(shared.ROM_DATA); #Add to end of index. Take last index.
	instructions:list[str] = [f"1011{getOperatorBinary('i_o')}{shared.toBin(ROMidx)}{shared.BLANK}",]

	text = text.replace("\\n", "\n"); #Replace with actual 0x0A newline chars;
	shared.ROM_DATA.append([ord(char) for char in text]);

	return instructions;

def ABS_load(ln:shared.Data) -> list[str]:
	#Example: [LOAD #0 #2]
	#Load ROM segment 0 into RAM [2:]
	try: ramAddress:int = f"{(ln.preB & 0xFFFF):16b}"; #16-bit.
	except TypeError: ramAddress = ln.preB;
	return [
		f"0{ln.immediates[0]}00{getOperatorBinary('set')}{shared.REG_X}{ln.A}",  #Use "A" as the ROM segment.
		f"1110{getOperatorBinary('mem')}{ramAddress}", #Write to this ram address.
	];
	
def ABS_copy(ln:shared.Data) -> list[str]:
	#Example: [COPY #2 #10 #11]
	#Copy RAM [2:10] to [11:]
	lineSplit:list[str] = ln.text.split(" "); #Get lineSplit manually - it has 3, normally no support for this.
	preC:str = convertValues(lineSplit[3])[0];

	try: srcStart:int = to16B(ln.preA); #16-bit.
	except TypeError: srcStart = ln.preA;

	try: srcEnd:int = to16B(ln.preB); #16-bit.
	except TypeError: srcEnd = ln.preB;

	try: destination:int = to16B(preC); #16-bit.
	except TypeError: destination = preC;

	return [
		f"0100{getOperatorBinary('set')}{shared.REG_X}{srcEnd[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Y}{srcEnd[8:]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Z}{destination[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_W}{destination[8:]}",
		f"1101{getOperatorBinary('mem')}{srcStart}"
	];
	
def ABS_RAMclear(ln:shared.Data) -> list[str]:
	#Example: [RAMCLEAR #2 #10 #1]
	#Clear values in RAM [2:10] to 1
	lineSplit:list[str] = ln.text.split(" "); #Get lineSplit manually - it has 3, normally no support for this.
	preC:str = convertValues(lineSplit[3])[0];

	try: srcStart:int = to16B(ln.preA); #16-bit.
	except TypeError: srcStart = ln.preA;

	try: srcEnd:int = to16B(ln.preB); #16-bit.
	except TypeError: srcEnd = ln.preB;

	return [
		f"0100{getOperatorBinary('set')}{shared.REG_X}{shared.toBin(preC)}",
		f"0100{getOperatorBinary('set')}{shared.REG_Y}{srcEnd[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Z}{srcEnd[8:]}",
		f"1101{getOperatorBinary('mem')}{srcStart}"
	];
#### ABSTRACTIONS ####



#Inline operators mapped to their actual opcode names.
INFIX_OPERATORS_MAP:dict[str, str] = {
	"+":  "add",
	"-":  "sub",
	"*":  "mul",
	"/":  "div",
	"%":  "mod",
	"!":  "not",  #Logical
	"&":  "Band", #...
	"|":  "Bor",  #...
	"^":  "xor",  #...
	"!^": "xnor", #Logical
	">":  "grt",
	"<":  "lss",
	">=": "gte",
	"<=": "lse",
	"==": "equ",
	"!=": "neq",
	"=":  "set",
	"~":  "mov",
	"++": "inc", #Modifies in-place
	"--": "dec", #Modifies in-place
	">>": "rsh",
	"<<": "lsh"
};
#Maps operation mneumonics to their instruction calls.
OPERATOR_MAPPING:dict[str, shared.Operation] = {
	#0-Operand operations;
	"nop":      shared.Operation(type=shared.OperationType.ZERO_OPERANDS, func=(lambda ln : [f"0000{getOperatorBinary('nop')}{shared.BLANK}{shared.BLANK}",])), #No-Operation
	"halt":     shared.Operation(type=shared.OperationType.ZERO_OPERANDS, func=(lambda ln : [f"0000{getOperatorBinary('ext')}{shared.BLANK}{shared.BLANK}",])), #Halt/Exit
	"wait":     shared.Operation(type=shared.OperationType.ZERO_OPERANDS, func=(lambda ln : [f"0001{getOperatorBinary('slp')}{shared.BLANK}{shared.BLANK}",])), #Wait for user input
	"update":   shared.Operation(type=shared.OperationType.ZERO_OPERANDS, func=(lambda ln : [f"0010{getOperatorBinary('slp')}{shared.BLANK}{shared.BLANK}",])), #Update screen


	#1-Operand operations;
	"not":      shared.Operation(type=shared.OperationType.ONE_OPERAND, func=(lambda ln : [f"{ln.immediates[0]}000{getOperatorBinary('not')}{ln.A}{shared.BLANK}",])), #Logical Not
	"inv":      shared.Operation(type=shared.OperationType.ONE_OPERAND, func=(lambda ln : [f"{ln.immediates[0]}010{getOperatorBinary('not')}{ln.A}{shared.BLANK}",])), #Bitwise Not
	"clear":    shared.Operation(type=shared.OperationType.ONE_OPERAND, func=(lambda ln : [f"{ln.immediates[0]}001{getOperatorBinary('ext')}{ln.A}{shared.BLANK}",])), #Fill all registers with value A|*A


	#2-Operand operations;
	"mov":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('mov')}{ln.A}{ln.B}",])), #Move memory
	"add":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('add')}{ln.A}{ln.B}",])), #Addition
	"sub":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('sub')}{ln.A}{ln.B}",])), #Subtraction
	"mul":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('mul')}{ln.A}{ln.B}",])), #Multiplication
	"div":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('div')}{ln.A}{ln.B}",])), #Division
	"equ":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('equ')}{ln.A}{ln.B}",])), #Equal
	"grt":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('grt')}{ln.A}{ln.B}",])), #Greater than
	"i_o":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('i_o')}{ln.A}{ln.B}",])), #Input_Output
	"shf":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('shf')}{ln.A}{ln.B}",])), #Bitshift


	#2-Opcode operations that use other operators;
	"and":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('mul')}{ln.A}{ln.B}",])), #Logical And
	"or":       shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('add')}{ln.A}{ln.B}",])), #Logical Or
	"Band":     shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('mul')}{ln.A}{ln.B}",])), #Logical And
	"Bor":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('add')}{ln.A}{ln.B}",])), #Logical Or
	"mod":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('div')}{ln.A}{ln.B}",])), #Modulo
	"neq":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('equ')}{ln.A}{ln.B}",])), #Not equal
	"xor":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}10{getOperatorBinary('equ')}{ln.A}{ln.B}",])), #Exclusive Or
	"xnor":     shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}11{getOperatorBinary('equ')}{ln.A}{ln.B}",])), #Exclusive Not-Or
	"lss":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('grt')}{ln.A}{ln.B}",])), #Less than
	"gte":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}10{getOperatorBinary('grt')}{ln.A}{ln.B}",])), #Greater than or equal
	"lse":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}11{getOperatorBinary('grt')}{ln.A}{ln.B}",])), #Less than or equal
	"rsh":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('shf')}{ln.A}{ln.B}",])), #Rightshift
	"lsh":      shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('shf')}{ln.A}{ln.B}",])), #Leftshift
	"ramwrite": shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}10{getOperatorBinary('ext')}{ln.A}{ln.B}",])), #Write into RAM
	"ramread":  shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}11{getOperatorBinary('ext')}{ln.A}{ln.B}",])), #Read from RAM
	"input":    shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}00{getOperatorBinary('i_o')}{ln.A}{ln.B}",])), #Input
	"output":   shared.Operation(type=shared.OperationType.TWO_OPERAND, func=(lambda ln : [f"{ln.immediates}01{getOperatorBinary('i_o')}{ln.A}{ln.B}",])), #Output


	#Complex operations;
	"set":      shared.Operation(type=shared.OperationType.B_AMBIGUOUS, func=OPC_set),    #Assign memory a value, or the result of a calculation.
	"brn":      shared.Operation(type=shared.OperationType.COMPLEX,     func=OPC_branch), #Conditional branching
	"jmp":      shared.Operation(type=shared.OperationType.COMPLEX,     func=OPC_jump),   #Unconditional branching
	"sleep":    shared.Operation(type=shared.OperationType.COMPLEX,     func=OPC_sleep),  #Sleep for specified number of milliseconds


	#Complex abstractions of operations;
	"inc":      shared.Operation(type=shared.OperationType.ONE_OPERAND,   func=ABS_increment), #Increments in-place
	"dec":      shared.Operation(type=shared.OperationType.ONE_OPERAND,   func=ABS_decrement), #Decrements in-place
	"sgn":      shared.Operation(type=shared.OperationType.ONE_OPERAND,   func=ABS_sign),      #Mathematical sign of value [+/- 1]
	"if":       shared.Operation(type=shared.OperationType.ONLY_B_OPRNDS, func=ABS_if),		   #Nicer formatted conditional branching
	"cout":		shared.Operation(type=shared.OperationType.COMPLEX,       func=ABS_cout),	   #Show single value in console
	"print":	shared.Operation(type=shared.OperationType.ZERO_OPERANDS, func=ABS_print),	   #Show larger, fixed block of text in console
	"load":		shared.Operation(type=shared.OperationType.COMPLEX,       func=ABS_load),	   #Loads section of ROM into RAM
	"copy":		shared.Operation(type=shared.OperationType.COMPLEX,       func=ABS_copy),      #Copies one section of RAM into another
	"ramclear": shared.Operation(type=shared.OperationType.COMPLEX,       func=ABS_RAMclear),  #Fills RAM in range with some value
};
#All one-value operations.
UNARY_OPERATIONS:list[str] = [k for (k,v) in OPERATOR_MAPPING.items() if (v.type == shared.OperationType.ONE_OPERAND)];




#### LINE FORMATS ####
def FORM_postfix(lineSplit:list[str]) -> shared.ParsedLine:
	if ((lineSplit[0] in OPCODES) or (lineSplit[0] in OPERATOR_MAPPING.keys())):
		return shared.ParsedLine(operator=lineSplit[0], operands=tuple(lineSplit[1:]), valid=True);
	return shared.INVALID_PARSED_LINE;

def FORM_infix(lineSplit:list[str]) -> shared.ParsedLine:
	if (lineSplit[1] in INFIX_OPERATORS_MAP):
		operands:tuple[str] = [lineSplit[0],];
		operands.extend(lineSplit[2:]);
		return shared.ParsedLine(operator=INFIX_OPERATORS_MAP[lineSplit[1]], operands=operands, valid=True);

	return shared.INVALID_PARSED_LINE;

def FORM_if(lineSplit:list[str]) -> shared.ParsedLine:
	if lineSplit[0] == "if":
		operands:tuple[str] = (
			lineSplit[1],
			lineSplit[-1]
		);
		return shared.ParsedLine(operator="if", operands=operands, valid=True);
	return shared.INVALID_PARSED_LINE;

def FORM_print(lineSplit:list[str]) -> shared.ParsedLine:
	if (lineSplit[0] == "print"):
		return shared.ParsedLine(operator="print", operands=("0", "0",), valid=True);
	return shared.INVALID_PARSED_LINE;

def FORM_unary(lineSplit:list[str]) -> shared.ParsedLine:
	if (lineSplit[0] == UNARY_OPERATIONS):
		return shared.ParsedLine(operator="print", operands=("0", "0",), valid=True);
	return shared.INVALID_PARSED_LINE;

def FORM_exclaim(lineSplit:list[str]) -> shared.ParsedLine:
	if (lineSplit[0] == "!"):
		return shared.ParsedLine(INFIX_OPERATORS_MAP["!"], operands=tuple(lineSplit[1:]));
	return shared.INVALID_PARSED_LINE;

def FORM_ROMdeclaration(lineSplit:list[str], makeHex:bool) -> bool:
	"Not a format as such, but specifically defines data to be added to a ROM index. [e.g. ROM3 BEEFBADA55DEADFEED..]";
	if (regex.match(r"(?i)ROM[0-9]+", lineSplit[0]) is None): return False; #Did not match correct format for ROM MNGR lines.
	if (not makeHex): return True; #Don't mess with ROM.

	index:int = int(lineSplit[0][3:]); #ROM index to write to.
	hexData:str = "".join(c for c in "".join(lineSplit[1:]) if c in "0123456789abcdef");
	if (len(hexData) % 2): hexData += "0"; #Must be of even length.

	ROMbytes:list[int] = [int(hexData[i:i+2], 16) for i in range(0, len(hexData), 2)];
	if ((len(shared.ROM_DATA) - 1) >= index): shared.ROM_DATA[index].extend(ROMbytes); #If index entry already exists, add to it.
	else:
		#Otherwise create new entry.
		#Needs to be "padded" to the correct index, with empty ROM indices to ensure the indexing is right.
		for i in range(len(shared.ROM_DATA), index): #If there are 4 entries (latest index is [3]), and this is index 7, loop through i=4,5,6.
			shared.ROM_DATA.append([]); #Empty ROM index.
		shared.ROM_DATA.append(ROMbytes);

	return True;
#### LINE FORMATS ####



#Maps formats of valid lines to ParsedLine instances.
FORMAT_MAPPING:tuple[str, Callable[[list[str]], ...], shared.ParsedLine] = (
    FORM_if, FORM_print,
    FORM_postfix, FORM_infix,
    FORM_unary, FORM_exclaim,
);




def seperateIntoSections(line:str) -> list[str]:
	lineSplit:list[str] = line.lower().split(" ");
	lineSplit = [operand.strip() for operand in lineSplit if operand != ""];

	currentlyInBrackets:bool = False;
	lineSplitModified:list[str] = [];
	for x in lineSplit:
		if (x.startswith("(")): #Open-bracket marks nested line.
			currentlyInBrackets = True;
			lineSplitModified.append(x.replace("(", ""));
		elif (currentlyInBrackets): lineSplitModified[-1] += " " + x.replace(")", "");
		else: lineSplitModified.append(x);
		if (x.endswith(")")): #End this nested line.
			currentlyInBrackets = False;


	lineSplitModified.extend(["#0" for _ in range(max(3 - len(lineSplitModified),0))]); #Ensure it's at least 3 long - fill missing lineSplit with 0s.
	return lineSplitModified;


def convertLine(line:str, makeHex:bool=True, convertMarkers:bool=True) -> list[str]:
	global SHOULD_ADD_TO_ROM;
	SHOULD_ADD_TO_ROM=convertMarkers;

	if (len(line) == 0): return [];
	#print(line)

	#Seperate into lineSplit.
	lineSplit:list[str] = seperateIntoSections(line);


	#Check if this line declares some data for a ROM index, and if so exit.
	if (FORM_ROMdeclaration(lineSplit, makeHex)): return [];

	#Find out what format fits, and if one is found then proceed.
	foundValidFormat:bool = False;
	parsedLine:shared.ParsedLine = shared.INVALID_PARSED_LINE;
	for form in FORMAT_MAPPING:
		result:shared.ParsedLine = form(lineSplit);
		if (result.valid):
			foundValidFormat = True;
			parsedLine = result;
			break;

	if (not foundValidFormat):
		raise FabricationError(f"Unknown Command encountered: {lineSplit}");





	#Something like this.
	operation:shared.Operation = None;
	if (parsedLine.operator in OPERATOR_MAPPING):
		operation = OPERATOR_MAPPING[parsedLine.operator]; #Get this operation from the map.
	else: raise FabricationError(f"Unknown Command encountered: {parsedLine.operator} {parsedLine.operands}");

	
	convertedValues:list[int] = [];
	if (operation.type == shared.OperationType.B_AMBIGUOUS): #B operand is NOT to be converted.
		convertedValues = [
			convertValues(parsedLine.operands[0], convertMarkers=convertMarkers) #Convert A
		];
		if (regex.match("(?i)^r[0-9]+$", parsedLine.operands[1]) is not None):
			convertedValues.append(list(convertValues(parsedLine.operands[1], convertMarkers=convertMarkers)));
			convertedValues[-1][0] = str(convertedValues[-1][0]);
		else:
			convertedValues.append((parsedLine.operands[1], True));
		print(parsedLine, convertedValues)
	elif (operation.type == shared.OperationType.ONLY_B_OPRNDS): #Do not convert A.
		convertedValues = [(parsedLine.operands[0], True),];
		convertedValues.extend([convertValues(x, convertMarkers=convertMarkers) for x in parsedLine.operands[1:]]);
	elif (operation.type != shared.OperationType.ZERO_OPERANDS):
		convertedValues = [convertValues(x, convertMarkers=convertMarkers) for x in parsedLine.operands];
	else:
		convertedValues = [(0, True) for _ in parsedLine.operands];
		

	parsedLine.convertedOperands = tuple([e[0] for e in convertedValues]);
	parsedLine.immediates = (
		("1" if convertedValues[0][1] else "0") +
		("1" if convertedValues[1][1] else "0")
	); #Bits to mark if operands are immediate values or register addresses.
	parsedLine.src = line;

	ln:shared.Data = shared.Data(parsedLine); #Dataset to contain all of the line values, to be passed into the mapping's func.
	#print(parsedLine)
	instructionList:list[str] = operation(ln);

	if (makeHex):
		hexList:list[str] = [f"{int(instruction, 2):06X}".zfill(6) for instruction in instructionList]; #Converts to hexadecimal.
		return hexList;
	else:
		return instructionList;




def processMarkers(aliasReplaced:list[str]):
	"Processes and calculates markers for this file.";
	global MARKERS;

	#Process all lines once, to get the number of instructions for each section. Used for calculating marker indices.
	expandedTMP:list[str] = [];
	for line in aliasReplaced:
		if (not line.startswith(":")):
			#Convert lines using convertLine().
			conversion:list[str] = convertLine(line, makeHex=False, convertMarkers=False);
			if (len(conversion)):
				expandedTMP.extend(conversion) #1 line of CFAB can correspond to multiple instructions
		else:
			expandedTMP.append(line);


	#Create MARKERS dataset.
	for curLine in expandedTMP:
		if (curLine.startswith(":")):
			"""
			Process markers, which are written like so;
			 :marker
			You may jump back to these using BRN, JMP or IF commands, like so;
			 JMP :marker
			 IF (condition) then-goto :marker
			"""
			MARKERS[curLine.replace(":", "").split(" ")[0].upper()] = [
				accLine for accLine in expandedTMP if (
					(not (
						accLine.startswith(":") or #Marker lines ignored
						accLine == "" or           #Empty lines ignored
						accLine.startswith("//")   #Comment lines ignored
					)
				) or accLine == curLine)
			].index(curLine);