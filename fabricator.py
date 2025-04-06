#Binary Opcodes (Processing)
"""
Bin  | Opc | Hex-Rep.
_____|_____|__________
0000 | NOP | 00000
0001 | SET | 1XXAA
0010 | MOV | 2XXYY
0011 | AND | 3XXYY
0100 | OR_ | 4XXYY
0101 | NOT | 5XX00
0110 | LSS | 6XXYY
0111 | EQU | 7XXYY
1000 | GRT | 8XXYY
1001 | ADD | 9XXYY
1010 | SUB | AXXYY
1011 | MUL | BXXYY
1100 | DIV | CXXYY
1101 | ABS | DXX00
1110 | BRN | EXXYY
1111 | UPD | FXXYY
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
DEF/END	 	|	 def %macroName {args}; ...; end 	|	Used to define a macro. Contents of macro added wherever called. Takes {args}.
{macro call}| 	 %macroName {args} 					|	Calls a pre-defined macro. Contents of macro added whenever called. Takes {args}.
{alias}		|	 $aliasName @ A 					|	Every time $aliasName is encountered, replace with register A (useful for programming formatting.)
"""

#Graphics Functions
"""
COL 	|	 mov A r254				|	Colour changer.
PTR 	|	 mov A r252; mov A r253	|	Pointer, with coordinates.
REC 	|	 rec A B				|	Rectangle.
PIX 	|	 pnt A B; rec A B		|	Singular Pixel.
LNE 	|	 dim A B; lne A B		|	Line between 2 points.
CLR 	|	 pnt 8 8; rec 0 0 		|	Clear screen.
"""

global currentColour
sw = None
currentColour = 0
VRAMw, VRAMh = 12, 8
VRAMstart = 512 - (VRAMw * VRAMh) #416

opcodes = {
	"nop": "0000", "set": "0001", "mov": "0010", "and": "0011",
	"or": "0100", "not": "0101", "lss": "0110", "equ": "0111",
	"gtr": "1000", "add": "1001", "sub": "1010", "mul": "1011",
	"div": "1100", "abs": "1101", "brn": "1110", "upd": "1111"
}

infixOperatorsList = {
	"+": "add", "-": "sub", "*": "mul", "/": "div", "%": "mod",
	"!": "not", "&": "and", "|": "or", "^": "xor",
	">": "gtr", "<": "lss", ">=": "gte", "<=": "lse", "==": "equ",
	"=": "set", "~": "mov",
}

colourNames = (
	"black", "grey", "white", "red",
	"pink", "darkbrown", "lightbrown", "orange",
	"yellow", "bluegrey", "darkgreen", "lightgreen",
	"magenta", "darkblue", "midblue", "lightblue",
)



class FabricationError(Exception):
	def __init__(self, error="Fabrication Failed!"):
		self.message = f"Fabrication Error; {error}"
		super().__init__(self.message)



def toBin(value, signed=False):
	if signed:
		#if not (-128 <= value < 128):
		#	raise FabricationError(f"Immediate values must be signed 8-bit Integers [-128 → 127]: Encountered value of {value}")
	
		return format(value+128, '08b')


	else:
		if not (0 <= value < 256):
			raise FabricationError(f"Register indices must be unsigned 8-bit Integers [0 → 255]: Encountered value of {value}")

		return format(value, '08b')




def SET(register, value, immediate=True):
	return opcodes["set"] + register + toBin(int(value), signed=immediate)



def BRN(line, condition, bits):
	binaryLine = str(bin(line)[2:]).zfill(16)
	halfA, halfB = int(binaryLine[:8], 2), int(binaryLine[8:], 2)
	return (
		"0001" + SET(toBin(29), halfA), #Set first JMP register to the first 8 bits.
		"0001" + SET(toBin(30), halfB), #Set second JMP register to the final 8 bits.
		"00" + bits[2] + "0" + opcodes["brn"] + toBin(condition) + toBin(0),
		blank*6, #NOP afterward for safety.
	)


