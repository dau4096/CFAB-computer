"alias.py"
import re as regex;
from fabsrc import shared;
from fabsrc.shared import FabricationError;



def replaceAliases(lines:list[str]) -> list[str]:
	global graphicsMode;

	aliases:dict[str,int] = {};
	aliasReplaced:list[str] = [];
	unassignedAliases:set[str] = set();

	#Find aliases
	"""
	Define aliasing for register names like so;
	 varName  @ r1
	Every time varName is written, it is replaced by r1 by the fabricator.
	Allows for nicer formatting of CFAB.
	Can also be defined without explicit register address, and will be automatically assigned an address.
	"""
	for (lineNum, curLine) in enumerate(lines):
		if (regex.match(r"(?i)^MODE\s.*$", curLine) is not None):
			#Line contains the graphicsMode value.
			graphicsMode = curLine.split(" ")[1].upper();


		operands:list[str] = curLine.split(" ");
		for operand in operands: #Find aliases in the current line.
			res:regex.match = regex.match(rf"(?i)\$[a-z0-9]+(?=$|\W)", operand);
			if (res is not None): #Alias found
				unassignedAliases.add(res.group(0));



	availableRegisters:list[str] = [f"r{x}" for x in range(shared.NUM_REGISTERS)]; #Does not include rOP, rX, rY, rZ and rW (r59-63) as they should NEVER be overwritten.
	builtinRegisterAliases:dict[str,str] = {};
	for (k,v) in shared.BUILTIN_REGISTERS.items():
		rIndex:str = f"r{v}";
		builtinRegisterAliases[f"r{k}"] = rIndex; #Add builtin register aliases to a dict.
		availableRegisters.remove(rIndex); #Don't let it assign over builtin register aliases.



	for (lineNum, curLine) in enumerate(lines):
		#Defined alias explicitly
		operands:list[str] = curLine.split(" ");
		if ((len(operands) == 3) and (operands[1] == "@")): #Assigning an alias explicitly
			#e.g. "$iteration @ r3"
			aliasName:str = operands[0].replace("$", "");
			aliases[aliasName] = operands[2]
			unassignedAliases.remove(operands[0]); #Remove alias from unassigned list, user defined.

			#Find the corresponding register name (e.g. "r8") and remove it from available registers.
			idx:str = "";
			if (operands[2].lower() in builtinRegisterAliases): idx = builtinRegisterAliases[operands[2].lower()];
			else: idx = operands[2];
			availableRegisters.remove(idx); 



	#Assign implicit aliases to registers.
	#Implicit aliases have no "$iteration @ r3" style line, and have their memory addresses assigned automatically.
	if (len(unassignedAliases) > len(availableRegisters)): #If there's more implicit aliases used than registers unoccupied.
		raise FabricationError(f"Too many assigned aliases: {len(aliases)+len(unassignedAliases)}. Can have at most, {shared.NUM_REGISTERS}.");
	for (alias, register) in zip(unassignedAliases, availableRegisters):
		aliases[alias.replace("$", "")] = register; #Assign register to alias.



	#Replace aliases in the line
	for (lineNum, curLine) in enumerate(lines):
		operands:list[str] = curLine.split(" ");
		if ((len(operands) == 3) and (operands[1] == "@")): continue; #Ignore alias def lines.

		fixedLine:str = regex.sub(
			rf"(?i)rop(?=$|\W)",
			builtinRegisterAliases["rop"], #Result register
			curLine
		);
		for alias, register in aliases.items():
			fixedLine = regex.sub(
				rf"\${alias}(?=$|\W)",
				register, #Any user-defined aliases.
				fixedLine
			);

		aliasReplaced.append(fixedLine);


	return aliasReplaced;