"translate.py"
from typing import Callable;
from fabsrc import shared;



global SHOULD_ADD_TO_ROM, MARKERS;
SHOULD_ADD_TO_ROM:bool = False;
MARKERS:dict[str, int] = {};


opcodes:tuple[str, ...] = (
	"nop", "set", "mov", "add",
	"sub", "mul", "div", "not",
	"equ", "grt", "brn", "i_o",
	"shf", "ext", "slp", "mem"
); #Index is opcode number.


#### HELPER FUNCTIONS ####
def getOperatorBinary(opcode:str) -> str:
	if (opcode in opcodes):
		return f"{opcodes.index(opcode):04b}";
	else:
		raise FabricationError(f"Unknown opcode: {opcode}");


def convertValues(V:str, convertMarkers:bool=True) -> tuple[int, bool]:
	"Convert operands to actual interger values";
	if V.startswith("r"): #Registers
		return (int(V.replace("r", "")), False);
	elif V.startswith("#x"): #Immediate values (Hex)
		return (int(V.replace("#x",""), 16), True);
	elif V.startswith("#b"): #Immediate values (Binary)
		return (int(V.replace("#b",""), 2), True);
	elif V.startswith("#"): #Immediate values (Denary)
		return (int(V.replace("#d", "").replace("#","")), True);
	elif V.startswith(":"): #Markers
		if (convertMarkers): return (MARKERS[V.replace(":", "").upper()], True);
		else: return (0, True); #Filler marker index, ensures binary is upheld even if not correct.
	else:
		try: return (int(V), True);
		except ValueError: return (str(V), True); #Ensure the fallback is a string.
#### HELPER_FUNCTIONS ####


infixOperatorsList:dict[str, str] = {
	"+":  "add",
	"-":  "sub",
	"*":  "mul",
	"/":  "div",
	"%":  "mod",
	"!":  "not",  #Logical
	"&":  "and",  #...
	"|":  "or",   #...
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
}; #Inline operators mapped to their actual opcode names.




#### COMPLEX OPCODES ####
def OPC_branch(ln:shared.Data) -> list[str, ...]:
	try: instrIdx:int = str(bin(ln.preA & 0xFFFF)[2:]).zfill(16); #16-bit.
	except TypeError: instrIdx = preA;
	return [f"{ln.immediates}10{getOperatorBinary('brn')}{instrIdx}",];

def OPC_jump(ln:shared.Data) -> list[str, ...]:
	try: instrIdx:int = str(bin(ln.preA & 0xFFFF)[2:]).zfill(16); #16-bit.
	except TypeError: instrIdx = ln.preA;
	return [f"{ln.immediates}00{getOperatorBinary('brn')}{instrIdx}",];

def OPC_sleep(ln:shared.Data) -> list[str, ...]:
	sleepMS:int = str(bin(ln.preA & 0xFFFF)[2:]).zfill(16); #16-bit.
	return [f"{ln.immediates}00{getOperatorBinary('slp')}{sleepMS}",];
#### COMPLEX OPCODES ####




#### ABSTRACTIONS ####
def ABS_increment(ln:shared.Data) -> list[str, ...]:
	return [
		f"{ln.immediates[0]}100{getOperatorBinary('add')}{ln.A}{shared.TRUE}",
		f"0000{getOperatorBinary('mov')}{shared.REG_RESULT}{ln.A}",
	];

def ABS_decrement(ln:shared.Data) -> list[str, ...]:
	return [
		f"{ln.immediates[0]}100{getOperatorBinary('sub')}{ln.A}{shared.TRUE}",
		f"0000{getOperatorBinary('mov')}{shared.REG_RESULT}{ln.A}",
	];

def ABS_sign(ln:shared.Data) -> list[str, ...]:
	return [
		f"{ln.immediates[0]}011{getOperatorBinary('not')}{ln.A}{shared.BLANK}",
		f"{ln.immediates[0]}000{getOperatorBinary('div')}{ln.A}{shared.REG_RESULT}",
	];

def ABS_if(ln:shared.Data) -> list[str, ...]:
	condition:str = convertLine(ln.preA, makeHex=False)[0];
	try: instrIdx:int = f"{(ln.preB & 0xFFFF):16b}"; #16-bit.
	except TypeError: instrIdx = ln.preB;
	
	return [
		condition,
		f"{ln.immediates}10{getOperatorBinary('brn')}{instrIdx}",
	];

