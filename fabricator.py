#Binary Opcodes (Processing)
"""
FB_ --> 2 flag-bits in instr; values 0-3.
A/B --> The values in the instruction; can be register addresses or immediate values.
Ia/Ib --> Immediate operand A/B. 0: register index. 1: Immediate value.
INSTR --> Instruction opcode (see below table)

Bit layout;
Ia | Ib | F B | I N S T R |
 1 |  1 | 1 1 |  1 1 1 1  |

 MNE | BITS | HEX | DESCRIPTION
-----+------+-----+-------------
 NOP | 0000 |  0  | Blank, No-Op instruction
 SET | 0001 |  1  | Sets a register to some value. If 2nd value is reg, copy data.
 MOV | 0010 |  2  | Moves contents of 1 register to another, resets source register to 0.
 ADD | 0011 |  3  | FB0, Adds 2 values. Can also be used as logical OR. FB1, bitwise OR.
 SUB | 0100 |  4  | Subtracts 2 values.
 MUL | 0101 |  5  | FB0, Multiplies 2 values. Can also be used as logical AND. FB1, bitwise AND.
 DIV | 0110 |  6  | Int-Divides 2 values. If denominator is 0, result is 0. (later -> div0 flag?)
 NOT | 0111 |  7  | FB0, logical NOT. FB1, bitwise NOT. FB2, invert number. FB3, abs(number)
 EQU | 1000 |  8  | FB0, A==B. FB1, A!=B. (!= is equivalent to XOR.) FB2, bitwise XOR. FB3, bitwise XNOR.
 GRT | 1001 |  9  | FB0, A>B. FB1, A<B. FB2, A>=B. FB3 A<=B. (bit 2 changes inclusivity, bit 1 changes func.)
 BRN | 1010 |  A  | FB0, Branch to (A<<8)|B if rOP!=0. FB1, unconditional branch to (A<<8)|B. FB2, same as FB0 but rOP==0.
 I_O | 1011 |  B  | FB0, Gets input 16, writing 8 to A and 8 to B [A/B MUST BE REGISTERS]. FB1 same but outputs immediate/register 16b.
 SHF | 1100 |  C  | FB0, Left-shifts A by B bits. FB1, Right-shifts A by B bits.
 EXT | 1101 |  D  | Extra; FB0, halt. FB1, Clear all registers. FB2, write to RAM. FB3, read from RAM. [RAM uses rOP for read/write value.]
 __E | 1110 |  E  | 
 __F | 1111 |  F  | 
"""

#Processing Functions
"""
SET 		|	 set A = B 							|	Sets the value in reg A to constant B.
MOV 		|	 mov A B 							|	Moves the value in register A to register B.
AND			|	 A and B 							|	If A and B.
OR			|	 A or B 							|	If A or B.
NOT			|	 not A {B unused}					|	If A is false.
XOR			|	 A xor B 							|	If A or B individually, but not both. (XOR)
EQU			|	 A = B 								|	If A is equal to B.
GRT			|	 A > B 								|	If A is GreaterThan B
LSS			|	 not (A > B or A == B)				|	If A is LessThan B
GTE			|	 not (A < B)						|	If A is GreaterThan B OR they are equal
LSE			|	 not (A > B)						|	If A is LessThan B OR they are equal
INV			|	 not (A) {for all bits}				|	Makes negative with 2s Compliment (All NOT, LSB = same)
ADD			|	 A + B 								|	Adds B to A
SUB			|	 A + (inv B)						|	Subtracts B from A
MUL			|	 A * B 								|	Multiplies 2 values.
DIV			|	 A / B 								|	Divides 2 values; always rounds down.
MOD			|	 A - ((A / B) * B)					|	Gets remainder of 2 values DIV'd.
ABS			|	 abs A {B unused}					|	Sets the MSB to 1. That's it.
SGN			|	 A / (abs A) {B unused}				|	Gets the sign of A (8 bit values, so MSB; written mathmatically for completeness.)
BRN			|	 brn A if B is 1 {Conditional}		|	Jumps to A if reg B is 1.
JMP			|	 brn A 1 {B unused} {Unconditional}	|	Jumps to a marker without a comparison
EXT 		|	 brn :_end 1 {Unconditional}		|	Exit immediately (Jump to end marker)
{marker}	|	 :A {B unused}						|	Marker. Used to branch to.
DEF/END	 |	def %macroName {args}; ...; end 	|	Used to define a macro. Contents of macro added wherever called. Takes {args}.
{macro call}| 	 %macroName {args} 					|	Calls a pre-defined macro. Contents of macro added whenever called. Takes {args}.
{alias}		|	 $aliasName @ A 					|	Every time $aliasName is encountered, replace with register A (useful for programming formatting.)
"""