def PIX(A, B):
	global currentColour
	#Coordinate (A, B)
	RAMaddr = VRAMstart + (A) + (VRAMw * (B))
	binaryLine = str(bin(RAMaddr)[2:]).zfill(16)
	halfA, halfB = int(binaryLine[:8], 2), int(binaryLine[8:], 2)

	return (
			"0101" + SET(toBin(29), halfA, immediate=False),
			"0101" + SET(toBin(30), halfB, immediate=False),
			"0001" + SET(toBin(28), currentColour),
			"0001" + SET(toBin(26), toBin(1)),
		)



def convertAllToBin(operator, convertedA, convertedB):
	global currentColour
	width, height, blank = toBin(24), toBin(16), "0000"
	zero, one, rOP = toBin(0), toBin(1), toBin(31)

	if convertedA[0] != None:
		initial4Bits = f"00{str(int(bool(convertedA[0])))}0"
	if convertedB[0] != None:
		initial4Bits = f"000{str(int(bool(convertedB[0])))}"
	if convertedA[0] != None and convertedB[0] != None:
		initial4Bits = f"00{str(int(bool(convertedA[0])))}{str(int(bool(convertedB[0])))}"


	A, B = convertedA[1], convertedB[1]

	
	match operator:
		case "mov" | "and" | "or" | "lss" | "equ" | "gtr" | "add" | "sub" | "mul" | "div" | "rec" | "lne":
			return (
				initial4Bits + opcodes[operator] + toBin(A, signed=convertedA[0]) + toBin(B, signed=convertedB[0]),
			)


		case "nop":
			return (
				blank*6,
			)


		case "upd":
			return (
				initial4Bits + opcodes[operator] + blank*4,
				initial4Bits + opcodes[operator] + blank*4,
			)


		case "set":
			return (
				initial4Bits + SET(toBin(A, signed=convertedA[0]), B, immediate=convertedB[0]),
			)


		case "not" | "abs":
			return (
				initial4Bits + opcodes[operator] + toBin(A, signed=convertedA[0]) + blank*2,
			)


		case "sgn":
			return (
				initial4Bits + opcodes["abs"] + toBin(A, signed=convertedA[0]) + blank,
				initial4Bits[:3] + "0" + opcodes["div"] + toBin(A, signed=convertedA[0]) + rOP,
			)


		case "inv":
			return (
				"000" + initial4Bits[3] + opcodes["sub"] + zero + toBin(A, signed=convertedA[0]),
			)


		case "mod":
			return (
				initial4Bits + opcodes["div"] + toBin(A, signed=convertedA[0]) + toBin(B, signed=convertedB[0]),
				"000" + initial4Bits[3] + opcodes["mul"] + rOP + toBin(B, signed=convertedB[0]),
				"000" + initial4Bits[3] + opcodes["sub"] + toBin(A, signed=convertedA[0]) + rOP,
			)


		case "xor":
			return (
				initial4Bits + opcodes["equ"] + toBin(A, signed=convertedA[0]) + toBin(B, signed=convertedB[0]),
				blank + opcodes["not"] + rOP + blank,
			)


		case "gte":
			return (
				initial4Bits + opcodes["lss"] + toBin(A, signed=convertedA[0]) + toBin(B, signed=convertedB[0]),
				blank + opcodes["not"] + rOP + blank,
			)


		case "lse":
			return (
				opcodes["gtr"] + toBin(A, signed=convertedA[0]) + toBin(B, signed=convertedB[0]),
				blank + opcodes["not"] + rOP + blank,
			)


		case "jmp":
			return BRN(A, 1, "0011")


		case "brn":
			return BRN(A, B, initial4Bits)


		case "ext":
			return BRN(markers["_end"], 1, "0011")


		case "ramread":
			if not (0 <= A <= 511): raise FabricationError(f"RAM Index cannot be higher than 511, or lower than 0. Got: {A}")
			binaryLine = str(bin(A)[2:]).zfill(16)
			halfA, halfB = int(binaryLine[:8], 2), int(binaryLine[8:], 2)
			val = (
				"0001" + SET(toBin(30), halfA, immediate=False),
				"0001" + SET(toBin(29), halfB, immediate=False),
				"0001" + SET(toBin(27), one),
			)
			if not sw: val.append("0000" + opcodes["mov"] + toBin(27) + rOP)
			return val


		case "ramwrite":
			if not (0 <= A <= 511): raise FabricationError(f"RAM Index cannot be higher than 511, or lower than 0. Got: {A}")
			binaryLine = str(bin(A)[2:]).zfill(16)
			halfA, halfB = int(binaryLine[:8], 2), int(binaryLine[8:], 2)
			if initial4Bits[3] == "1":
				return (
					"0101" + SET(toBin(30), halfA, immediate=False),
					"0101" + SET(toBin(29), halfB, immediate=False),
					"0001" + SET(toBin(28), B),
					"0001" + SET(toBin(26), one),
				)
			else:
				return (
					"0101" + SET(toBin(30), halfA, immediate=False),
					"0101" + SET(toBin(29), halfB, immediate=False),
					"0000" + opcodes["mov"] + toBin(B, signed=convertedB[0]) + toBin(28),
					"0001" + SET(toBin(26), one),
				)


		case "col":
			if A.lower() not in colourNames:
				raise FabricationError(f"Unknown colour; {A.lower()}")
			currentColour = colourNames.index(A.lower())


		case "pix":
			return PIX(A, B)


		case "clr":
			instructions = []
			for x in range(VRAMw):
				for y in range(VRAMh):
					instructions.append(PIX(x, y))

			return instructions


		case _:
			raise FabricationError(f"Unknown Command encountered: {operator} {A} {B}")