def ABS_cout(ln:shared.Data) -> list[str, ...]:
	if (preA == "\\n"): #Just a newline char (0x0A)
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

def ABS_print(ln:shared.Data) -> list[str, ...]:
	#Writes text to console. Uses ROM at the end of the file.
	text:str = ln.text.split('"')[1].replace('"','');
	if (not SHOULD_ADD_TO_ROM): return [f"1011{getOperatorBinary('i_o')}{shared.BLANK}{shared.BLANK}",];

	ROMidx:int = len(shared.ROM_DATA); #Add to end of index. Take last index.
	instructions:list[str] = [f"1011{getOperatorBinary('i_o')}{shared.toBin(ROMidx)}{shared.BLANK}",]

	text = text.replace("\\n", "\n"); #Replace with actual 0x0A newline chars;
	shared.ROM_DATA.append([ord(char) for char in text]);

	return instructions;

def ABS_load(ln:shared.Data) -> list[str, ...]:
	#Example: [LOAD #0 #2]
	#Load ROM segment 0 into RAM [2:]
	try: ramAddress:int = f"{(ln.preB & 0xFFFF):16b}"; #16-bit.
	except TypeError: ramAddress = ln.preB;
	return [
		f"0{ln.immediates[0]}00{getOperatorBinary('set')}{shared.REG_X}{ln.A}",  #Use "A" as the ROM segment.
		f"1110{getOperatorBinary('mem')}{ramAddress}", #Write to this ram address.
	];
	
def ABS_copy(ln:shared.Data) -> list[str, ...]:
	#Example: [COPY #2 #10 #11]
	#Copy RAM [2:10] to [11:]
	operands:list[str, ...] = ln.text.split(" "); #Get operands manually - it has 3, normally no support for this.
	preC:str = convertValues(operands[3])[0];

	try: srcStart:int = f"{(ln.preA & 0xFFFF):16b}"; #16-bit.
	except TypeError: srcStart = ln.preA;

	try: srcEnd:int = f"{(ln.preB & 0xFFFF):16b}"; #16-bit.
	except TypeError: srcEnd = ln.preB;

	try: destination:int = f"{(preC & 0xFFFF):16b}"; #16-bit.
	except TypeError: destination = preC;

	return [
		f"0100{getOperatorBinary('set')}{shared.REG_X}{srcEnd[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Y}{srcEnd[8:]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Z}{destination[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_W}{destination[8:]}",
		f"1101{getOperatorBinary('mem')}{srcStart}"
	];
	
def ABS_RAMclear(ln:shared.Data) -> list[str, ...]:
	#Example: [RAMCLEAR #2 #10 #1]
	#Clear values in RAM [2:10] to 1
	operands:list[str, ...] = ln.text.split(" "); #Get operands manually - it has 3, normally no support for this.
	preC:str = convertValues(operands[3])[0];

	try: srcStart:int = f"{(ln.preA & 0xFFFF):16b}"; #16-bit.
	except TypeError: srcStart = ln.preA;

	try: srcEnd:int = f"{(ln.preB & 0xFFFF):16b}"; #16-bit.
	except TypeError: srcEnd = ln.preB;

	return [
		f"0100{getOperatorBinary('set')}{shared.REG_X}{shared.toBin(preC)}",
		f"0100{getOperatorBinary('set')}{shared.REG_Y}{srcEnd[:8]}",
		f"0100{getOperatorBinary('set')}{shared.REG_Z}{srcEnd[8:]}",
		f"1101{getOperatorBinary('mem')}{srcStart}"
	];
#### ABSTRACTIONS ####



