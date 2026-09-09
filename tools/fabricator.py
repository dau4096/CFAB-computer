"fabricator.py";
import sys;
import re as regex;

from fabsrc import shared, macro, alias, translate;
from fabsrc.shared import FabricationError;




def getHeader(numberOfInstructions:int, numberOfROMSegments:int=1) -> str:
	"Gets the header for this file, containing some metadata and an identifier string."

	def STRtoBinary(s:str) -> str: return "".join([format(ord(x)&0xFF, "08b") for x in s]);
	def INTtoBinary(i:int, bits:int) -> str: return f"{i & ((1 << bits) - 1):0{bits}b}";
	def BINtoHexade(binary: str) -> str: return format(int(binary, 2), f"0{len(binary)//4}x");


	HEADER_LENGTH_BITS = 72; #MUST be multiple of 4[bits].


	modeMap:tuple[str] = ("NONE", "TEXT", "256C", "RGB");
	modeIndex:int = 0; #Default is "NONE"
	if (shared.GRAPHICS_MODE.upper().replace("RGBC","RGB") in modeMap):
		modeIndex = modeMap.index(shared.GRAPHICS_MODE.upper().replace("RGBC","RGB")) & 0x3; #2 bits.


	#Header parts
	IDENT:str = STRtoBinary("CFAB"); #32 bits.
	VERSION:str = INTtoBinary(4, bits=8); #Version 4 of this CFAB format. [CFABv2]. 8 bits.
	INSTR:str = INTtoBinary(numberOfInstructions, bits=16); #16 bits.
	MODE:str = INTtoBinary(modeIndex, bits=2); #2 bits.
	ROMSEGS:str = INTtoBinary(numberOfROMSegments, bits=10); #10 bits.


	totalUsed:int = len(IDENT)+len(VERSION)+len(INSTR)+len(MODE)+len(ROMSEGS);
	PADDING:str = "0" * (HEADER_LENGTH_BITS-totalUsed); #Pad to HEADER_LENGTH_BITS.

	#Convert to HexaDe(cimal)
	return BINtoHexade(IDENT + VERSION + INSTR + MODE + ROMSEGS + PADDING);






def fabricate(src:list[str]) -> tuple[bytes, bytes, bytes, bytes]:
	"Converts CFAB source into bytes to be interpreted.";
	#Get graphics mode;
	for line in src:
		if (line.lower().startswith("mode")):
			lineSplit:list[str] = line.split(" ");
			if (len(lineSplit) < 2): raise FabricationError("Graphics mode line must be formatted: 'MODE [NONE|TEXT|256c|RGB]'.");
			shared.GRAPHICS_MODE = lineSplit[1]; #Set file-wide.
			break; #Only accept the 1st def.
	src = [ln for ln in src if (not ln.lower().startswith("mode"))];


	macrosReplaced:list[str] = macro.replaceMacros(src);
	aliasReplaced = alias.replaceAliases(macrosReplaced);
	del macrosReplaced;


	#Process BRANCH markers
	translate.processMarkers(aliasReplaced);

	#Acctually convert lines to fabricated hex now.
	instructionHexList:list[str] = [];
	for line in aliasReplaced:
		if (not line.startswith(":")): #Non-marker lines.
			#Convert lines using convertLine(), and convert to Hexadecimal.
			instructionHexList.extend(translate.convertLine(line, makeHex=True, convertMarkers=True)) #1 line of CFAB src can correspond to multiple instructions
	del aliasReplaced;



	#Add the END index to the ROM_INDEX set.
	sumIndex:int = 0;
	for romDat in shared.ROM_DATA:
		shared.ROM_INDEX.append(format(sumIndex, "04x"));
		sumIndex += len(romDat);
	shared.ROM_INDEX.append(format(sumIndex, "04x")); #16-bit.
	


	#Make the list of hex instructions into a set of bytes.
	numberOfInstructions:int = len(instructionHexList); #6 hex values per instr (3 bytes)
	instructionHex:str = "".join(instructionHexList);

	#Get file header;
	headerHex:str = getHeader(
		numberOfInstructions,
		max(len(shared.ROM_INDEX)-1,0) #Number of ROM segments. Will always have 1 extra for the END index.
	);

	#Deal with ROM data/indices at E.O.F.
	ROMindexHex:str = "".join(shared.ROM_INDEX);
	ROM_DATA_FLAT:list[int] = [];
	for ROM_SEGMENT in shared.ROM_DATA: ROM_DATA_FLAT.extend(ROM_SEGMENT);
	ROMdataHex:str = "".join([format(x, "02x") for x in ROM_DATA_FLAT]); #8-bit.


	if (len(instructionHex) % 2): instructionHex += "0"; #Even length.
	headerBytes:bytes = bytes.fromhex(headerHex);
	instructionBytes:bytes = bytes.fromhex(instructionHex);
	ROMindexBytes:bytes = bytes.fromhex(ROMindexHex);
	ROMdataBytes:bytes = bytes.fromhex(ROMdataHex);


	#Return the byte sections.
	return (headerBytes, instructionBytes, ROMindexBytes, ROMdataBytes, numberOfInstructions);





if (__name__ == "__main__"):
	"Converts CFAB source file to into a CFAB data file to be interpreted.";

	inFileName:str = "help.cfab"; #Fallback filename, if none provided.
	if len(sys.argv) > 1: #Was given some argument(s). Treat as file(names).
		inFileName = sys.argv[1];
		if (len(sys.argv) > 2):	outFileName = sys.argv[2]; #Ditto but with seperate out-file name.
		else: outFileName = inFileName.replace(".cfab", ".dat");
	else: outFileName = inFileName.replace(".cfab", ".dat");


	#Read from the src CFAB file.
	print(f"Reading: cfab/{inFileName}");
	src:list[str] = [];
	with open(f"../cfab/{inFileName}", "r") as CFABFile:
		raw:str = CFABFile.read();
		raw = regex.sub(
			r"(\/\*[\s\S]*?\*\/)|(\/\/.*\n)", #Matches multiline comments "/* ... */" And normal comments "//..."
			"", #Replace with nothing
			raw
		); #Remove comments from raw.
		rawLines:list[str] = [ln for ln in raw.replace(";","\n").split("\n") if len(ln)]; #Seperate on semicolons or newlines.
		partialLines:list[str] = [line.strip() for line in rawLines];
		src = [line for line in partialLines if line != ""];
		del rawLines, partialLines;


	#Convert to fabricated bytes.
	(headerBytes, instructionBytes, ROMindexBytes, ROMdataBytes, numberOfInstructions) = fabricate(src);
	totalBytes:int = len(instructionBytes) + len(ROMindexBytes) + len(ROMdataBytes); #Find total bytes (Excluding header)
	print(f"Fabrication complete.\nWrote {totalBytes} bytes [{numberOfInstructions} instructions] to data/{outFileName}");


	#Write to the out-file.
	with open(f"../data/{outFileName}", "wb") as outFile:
		outFile.write(headerBytes);
		outFile.write(instructionBytes);
		outFile.write(ROMindexBytes);
		outFile.write(ROMdataBytes);