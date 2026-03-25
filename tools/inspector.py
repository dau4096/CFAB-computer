import ttkbootstrap as ttkb;
from ttkbootstrap.constants import *
import tkinter as tk;
from tkinter import ttk, filedialog;
import os, pprint;


def hxd(value:int) -> str:
	return format(value, "02x").upper();


global ROMdataFormat;
ROMdataFormat = hxd;
global INSTR_TABLE, ROM_SEG_TABLE, ROM_DAT_TABLE, ROOT;


class Header:
	def __init__(self, headerBytes:bytes, size:int):
		self.size:int = size;

		self.ident:str = "".join([chr(int(x)) for x in headerBytes[:4]]);
		self.version:int = int(headerBytes[4]);
		self.numberOfInstructions:int = (int(headerBytes[5]) << 8) | int(headerBytes[6]);
		self.graphicsMode:str = ("NONE", "TEXT", "256c", "RGBc")[(headerBytes[7]&0xC0) >> 6];
		self.numberOfROMSegments:int = ((int(headerBytes[7]) & 0x3F) << 4) | ((int(headerBytes[8]) & 0xF0) >> 4)

		self.valid:bool = self.isValid();


	def isValid(self) -> bool:
		return (
			(self.size > 0) and
			(self.ident == "CFAB") and
			(self.version >= 2) and
			(self.numberOfInstructions > 0)
		);


	def __str__(self) -> str:
		return f"""{self.ident} file containing {self.size} Bytes.
 - Version: {self.version}
 - Contains {self.numberOfInstructions} instructions and {self.numberOfROMSegments} ROM segments
 - Graphics Enum: {self.graphicsMode}""";

	def __repr__(self) -> str: return str(self);




class Operand:
	def __init__(self, value:int, immediate:bool):
		self.value:int = value&0xFF;
		self.immediate:bool = immediate;

		self.index:int = self.value;
		if (self.value < 0):
			self.index += 256;


	def __str__(self) -> str:
		regMap:dict[int,str] = {
			63: "RESULT",
			59: "X", 60: "Y", 61: "Z", 62:"W"
		};
		if (self.immediate): return str(self.value);
		elif (self.index in regMap): return regMap[self.index]; 
		else: return f"Register {self.index}";

	def __repr__(self) -> str: return str(self);





opcodes:tuple[str] = (
	"NOP", "SET", "MOV", "ADD",
	"SUB", "MUL", "DIV", "NOT",
	"EQU", "GRT", "BRN", "I_O",
	"SHF", "EXT", "SLP", "MEM"
);
class Instruction:
	def __init__(self, instrBytes:bytes, index:int=0):
		self.index:int = index;

		self.flagBits:int = (int(instrBytes[0])&0x30) >> 4;
		self.opcode:str = opcodes[int(instrBytes[0])&0x0F];
		self.operands:tuple[Operand] = (
			Operand(value=int(instrBytes[1]), immediate=bool(int(instrBytes[0])&0x80)),
			Operand(value=int(instrBytes[2]), immediate=bool(int(instrBytes[0])&0x40))
		);

		self.function:str = self._getFunctionName();

	def _getFunctionName(self) -> str:
		functionMap:dict[str,tuple[str]] = {
			"NOP": ("No-Operation",),
			"SET": ("Set register",),
			"MOV": ("Move register",),
			"ADD": ("Addition/Logical OR", "Bitwise OR"),
			"SUB": ("Subtract",),
			"MUL": ("Multiplication/Logical AND", "Bitwise AND",),
			"DIV": ("Division", "Modulus",),
			"NOT": ("Logical NOT", "Bitwise NOT", "Numerical Inversion", "Absolute Value",),
			"EQU": ("Equals", "Not Equals", "Bitwise XOR", "Bitwise XNOR",),
			"GRT": ("Greater-Than", "Less-Than",),
			"BRN": ("Branch-if-False", "Branch-if-True", "Unconditional Jump",),
			"I_O": ("Get-Input", "Set-Output", "COUT Value", "COUT String",),
			"SHF": ("Right-Shift", "Left-Shift",),
			"EXT": ("Halt", "Fill Registers", "RAM-Write", "RAM-Read",),
			"SLP": ("Sleep [Input]", "Sleep [ms]", "Update screen [if applicable]",),
			"MEM": ("Fill RAM in Range", "Copy RAM in Range", "Load ROM Segment",)
		}
		if (
			(self.opcode in functionMap) and
			(self.flagBits < len(functionMap[self.opcode]))
		): return functionMap[self.opcode][self.flagBits]

		return "Unknown";



	def __str__(self) -> str:
		return f"Opcode: [{self.opcode}], FBs: 0b{format(self.flagBits, '02b')}, Function: '{self.function}', A: {self.operands[0]}, B: {self.operands[1]}";

	def __repr__(self) -> str: return str(self);

	def getList(self) -> list[str]:
		return [
			self.index,
			self.opcode, bin(self.flagBits),
			self.function,
			self.operands[0], self.operands[1]
		];