#Maps operation mneumonics to their instruction calls.
operatorMapping:dict[str, Callable] = {
	#0-Operand operations;
	"nop":      (lambda ln : [f"0000{getOperatorBinary('nop')}{shared.BLANK}{shared.BLANK}",]), #No-Operation
	"halt":     (lambda ln : [f"0000{getOperatorBinary('ext')}{shared.BLANK}{shared.BLANK}",]), #Halt/Exit
	"wait":     (lambda ln : [f"0001{getOperatorBinary('slp')}{shared.BLANK}{shared.BLANK}",]), #Wait for user input


	#1-Operand operations;
	"set":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('set')}{ln.A}{ln.B}",]), #Assign memory a value
	"mov":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('mov')}{ln.A}{ln.B}",]), #Move memory
	"add":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('add')}{ln.A}{ln.B}",]), #Addition
	"sub":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('sub')}{ln.A}{ln.B}",]), #Subtraction
	"mul":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('mul')}{ln.A}{ln.B}",]), #Multiplication
	"div":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('div')}{ln.A}{ln.B}",]), #Division
	"equ":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('equ')}{ln.A}{ln.B}",]), #Equal
	"grt":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('grt')}{ln.A}{ln.B}",]), #Greater than
	"i_o":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('i_o')}{ln.A}{ln.B}",]), #Input_Output
	"shf":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('shf')}{ln.A}{ln.B}",]), #Bitshift


	#2-Operand operations;
	"not":      (lambda ln : [f"{ln.immediates[0]}000{getOperatorBinary('not')}{ln.A}{shared.BLANK}",]), #Logical Not
	"inv":      (lambda ln : [f"{ln.immediates[0]}010{getOperatorBinary('not')}{ln.A}{shared.BLANK}",]), #Bitwise Not
	"clear":    (lambda ln : [f"{ln.immediates[0]}001{getOperatorBinary('ext')}{ln.A}{shared.BLANK}",]), #Fill all registers with value A|*A


	#2-Opcode operations that use other operators;
	"and":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('mul')}{ln.A}{ln.B}",]), #Logical And
	"or":       (lambda ln : [f"{ln.immediates}00{getOperatorBinary('add')}{ln.A}{ln.B}",]), #Logical Or
	"mod":      (lambda ln : [f"{ln.immediates}01{getOperatorBinary('div')}{ln.A}{ln.B}",]), #Modulo
	"neq":      (lambda ln : [f"{ln.immediates}01{getOperatorBinary('equ')}{ln.A}{ln.B}",]), #Not equal
	"xor":      (lambda ln : [f"{ln.immediates}10{getOperatorBinary('equ')}{ln.A}{ln.B}",]), #Exclusive Or
	"xnor":     (lambda ln : [f"{ln.immediates}11{getOperatorBinary('equ')}{ln.A}{ln.B}",]), #Exclusive Not-Or
	"lss":      (lambda ln : [f"{ln.immediates}01{getOperatorBinary('grt')}{ln.A}{ln.B}",]), #Less than
	"gte":      (lambda ln : [f"{ln.immediates}10{getOperatorBinary('grt')}{ln.A}{ln.B}",]), #Greater than or equal
	"lse":      (lambda ln : [f"{ln.immediates}11{getOperatorBinary('grt')}{ln.A}{ln.B}",]), #Less than or equal
	"rsh":      (lambda ln : [f"{ln.immediates}00{getOperatorBinary('shf')}{ln.A}{ln.B}",]), #Rightshift
	"lsh":      (lambda ln : [f"{ln.immediates}01{getOperatorBinary('shf')}{ln.A}{ln.B}",]), #Leftshift
	"ramwrite": (lambda ln : [f"{ln.immediates}10{getOperatorBinary('ext')}{ln.A}{ln.B}",]), #Write into RAM
	"ramread":  (lambda ln : [f"{ln.immediates}11{getOperatorBinary('ext')}{ln.A}{ln.B}",]), #Read from RAM
	"input":    (lambda ln : [f"{ln.immediates}00{getOperatorBinary('i_o')}{ln.A}{ln.B}",]), #Input
	"output":   (lambda ln : [f"{ln.immediates}01{getOperatorBinary('i_o')}{ln.A}{ln.B}",]), #Output


	#Complex operations;
	"brn":      OPC_branch,    #Conditional branching
	"jmp":      OPC_jump,      #Unconditional branching
	"sleep":    OPC_sleep,     #Sleep for specified number of milliseconds


	#Complex abstractions of operations;
	"inc":      ABS_increment, #Increments in-place
	"dec":      ABS_decrement, #Decrements in-place
	"sgn":      ABS_sign,      #Integer sign
	"if":       ABS_if,		   #Nicer formatted conditional branching
	"cout":		ABS_cout,	   #Show single value in console
	"print":	ABS_print,	   #Show larger, fixed block of text in console
	"load":		ABS_load,	   #Loads section of ROM into RAM
	"copy":		ABS_copy,      #Copies one section of RAM into another
	"ramclear": ABS_RAMclear,  #Fills RAM in range with some value
};