opcodes = {
	"nop": "0000", "set": "0001", "mov": "0010", "add": "0011",
	"sub": "0100", "mul": "0101", "div": "0110", "not": "0111",
	"equ": "1000", "grt": "1001", "brn": "1010", "i_o": "1011",
	"shf": "1100", "ext": "1101", "__e": "1110", "__f": "1111"
}

infixOperatorsList = {
	"+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
	"!": "not", "&": "and", "|": "or", "~^": "xnor", "^": "xor",
	">": "gtr", "<": "lss", ">=": "gte", "<=": "lse", "==": "equ",
	"=": "set", "~": "mov",
}



class FabricationError(Exception):
	def __init__(self, error:str="Fabrication Failed!"):
		self.message:str = f"Fabrication Error; {error}"
		super().__init__(self.message)



def toBin(value:int) -> str:
	absValue:int = abs(value) & 0x7F; #7 bits
	if (value >= 0):
		return "0" + str(bin(absValue)[2:]).zfill(7);
	else:
		places:list[int] = [
			-128, 64, 32,
			16, 8, 4, 2, 1
		];
		binRep:str = "1";
		recreatedValue:int = places[0];
		for place in places[1:]:
			if ((recreatedValue+place) < value):
				binRep += "1";
				recreatedValue += place;
			else:
				binRep += "0";

		return binRep; 




def convertAllToBin(operator:str, immediates:str, preA:int, preB:int):
	BLANK:str = "00000000";
	REG_RESULT:str = toBin(63);
	A, B = toBin(preA), toBin(preB);

	
	match operator:
		case "nop" | "__e" | "__f":
			return (
				"0000" + opcodes[operator] + BLANK + BLANK,
			);

		case "set" | "mov" | "add" | "sub" | "mul" | "div" | "equ" | "grt" | "jmp" | "i_o" | "shf":
			return (
				immediates + "00" + opcodes[operator] + A + B,
			)

		case "and":
			return (
				immediates + "01" + opcodes["mul"] + A + B,
			);

		case "or":
			return (
				immediates + "01" + opcodes["add"] + A + B,
			);

		case "not":
			return (
				immediates[0] + "000" + opcodes[operator] + A + BLANK,
			)

		case "sgn":
			return (
				immediates[0] + "011" + opcodes["not"] + A + BLANK,
				immediates[0] + "000" + opcodes["div"] + A + REG_RESULT,
			);

		case "inv":
			return (
				immediates[0] + "010" + opcodes["not"] + A,
			);

		case "mod":
			return (
				immediates + "10" + opcodes["div"] + A + B,
			);

		case "neq":
			return (
				immediates + "01" + opcodes["equ"] + A + B,
			);

		case "xor":
			return (
				immediates + "10" + opcodes["equ"] + A + B,
			);

		case "xnor":
			return (
				immediates + "11" + opcodes["equ"] + A + B,
			);

		case "lss":
			return (
				immediates + "01" + opcodes["grt"] + A + B,
			);

		case "gte":
			return (
				immediates + "10" + opcodes["grt"] + A + B,
			);

		case "lse":
			return (
				immediates + "11" + opcodes["grt"] + A + B,
			);

		case "brn":
			instrIdx:int = str(bin(preA & 0xFFFF)[2:]).zfill(16); #16-bit.
			return (
				immediates + "10" + opcodes["brn"] + instrIdx,
			);

		case "halt":
			return (
				"0000" + opcodes["ext"] + BLANK + BLANK,
			);

		case "clear":
			return (
				immediates[0] + "001" + opcodes["ext"] + A + BLANK,
			);

		case "ramwrite":
			return (
				immediates + "10" + opcodes["ext"] + A + B,
			);

		case "ramread":
			return (
				immediates + "11" + opcodes["ext"] + A + B,
			);



		case _:
			raise FabricationError(f"Unknown Command encountered: {operator} {A} {B}")