def getFileData(filePath:str) -> dict[str, any]:
	rawBytes:bytes = b"";
	with open(filePath, "rb") as dataFile: rawBytes = dataFile.read();
	
	HEADER_LENGTH_BYTES = 9;
	headerBytes:bytes = rawBytes[:HEADER_LENGTH_BYTES];
	meta:Header = Header(headerBytes, len(rawBytes));
	if (not meta.valid):
		raise ValueError("File is not valid CFAB file.")

	numberOfInstructionBytes:int = meta.numberOfInstructions * 3; #24b per index
	instructionBytes:bytes = rawBytes[
		(HEADER_LENGTH_BYTES):(HEADER_LENGTH_BYTES+numberOfInstructionBytes)
	];
	instructions:list[Instruction] = [];
	for index in range(meta.numberOfInstructions):
		instructions.append(Instruction(instructionBytes[(index*3):((index+1)*3)], index=index));


	numberOfROMIndexBytes:int = (meta.numberOfROMSegments+1) * 2; #16b per index
	ROMindexTableBytes:bytes = rawBytes[
		(HEADER_LENGTH_BYTES+numberOfInstructionBytes):(HEADER_LENGTH_BYTES+numberOfInstructionBytes+numberOfROMIndexBytes)
	];
	ROMindexTable:list[tuple[int]] = [];
	for segmentIDX in range(meta.numberOfROMSegments):
		ROMindexTable.append((
			(int(ROMindexTableBytes[(segmentIDX*2)+0]) << 8) | int(ROMindexTableBytes[(segmentIDX*2)+1]),
			(int(ROMindexTableBytes[(segmentIDX*2)+2]) << 8) | int(ROMindexTableBytes[(segmentIDX*2)+3]),
		));


	ROMdataBytes:bytes = rawBytes[(HEADER_LENGTH_BYTES+numberOfInstructionBytes+numberOfROMIndexBytes):];
	ROMdata:list[list[int]] = [
		[int(x) for x in ROMdataBytes[pair[0]:pair[1]]] for pair in ROMindexTable
	];


	return {
		"Metadata": meta,
		"Instructions": instructions,
		"ROM Indices": ROMindexTable,
		"ROM Data": ROMdata	
	}




def displayInstructions(results:dict[str,any]) -> None:
	for row in INSTR_TABLE.get_children(): INSTR_TABLE.delete(row);

	for (i, instruction) in enumerate(results["Instructions"]):
		tag:str = "oddrow" if (i % 2) else "evenrow"
		INSTR_TABLE.insert("", "end", values=instruction.getList(), tags=(tag,))


def displayMetadata(meta:Header) -> None:
	for widget in metadataFrame.winfo_children(): widget.destroy();

	# Display metadata as labels
	info:tuple[str] = (
		f"File Identifier: {meta.ident}",
		f"File Size: {meta.size} bytes",
		f"Version: {meta.version}",
		f"Number of Instructions: {meta.numberOfInstructions}",
		f"Number of ROM Segments: {meta.numberOfROMSegments}",
		f"Graphics Mode: {meta.graphicsMode}"
	);

	for (i, line) in enumerate(info):
		lbl = ttk.Label(metadataFrame, text=line, anchor="center");
		lbl.pack(fill="x", padx=10, pady=2);


