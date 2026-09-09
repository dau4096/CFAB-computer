"sw-reencode.py"

import sys, os;
from pathlib import Path;
from dataclasses import dataclass;


HEADER_LENGTH:int = 9; #9 Bytes, 72 bits.
INSTR_LENGTH:int = 3; #3 Bytes, 24 bits.
ROM_INDEX_LENGTH:int = 2; #2 Bytes, 16 bits.


@dataclass
class Header:
	ident:str;
	version:int;
	numberOfInstructions:int;
	graphicsMode:int;
	numberOfROMSegments:int;



def readHeader(headerBytes:bytes) -> Header:
	ident:str = "".join([
		chr(headerBytes[i]) for i in range(4)
	]);

	version:int = headerBytes[4];
	numberOfInstructions:int = (headerBytes[5] << 8) | headerBytes[6];
	graphicsMode:int = headerBytes[7] >> 6;
	numberOfROMSegments:int = ((headerBytes[7] & 0x3F) << 2) | (headerBytes[8] >> 4)
	
	return Header(
		ident, version,
		numberOfInstructions,
		graphicsMode,
		numberOfROMSegments
	);



def readInstrs(fileData:bytes, header:Header) -> bytes:
	return fileData[
		HEADER_LENGTH : HEADER_LENGTH + (header.numberOfInstructions * INSTR_LENGTH)
	];


def readROM(fileData:bytes, header:Header) -> [list[tuple[int, int]], bytes]:
	instrsEnd:int = HEADER_LENGTH + (header.numberOfInstructions * INSTR_LENGTH);
	ROMdataStart:int = instrsEnd + (header.numberOfROMSegments + 1) * ROM_INDEX_LENGTH;

	return fileData[instrsEnd:ROMdataStart], fileData[ROMdataStart:];



def encodeBytes(data:bytes) -> str:
	encoded:str = "";
	hexa = "0123456789ABCDEF";

	for byte in data:
		encoded += hexa[byte >> 4];
		encoded += hexa[byte & 0xF];

	if (len(encoded) > 4000): raise Exception(f"Exceeded 4000 char limit: {len(encoded)}");
	return encoded;



if (__name__ == "__main__"):
	"Converts CFAB data files into SW-formats.";

	inFileName:str = "help"; #Fallback filename, if none provided
	if (len(sys.argv) > 1): #Was given some argument(s). Treat as file(names)
		inFileName = sys.argv[1];

	print(f"Converting: ../data/{inFileName}.dat");

	fileData:bytes = b'';
	with open(f"../data/{inFileName}.dat", "rb") as CFABFile:
		fileData = CFABFile.read();

	
	header:Header = readHeader(fileData[:HEADER_LENGTH]);
	
	instructionBytes:bytes = readInstrs(fileData, header);
	ROMIndicesBytes, ROMDataBytes = readROM(fileData, header);

	try:
		instructionEncoded:str = encodeBytes(instructionBytes);
		ROMIndicesEncoded:str = encodeBytes(ROMIndicesBytes);
		ROMDataEncoded:str = encodeBytes(ROMDataBytes);
	except Exception:
		print("[FAIL] Ran out of free space.");
		sys.exit(-1);

	pth:Path = Path(".") / "sw" / inFileName;
	if (not pth.is_dir()):
		os.mkdir(pth.resolve());


	with open(pth / "instrs.txt", "w") as instrFile:
		instrFile.write(instructionEncoded);
	with open(pth / "ROM.segs.txt", "w") as segsFile:
		segsFile.write(ROMIndicesEncoded);
	with open(pth / "ROM.data.txt", "w") as dataFile:
		dataFile.write(ROMDataEncoded);


	print("[SUCCESS] Completed re-encoding.");
	print(f"Saved outfiles to ./sw/{inFileName}/");
	sys.exit(1);