def convertValues(A):
	if A.lower() in colourNames:
		return None, A.lower()

	elif A.startswith("r"):
		registerIndex = int(A.replace("r", ""))
		if registerIndex > 31 or registerIndex < 0:
			if registerIndex < 48: return False, registerIndex

			raise FabricationError(f"r{registerIndex} is out of range for registers. Limits are r0 and r31.")

		return False, registerIndex

	elif A.startswith("#"):
		return True, int(A.replace("#", ""))

	elif A.startswith(":"):
		return None, markers[A.replace(":", "")][1]

	elif A.capitalize() == "True":
		return True, 1

	elif A.capitalize() == "False":
		return True, 0

	else:
		return None, str(A).lower() #Ensure the fallback is a string.



def convertLine(line):
	operands = line.split(" ")
	operands = [operand.strip() for operand in operands if operand != ""]
	if len(operands) == 2:
		operands.append("0")
	elif len(operands) == 1:
		operands.extend(["0", "0"])

	if operands[0] in opcodes or operands[0] in ("ext", "lse", "gte", "inv", "sgn", "jmp", "xor", "mod", "ramwrite", "ramread", "nop", "col", "clr", "pix"):
		#Prefix
		operator, A, B = operands

	elif operands[1] in infixOperatorsList:
		#Infix
		A, operator, B = operands
		operator = infixOperatorsList[operator]

	elif operands[0] in ("!",):
		operator, A, B = operands
		operator = infixOperatorsList[operator]

	else:
		raise FabricationError(f"Unknown Command encountered: {operands}")

	convertedA = convertValues(A)
	convertedB = convertValues(B)

	instructionList = convertAllToBin(operator, convertedA, convertedB)
	if instructionList is not None:
		hexList = [f"{int(instruction, 2):06X}" for instruction in instructionList]
		return hexList
	return None


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
	baseAliases = {
		"out0": "r18",
		"out1": "r19",
		"out2": "r20",
		"out3": "r21",

		"in0": "r22",
		"in1": "r23",
		"in2": "r24",
		"in3": "r25",

		"rop": "r31",
	}
	aliases = {}
	aliasReplaced = []

	for lineNum, curLine in enumerate(lines):
		"""
		Define aliasing for register names like so;
		 varName @ r1
		Every time varName is written, it is replaced by r1 by the fabricator.
		Allows for nicer formatting of CFAB.
		"""
		operands = curLine.split(" ")
		if len(operands) == 3:
			if operands[1] == "@":
				aliases[operands[0].replace("$", "")] = operands[2]
				continue

		fixedLine = curLine.lower()
		for alias, reg in baseAliases.items():
			if alias in fixedLine.split(" "):
				fixedLine = fixedLine.replace(alias, reg)
		for alias, reg in aliases.items():
			fixedLine = fixedLine.replace(f"${alias}", reg)

		if fixedLine not in aliasReplaced:
			aliasReplaced.append(fixedLine)

	return aliasReplaced