def displayROMSegments(results:dict[str,any]) -> None:
	for row in ROM_SEG_TABLE.get_children(): ROM_SEG_TABLE.delete(row);

	for (i, ROMsegment) in enumerate(results["ROM Indices"]):
		tag:str = "oddrow" if (i % 2) else "evenrow"
		ROMsegmentDisp = (i, ROMsegment[0], ROMsegment[1], ROMsegment[1]-ROMsegment[0]);
		ROM_SEG_TABLE.insert("", "end", values=ROMsegmentDisp, tags=(tag,))


def displayROMDataGeneric(results:dict[str,any], t:type) -> None:
	for row in ROM_DAT_TABLE.get_children(): ROM_DAT_TABLE.delete(row);

	for (i, ROMdata) in enumerate(results["ROM Data"]):
		tag:str = "oddrow" if (i % 2) else "evenrow"
		ROMDataRow:tuple[any] = (
			i, len(ROMdata), [t(x) for x in ROMdata]
		);
		ROM_DAT_TABLE.insert("", "end", values=ROMDataRow, tags=(tag,))




def selectFile() -> None:
	path:str = filedialog.askopenfilename()
	if (not path): return;

	results:dict[str,any] = getFileData(path);
	displayInstructions(results);
	displayMetadata(results["Metadata"]);
	displayROMSegments(results);
	ROM_DAT_TABLE.currentResults = results;
	displayROMDataGeneric(results, ROMdataFormat);

	projIndex = path.find("Documents");
	ROOT.title(f"CFABv2 Inspector - Viewing [~/{path[projIndex:]}]");