def convertValues(A):
	if A.startswith("r"): #Registers
		return (int(A.replace("r", "")), False);
	elif A.startswith("#x"): #Immediate values (Hex)
		return (int(A.replace("#x",""), 16), True);
	elif A.startswith("#b"): #Immediate values (Binary)
		return (int(A.replace("#b",""), 2), True);
	elif A.startswith("#"): #Immediate values (Denary)
		return (int(A.replace("#d", "").replace("#","")), True);
	elif A.startswith(":"): #Markers
		return (markers[A.replace(":", "")], True);
	else:
		try:
			return (int(A), True);
		except ValueError:
			return (str(A), True); #Ensure the fallback is a string.



def convertLine(line):
	operands = line.split(" ")
	operands = [operand.strip() for operand in operands if operand != ""]
	if len(operands) == 2:
		operands.append("0")
	elif len(operands) == 1:
		operands.extend(["0", "0"])

	if operands[0] in opcodes:
		#Postfix
		operator, A, B = operands

	elif operands[1] in infixOperatorsList:
		#Infix
		A, operator, B = operands
		operator = infixOperatorsList[operator]

	elif operands[0] in ("ext", "inv", "sgn", "jmp", "clr", "ramwrite", "ramread", "xnor"):
		#Chained or unusual operators
		operator, A, B = operands

	elif operands[0] in ("!",):
		operator, A, B = operands
		operator = infixOperatorsList[operator]

	else:
		raise FabricationError(f"Unknown Command encountered: {operands}")


	A, immA = convertValues(A)
	B, immB = convertValues(B)

	immediates = f"{'1' if immA else '0'}{'1' if immB else '0'}"

	instructionList = convertAllToBin(operator, immediates, A, B)
	hexList = [f"{int(instruction, 2):06X}" for instruction in instructionList]
	return hexList