if __name__ == "__main__":
	filename = input("File to convert [With extension];\n> ")
	
	sw = input("Stormworks format [yes/no]?\n> ")
	if sw.lower() in ("yes", "ye", "y", "true", "t"): sw = True
	else: sw = False

	with open(f"cfab\\{filename}", "r") as CFABFile:
		readlines = CFABFile.readlines()
		partial_lines = [line.strip().lower() for line in readlines if not line.startswith("//")]
		lines = [line for line in partial_lines if line != ""]


		macros, markers = {}, {}
		macrosReplaced = replaceMacros(lines) #Unpacks macros
		aliasReplaced = replaceAliases(macrosReplaced) #Replaces aliases
		del macrosReplaced


		for curLine in aliasReplaced: #Finds markers, and assigns values to them.
			if curLine.startswith(":"):
				"""
				Process markers, which are written like so;
				 :marker
				You may jump back to these using BRN, JMP or EXT commands, like so;
				 JMP :marker
				EXT Always jumps to the final line of the instructions.
				"""
				markers[curLine.replace(":", "")] = ([accLine for accLine in aliasReplaced if ((not (accLine.startswith(":") or accLine == "" or accLine.startswith("//"))) or accLine == curLine)].index(curLine)+1, 0)
		markers["_end"] = (len(aliasReplaced)-1, 0)


		lnnum = 0
		for line in aliasReplaced:
			if not line.startswith(":"):
				#Convert lines using convertLine().
				instructions = convertLine(line)
				if instructions is None: continue

				for markerName, (markerLine, markerCount) in markers.items():
					if lnnum < markerLine:
						markers[markerName] = (markerLine, markerCount+len(instructions))
				lnnum += 1


		fabricated = []
		for i,line in enumerate(aliasReplaced):
			if not line.startswith(":"):
				#Convert lines, again, using convertLine().
				instructions = convertLine(line)
				if instructions is not None: fabricated.extend(instructions)

		del macros, markers
		del aliasReplaced

		
		#Make the list of hex instructions into a set of bytes.
		swForm = "d={"
		charlength = 3
		combinedHex = ""
		for idx, hexInstruction in enumerate(fabricated):
			combinedHex += hexInstruction
			if sw:
				newInstr = str(int(hexInstruction.lower(),16)) + ","
				charlength += len(newInstr)
				swForm += newInstr
				if charlength >= 8125:
					raise FabricationError(f"swForm has exceeded 8192 characters at command {idx} / {len(fabricated)} ({100*round(idx/len(fabricated),4)}%)")
		swForm += "0,3211263}function onTick()output.setNumber(1,d[input.getNumber(1)])end" #64 characters long


		if len(combinedHex) % 2 == 1: combinedHex += "0"

		combinedBytes = bytes.fromhex(combinedHex)


	with open(f"data\\{filename.split('.')[0]}.dat", "wb") as outFile:
		#Write to a file.
		outFile.write(combinedBytes)
		print(f"data\\{filename.split('.')[0]}.dat was successfully created.")

	if sw:
		with open(f"data\\{filename.split('.')[0]}.sw", "w") as swFile:
			swFile.write(swForm)
			print(f"data\\{filename.split('.')[0]}.sw was successfully created.")