if (__name__ == "__main__"):
	ROOT:ttkb.Window = ttkb.Window(title="CFABv2 Inspector", themename="darkly")
	ROOT.title("CFABv2 Inspector");
	ROOT.geometry("800x400");
	ROOT.minsize(800, 400);

	style = ttkb.Style();
	style.configure("Treeview", rowheight=24);
	ROW_COLOR_1:str = "#404040";
	ROW_COLOR_2:str = "#505050";

	mainContainer:ttk.Frame = ttk.Frame(ROOT);
	mainContainer.pack(fill="both", expand=True);

	instructionsFrame:ttk.Frame = ttk.Frame(mainContainer);
	metadataFrame:ttk.Frame = ttk.Frame(mainContainer);
	ROMsegmentsFrame:ttk.Frame = ttk.Frame(mainContainer);
	ROMdataFrame:ttk.Frame = ttk.Frame(mainContainer);
	for frame in (
		instructionsFrame, metadataFrame, ROMsegmentsFrame, ROMdataFrame,
	): frame.place(relx=0, rely=0, relwidth=1, relheight=1);

	tabBar = ttk.Frame(ROOT);
	tabBar.pack(side="bottom", fill="x");

	def bringROMdataFrameToFront(t:type) -> None:
		global ROMdataFormat;
		ROMdataFormat = t;
		ROMdataFrame.tkraise();
		if (hasattr(ROM_DAT_TABLE, "currentResults")):
			displayROMDataGeneric(ROM_DAT_TABLE.currentResults, ROMdataFormat)


	ttk.Button(tabBar, text="Instructions", command=lambda: instructionsFrame.tkraise()).pack(side="left");
	ttk.Button(tabBar, text="Metadata", command=lambda: metadataFrame.tkraise()).pack(side="left");
	ttk.Button(tabBar, text="ROM Segments", command=lambda: ROMsegmentsFrame.tkraise()).pack(side="left");
	ttk.Button(tabBar, text="ROM Data [HEX]", command=lambda: bringROMdataFrameToFront(hxd)).pack(side="left");
	ttk.Button(tabBar, text="ROM Data [INT]", command=lambda: bringROMdataFrameToFront(int)).pack(side="left");
	ttk.Button(tabBar, text="ROM Data [STR]", command=lambda: bringROMdataFrameToFront(chr)).pack(side="left");
	ttk.Button(tabBar, text="Select File", command=selectFile).pack(side="right");
	instructionsFrame.tkraise();



	instrTableScrollY:ttk.Scrollbar = ttk.Scrollbar(instructionsFrame, orient="vertical");
	instrColumns:dict[str,int] = {"Index":32, "Mneumonic":32, "Flags":32, "Function":96, "Operand A":32, "Operand B":32};
	INSTR_TABLE:ttk.Treeview = ttk.Treeview(
		instructionsFrame,
		columns=list(instrColumns.keys()),
		show="headings",
		yscrollcommand=instrTableScrollY.set
	);
	instrTableScrollY.config(command=INSTR_TABLE.yview);
	instrTableScrollY.pack(side="right", fill="y");
	INSTR_TABLE.pack(fill="both", expand=True);
	for (col,x) in instrColumns.items():
		INSTR_TABLE.heading(col, text=col);
		INSTR_TABLE.column(col, width=x, anchor=(tk.CENTER if (x == 32) else tk.W));
	INSTR_TABLE.tag_configure("evenrow", background=ROW_COLOR_1);
	INSTR_TABLE.tag_configure("oddrow",  background=ROW_COLOR_2);




	ROMsegmentsTableScrollY:ttk.Scrollbar = ttk.Scrollbar(ROMsegmentsFrame, orient="vertical");
	ROMsegColumns:dict[str,int] = {"Index":32, "Start Index":32, "End Index":32, "Size":32};
	ROM_SEG_TABLE:ttk.Treeview = ttk.Treeview(
		ROMsegmentsFrame,
		columns=list(ROMsegColumns.keys()),
		show="headings",
		yscrollcommand=ROMsegmentsTableScrollY.set
	);
	ROMsegmentsTableScrollY.config(command=ROM_SEG_TABLE.yview);
	ROMsegmentsTableScrollY.pack(side="right", fill="y");
	ROM_SEG_TABLE.pack(fill="both", expand=True);
	for (col,x) in ROMsegColumns.items():
		ROM_SEG_TABLE.heading(col, text=col);
		ROM_SEG_TABLE.column(col, width=x, anchor=tk.CENTER);
	ROM_SEG_TABLE.tag_configure("evenrow", background=ROW_COLOR_1);
	ROM_SEG_TABLE.tag_configure("oddrow",  background=ROW_COLOR_2);




	ROMsegmentsTableScrollY:ttk.Scrollbar = ttk.Scrollbar(ROMdataFrame, orient="vertical");
	ROMsegmentsTableScrollX:ttk.Scrollbar = ttk.Scrollbar(ROMdataFrame, orient="horizontal");
	ROMdatColumns:dict[str,int] = {"Segment Index":32, "Size":32, "Data":256};
	ROM_DAT_TABLE:ttk.Treeview = ttk.Treeview(
		ROMdataFrame,
		columns=list(ROMdatColumns.keys()),
		show="headings",
		yscrollcommand=ROMsegmentsTableScrollY.set,
		xscrollcommand=ROMsegmentsTableScrollX.set
	);
	ROMsegmentsTableScrollY.config(command=ROM_DAT_TABLE.yview);
	ROMsegmentsTableScrollY.pack(side="right", fill="y");
	ROMsegmentsTableScrollX.config(command=ROM_DAT_TABLE.xview);
	ROMsegmentsTableScrollX.pack(side="bottom", fill="x");
	ROM_DAT_TABLE.pack(fill="both", expand=True);
	for (col,x) in ROMdatColumns.items():
		ROM_DAT_TABLE.heading(col, text=col);
		ROM_DAT_TABLE.column(col, width=x, anchor=(tk.CENTER if (x == 32) else tk.W), stretch=(x!=32));
	ROM_DAT_TABLE.tag_configure("evenrow", background=ROW_COLOR_1);
	ROM_DAT_TABLE.tag_configure("oddrow",  background=ROW_COLOR_2);







	ROOT.mainloop();