def replaceMacros(lines, depth=0, activeMacros=None, previousMacro=None):
	if activeMacros is None:
		activeMacros = set()
	macrosReplaced = []
	lineNum = 0


	#Check for excessive recursion depth to prevent absurdly long chaining.
	maxRecursionDepth = 32
	if depth > maxRecursionDepth:
		raise FabricationError("Exceeded maximum macro recursion depth (32)")

	while lineNum < len(lines):
		curLine = lines[lineNum]

		#Macro has been defined.
		if curLine.startswith("def"):
			macroData = curLine.split(" ")
			if len(macroData) < 2:
				raise FabricationError(f"Macro definition missing name and/or parameters: {curLine}")

			macroName = macroData[1].replace("%", "")
			macroParams = macroData[2:]
			macroLines = []
			i = 0

			#Save the contents of the macro.
			while True:
				i += 1
				macroLine = lines[lineNum + i]
				if macroLine.startswith("end"):
					lineNum += i
					macros[macroName] = (macroLines, macroParams)
					break
				macroLines.append(macroLine)


		#Macro has been called.
		elif curLine.startswith("%"):
			lineData = curLine.split(" ")
			macroName = lineData[0][1:]

			#Check the macro was defined beforehand.
			if macroName not in macros:
				raise FabricationError(f"Macro %{macroName} is not defined.")

			#Prevent macros from making infinite recursive loops.
			if macroName in activeMacros:
				if previousMacro is None:
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName}."
				elif previousMacro == macroName:
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName} within itself"
				else:
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName} within %{previousMacro}.\nThis resulted in a chain of macros, in a loop."

				raise FabricationError(errorMessage)


			#Begin expansion of macro
			macroLines, macroParams = macros[macroName]
			if len(lineData[1:]) != len(macroParams):
				raise FabricationError(f"Macro {macroName} expects {len(macroParams)} arguments, but got {len(lineData[1:])}")

			#Change macro args to their called counterparts.
			paramMapping = dict(zip(macroParams, lineData[1:]))
			activeMacros.add(macroName)  # Track macro to prevent re-expansion


			#Expand the macro to the line it was called at.
			expandedLines = []
			for macroLine in macroLines:
				processedLine = macroLine
				for param, arg in paramMapping.items():
					processedLine = processedLine.replace(param, arg)
				expandedLines.append(processedLine)


			#If a macro was found inside this macro, recursively unpack that too.
			macrosReplaced.extend(replaceMacros(expandedLines, depth + 1, activeMacros, macroName))
			activeMacros.remove(macroName)


		#Any other lines.
		else:
			macrosReplaced.append(curLine)

		lineNum += 1

	return macrosReplaced



def replaceAliases(lines):
	aliases = {
		"rop": "r63",
	}
	aliasReplaced = []

	for lineNum, curLine in enumerate(lines):
		#print(curLine)
		"""
		Define aliasing for register names like so;
		 varName  @ r1
		Every time varName is written, it is replaced by r1 by the fabricator.
		Allows for nicer formatting of CFAB.
		"""
		operands = curLine.split(" ")
		if len(operands) == 3:
			if operands[1] == "@":
				aliases[operands[0].replace("$", "")] = operands[2]
				continue

		fixedLine = curLine
		for alias, reg in aliases.items():
			fixedLine = fixedLine.replace(f"${alias}", reg).replace(alias, reg)

		if fixedLine not in aliasReplaced:
			aliasReplaced.append(fixedLine)

	return aliasReplaced




if __name__ == "__main__":

	filename = "testloop"
	with open(f"cfab/{filename}.cfab", "r") as CFABFile:
		readlines = CFABFile.readlines()
		partial_lines = [line.strip().lower() for line in readlines if not line.startswith("//")]
		lines = [line for line in partial_lines if line != ""]


		macros, markers = {}, {}
		macrosReplaced = replaceMacros(lines)
		aliasReplaced = replaceAliases(macrosReplaced)
		del macrosReplaced


		for curLine in aliasReplaced:
			if curLine.startswith(":"):
				"""
				Process markers, which are written like so;
				 :marker
				You may jump back to these using BRN, JMP or EXT commands, like so;
				 JMP :marker
				EXT Always jumps to the final line of the instructions.
				"""
				markers[curLine.replace(":", "")] = [accLine for accLine in aliasReplaced if ((not (accLine.startswith(":") or accLine == "" or accLine.startswith("//"))) or accLine == curLine)].index(curLine)
		markers["_end"] = len(aliasReplaced)-1


		fabricated = []
		for line in aliasReplaced:
			if not line.startswith(":"):
				#Convert lines using convertLine().
				fabricated.extend(convertLine(line))

		del macros, markers
		del aliasReplaced

		
		#Make the list of hex instructions into a set of bytes.
		combinedHex = ""
		for hexInstruction in fabricated:
			combinedHex += hexInstruction

		if len(combinedHex) % 2 == 1: combinedHex += "0"

		combinedBytes = bytes.fromhex(combinedHex)


	with open(f"data/{filename}.dat", "wb") as outFile:
		#Write to a file.
		outFile.write(combinedBytes)