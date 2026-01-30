"shared.py"

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
global graphicsMode, ROM_DATA, ROM_INDEX;
graphicsMode = "NONE";
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
class FabricationError(Exception): #Fabrication error exception.
	def __init__(self, error:str="Fabrication Failed!"):
		self.message:str = f"Fabrication Error; {error}";
		super().__init__(self.message);


class Data: #Stores data about this line.
	def _getAB(self):
		try: self.A = toBin(self.preA);
		except ValueError as e:
			print(f"Cannot convert value [A]: {e}");
			return ("",);
		except TypeError: pass; #Ignore TypeErrors.

		try: self.B = toBin(self.preB);
		except ValueError as e:
			print(f"Cannot convert value [B]: {e}");
			return ("",);
		except TypeError: pass; #Ignore TypeErrors.
	
	def __init__(self, text:str, immediates:str, preA:str, preB:str):
		self.text:str = text;
		self.immediates:str = immediates;
		self.preA:str = preA;
		self.preB:str = preB;

		self.A:str = "";
		self.B:str = "";
		self._getAB();


class Macro: #Stores data about a given macro.
	def __init__(self, name:str, params:list[str], lines:list[str]):
		self.name:str = name;
		self.params:list[str] = params;
		self.lines:list[str] = lines;
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