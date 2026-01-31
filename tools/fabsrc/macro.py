"macro.py"
from fabsrc import shared;
from fabsrc.shared import FabricationError;


global MACROS;
MACROS:dict[str, shared.Macro] = {}; #Maps macro name to 


def replaceMacros(lines:list[str], depth:int=0, activeMacros:set[str]=None, previousMacro:str|None=None) -> list[str]:
	global MACROS;
	if activeMacros is None: activeMacros = set(); #Create set to keep track of macros being expanded.
	macrosReplaced:list[str] = [];


	#Check for excessive unpacking depth to prevent absurdly long chaining.
	if (depth > shared.MAX_MACRO_UNPACKING):
		raise FabricationError(f"Exceeded maximum macro unpacking depth ({MAX_MACRO_UNPACKING})")



	for lineNum in range(len(lines)):
		curLine:str = lines[lineNum];


		#Macro has been defined.
		if (curLine.startswith("define")):
			macroData:list[str] = curLine.split(" ");
			if (len(macroData) < 3): #"define %name ($args)* as"
				raise FabricationError(f"Macro definition missing name and/or parameters: {curLine}");


			macroName:str = macroData[1].replace("%", "");
			macroParams:list[str] = macroData[2:-1];
			macroLines:list[str] = [];

			#Save the contents of the macro.
			for i in range(shared.MAX_MACRO_LENGTH):
				macroLine:str = lines[lineNum + i];
				if (macroLine.startswith("end")): #End the macro definition
					lineNum += i;
					MACROS[macroName] = shared.Macro(name=macroName, params=macroParams, lines=macroLines); #Store this macro.
					break;
				macroLines.append(macroLine);


		#Macro has been called.
		elif curLine.startswith("%"):
			lineData:list[str] = curLine.split(" ");
			macroName:str = lineData[0][1:];

			#Check the macro was defined beforehand.
			if (macroName not in MACROS):
				raise FabricationError(f"Macro %{macroName} is not defined.")

			#Prevent macros from making infinite recursive loops.
			if macroName in activeMacros:
				errorMessage:str = "";
				if (previousMacro is None):
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName}."
				elif (previousMacro == macroName):
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName} within itself."
				else:
					errorMessage = f"Infinite Recursion; Attempted to unpack %{macroName} within %{previousMacro}.\nThis resulted in a chain of MACROS, in a loop."
				raise FabricationError(errorMessage);



			#Begin expansion of macro
			macro:shared.Macro = MACROS[macroName];
			if len(lineData[1:]) != len(macro.params):
				raise FabricationError(f"Macro {macro.name} expects {len(macro.params)} arguments, but got {len(lineData[1:])}")

			#Change macro args to their called counterparts.
			paramMapping:dict[str, str] = dict(zip(macro.params, lineData[1:]));
			activeMacros.add(macro.name);  #Track macro to prevent re-expansion later


			#Expand the macro to the line it was called at.
			expandedLines:list[str] = [];
			for macroLine in macro.lines:
				processedLine:str = macroLine;
				for (param, arg) in paramMapping.items():
					processedLine = processedLine.replace(param, arg); #Replace parameters with arguments for this macro invocation.
				expandedLines.append(processedLine);


			#If a macro was found inside this macro, recursively call this func to unpack that too.
			macrosReplaced.extend(replaceMacros(expandedLines, depth + 1, activeMacros, macro.name));
			activeMacros.remove(macro.name);


		#Any other lines.
		else:
			macrosReplaced.append(curLine);



	del MACROS;
	return macrosReplaced;
