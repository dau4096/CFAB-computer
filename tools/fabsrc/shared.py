"shared.py"
from typing import Callable;
from dataclasses import dataclass;
from enum import Enum;


#### CONSTANTS ####
NUM_REGISTERS:int = 64;
BUILTIN_REGISTERS:dict[str, int] = {
	"op": 63,
	"x": 60, "y": 61,
	"z": 62, "w": 59
};
MAX_MACRO_UNPACKING:int = 32; #Number of macros that can be unpacked within other macros.
MAX_MACRO_LENGTH:int = 256; #Max lines in a macro
#### CONSTANTS ####


#### GLOBAL ####
global GRAPHICS_MODE, ROM_DATA, ROM_INDEX;
GRAPHICS_MODE = "NONE";
ROM_INDEX:list[int] = [] #Start index for this segment [16b]*. Needs to contain ROM_NUMBER + 1.
ROM_DATA:list[int] = []; #Static data, such as long text strings. [8b]*
#### GLOBAL ####




#### FUNCTIONS ####
def toBin(value:int) -> str:
	"Converts some integer value into its signed 8b representation. [STR]";

	absValue:int = abs(value) & 0x7F; #7 bits
	if (value >= 0): #Positive values
		return "0" + str(bin(absValue)[2:]).zfill(7);

	else: #Negative values - uses 2's complement.
		#Redo?
		places:tuple[int] = (
			-128, 64, 32,
			16, 8, 4, 2, 1
		); 
		binRep:str = "1";
		recreatedValue:int = places[0];
		for place in places[1:]:
			if ((recreatedValue+place) < value):
				binRep += "1";
				recreatedValue += place;
			else:
				binRep += "0";

		return binRep;
#### FUNCTIONS ####




#### CLASSES ####
class OperationType(Enum): #Used to distinguish types of instruction.
	ZERO_OPERANDS = 1;
	ONE_OPERAND   = 2;
	TWO_OPERAND   = 3;
	COMPLEX       = 4;
	ONLY_A_OPRNDS = 5;
	ONLY_B_OPRNDS = 6;


class FabricationError(Exception): #Fabrication error exception.
	def __init__(self, error:str="Fabrication Failed!"):
		self.message:str = f"Fabrication Error; {error}";
		super().__init__(self.message);


@dataclass
class ParsedLine:
	operator:str = "";
	operands:tuple[str] = ();
	valid:bool = False;
	convertedOperands:tuple[int] = ();
	immediates:str = "";
	src:str = "";
INVALID_PARSED_LINE = ParsedLine(operator="", operands=[], valid=False, convertedOperands=(), immediates="", src="");


@dataclass
class Macro: #Stores data about a given macro.
	name:str;
	params:tuple[str];
	lines:tuple[str];


class Data: #Stores data about this line.
	def _getAB(self):
		try: self.A = toBin(int(self.preA));
		except ValueError as e:
			pass; #print(f"Cannot convert value [A]: {e}");
		except TypeError: pass; #Ignore TypeErrors.

		try: self.B = toBin(int(self.preB));
		except ValueError as e:
			pass; #print(f"Cannot convert value [B]: {e}");
		except TypeError: pass; #Ignore TypeErrors.
	
	def __init__(self, parsedLine:ParsedLine):
		self.text:str = parsedLine.src;
		self.immediates:str = parsedLine.immediates;
		self.preA:str = parsedLine.convertedOperands[0];
		self.preB:str = parsedLine.convertedOperands[1];

		self.A:str = "";
		self.B:str = "";
		self._getAB();


class Operation:
	def __init__(self, type:OperationType, func:Callable):
		self.type:OperationType = type;
		self.func:Callable = func;

	def __call__(self, ln:Data) -> list[str]: #Let you call the Operation as usual.
		return self.func(ln);
#### CLASSES ####




#### OTHER CONSTANTS ####
BLANK:str = "00000000";
REG_RESULT:str = toBin(BUILTIN_REGISTERS["op"]);
REG_X:str = toBin(BUILTIN_REGISTERS["x"]);
REG_Y:str = toBin(BUILTIN_REGISTERS["y"]);
REG_Z:str = toBin(BUILTIN_REGISTERS["z"]);
REG_W:str = toBin(BUILTIN_REGISTERS["w"]);

NEW_LN:str = toBin(0x0A); #0x0A is the decimal rep for "\n" [NEWLINE_CHAR].
ZERO:str = toBin(0);
ONE:str = toBin(1);
FALSE:str = ZERO;
TRUE:str = ONE;
#### OTHER CONSTANTS ####