def convertAllToBin(operator:str, immediates:str, preA:int, preB:int, text:str=""):
	ln:shared.Data = shared.Data(text, immediates, preA, preB); #Dataset to contain all of the line values, to be passed into the mapping's func.

	if (operator in operatorMapping):
		return operatorMapping[operator](ln); #Get this operator's instructions via map.
	else: raise FabricationError(f"Unknown Command encountered: {operator} {A} {B}");





def convertLine(line:str, makeHex:bool=True, convertMarkers:bool=True) -> list[str]:
	global SHOULD_ADD_TO_ROM;

	operands:list[str] = line.lower().split(" ");
	operands = [operand.strip() for operand in operands if operand != ""];
	if len(operands) == 2:
		operands.append("0")
	elif len(operands) == 1:
		operands.extend(["0", "0"])

	if operands[0] in opcodes:
		#Postfix
		operator, A, B = operands[:3];

	elif operands[1] in infixOperatorsList:
		#Infix
		A, operator, B = operands[:3];
		operator = infixOperatorsList[operator]

	elif (operands[0] == "if"):
		operator = "if";
		A = " ".join(operands[1:-2]).replace("(", "").replace(")","")
		B = operands[-1]

	elif (operands[0] == "print"):
		operator = "print";
		A = "0";
		B = "0";

	elif (regex.match(r"(?i)ROM[0-9]+", operands[0]) is not None):
		#ROM data definition, Hex.
		if (makeHex):
			index:int = int(operands[0].replace("rom",""));
			hexData:str = "".join([(x if (x in "0123456789abcdef") else "") for x in "".join(operands[1:]).strip()]); #Completely ignores non-hex values.
			if (len(hexData)%2): hexData += "0"; #Must end on a whole number of bytes.
			intData:list[int] = [];
			for i in range(len(hexData)//2): intData.append(int(hexData[i*2]+hexData[(i*2)+1], 16)); #Combines 2 hex digits to create an 8 bit number.
			if (len(ROM_DATA)-1 >= index): ROM_DATA[index].extend(intData);
			else: ROM_DATA.append(intData);
		return [];

	elif operands[0] in (
		"ext", "inv", "sgn", "jmp", "clr", "ramwrite", "ramread",
		"and", "or", "xor", "xnor", "halt", "sleep", "wait",
		"input", "output", "cout", "inc", "dec"
	):
		#Chained or unusual operators
		operator, A, B = operands[:3];

	elif operands[0] in ("load", "ramclear", "copy"):
		#Chained or unusual operators with 3+ values
		operator, A, B = operands[:3];



	elif operands[0] in ("!",):
		operator, A, B = operands
		operator = infixOperatorsList[operator]

	else:
		raise FabricationError(f"Unknown Command encountered: {operands}")

	if (operator != "if"):
		A, immA = convertValues(A, convertMarkers);
	else:
		immA = True;
	B, immB = convertValues(B, convertMarkers);

	immediates = f"{'1' if immA else '0'}{'1' if immB else '0'}"

	SHOULD_ADD_TO_ROM=convertMarkers;
	instructionList = convertAllToBin(operator=operator, immediates=immediates, preA=A, preB=B, text=line);
	hexList:list[str] = [f"{int(instruction, 2):06X}" for instruction in instructionList] if makeHex else instructionList;

	return hexList;





def processMarkers(aliasReplaced:list[str]):
	"Processes and calculates markers for this file.";
	global MARKERS;

	#Process all lines once, to get the number of instructions for each section. Used for calculating marker indices.
	expandedTMP:list[str] = [];
	for line in aliasReplaced:
		if not line.startswith(":"):
			#Convert lines using convertLine().
			expandedTMP.extend(convertLine(line, makeHex=True, convertMarkers=False)) #1 line of CFAB can correspond to multiple instructions
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