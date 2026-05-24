# General;
## Memory;
Has 64 registers, addressable via `rN` format (for register N)
It is acceptable to use direct register names such as `r0`, `r1`, yet "Aliasing" is reccomended (see later) instead for clarity.
Has 4096 RAM addresses, the last 512 of which are assigned to the screen.
Immediate values are represented by `#X`. `#X` and `#dX` assume signed-8b decimal values. `#bX` uses the 8-bit 2's complement binary format (such as `#b00001111`). `#xX` uses hexadecimal format (such as `#xFF`).
Special registers;
- `rOP` : Results register / Sometimes used for other commands.
- `rX/Y/Z/W` : 4 assorted other registers used for commands. Anywhere that data seems to be missing, like copying ROM into RAM, clearing RAM or copying RAM, will use these (X/Y as start/end of RAM segment 1, Z/W as start/end of RAM segment 2 (if applicable))

All values are 8-bit. Arithmetic will always use 2's complement signed 8-bit values. Addresses/I_O assume unsigned 8-bit.
Registers (internally) store signed 8 bit values, but are converted to unsigned when required (infrequently ^^)
Booleans/Conditionals are C-style, so non-zero integer values are `True`.

"ROM segments" can be assigned like so;
`ROM0 [HEX_DATA]`
This will correspond to the instruction `LOAD #0`, which also uses `rX` and `rY` as stated previously.
Allows for larger data storage without use of many individual instruction calls.
There are a maximum of 256 ROM segments, and each can contain up to 8,132 bytes (4,096 (un?)signed 8-bit values) of data. (Limited only by RAM size, as otherwise it could not copy the total in. You cannot copy PART of a segment, only all of it.) (In reality maximum the parser/file-format supports is 65,535 per, but as previously stated this is wasteful and not useful data. In the future the RAM may grow, hence the increased functional maximum.)
Text used in `PRINT` calls is stored in a ROM segment. Be aware of this. Define ROM manually at the START of the file, to ensure it is parsed first, and so does not overwrite any auto-generated ROM segments via `PRINT`.

___
# Commands;
Case optional (any char can be upper or lower). General format is (CMD A B)
- `ADD A B` (or `A + B`)
- `SUB A B` (or `A - B`)
- `MUL A B` (or `A * B`)
- `DIV A B` (or `A / B`) : if B is 0, returns 0.
- `MOD A B` (or `A % B`)
- `NOT A` (or `! A`) : Bitwise/Logical NOT
- `AND A B` (or `A && B`) : Logical AND
- `OR A B` (or `A || B`) : Logical OR
- `bAND A B` (or `A & B`) : Bitwise AND
- `bOR A B` (or `A | B`) : Bitwise OR
- `XOR A B` (or `A ^ B`) : Bitwise XOR
- `XNOR A B` (or `A !^ B`) : Bitwise XNOR
- `GRT A B` (or `A > B`)
- `LSS A B` (or `A < B`)
- `GTE A B` (or `A >= B`)
- `LSE A B` (or `A <= B`)
- `EQU A B` (or `A == B`)
- `NEQ A B` (or `A != B`)
- `SET A B` (or `A = B`) : A must be a register, B can be register or immediate value.
- `MOV A B` (or `A ~ B`) : Moves contents of A to B. Sets A to be 0.
- `INC A` (or `A ++`) : Increment A
- `DEC A` (or `A --`) : Decrement A
- `RSH A B` (or `A >> B`)
- `LSH A B` (or `A << B`)
- `NOP` : Does nothing.
- `HALT` : Ends program flow.
- `WAIT` : Waits for user input.
- `UPDATE` : Updates screen, if enabled.
- `INV A` : Inverts A (-A)
- `CLEAR A` : Clears all registers to value A.
- `RAMwrite A B` : Write `rOP` value to RAM[`(A<<8)|B`]
- `RAMread A B` : Read RAM[`(A<<8)|B`] into `rOP`
- `OUTPUT A B` : Set output to be the 16-bit value `(A<<8)|B`
- `INPUT A B` : Read 16-bits of of input (the form `(A<<8)|B`) into A and B registers.
- `JMP :LABEL` : Unconditional jump to label name `:LABEL`
- `SLEEP A` : Sleep specified number of ms
- `if (condition) then-goto :LABEL` : Conditional jump. Condition can be regular line, such as `if (r0 > r1) then-goto :end`
- `COUT A (\n?)` : Prints the value A (or value stored at A). Optional single newline character, can be omitted.
- `PRINT ""` : Print text to the console. Supports 8-bit ASCII.
- `LOAD` : Copies section of ROM into RAM.
- `COPY` : Copies section of RAM, into another area of RAM.
- `RAMclear` : Clears section of RAM.

___
# Screen;
32x16 addressable area, 512 addresses at the end of RAM.
Uses command `MODE [MODENAME]` to set screen output format.
Graphics modes;
- NONE : Default mode, outputs nothing from the SCREEN section of RAM.
- TEXT : ASCII text mode
- 256c : Uses terminal 256-colour cube indexes. (RAM interpreted as uint8)
- RGBc : Uses RRRGGGBB colour format.
Screen starts at RAM `0xE00` and ends at `0xFFF`.
To display current screen data, `UPDATE` must be called. Exception is `NONE`, as nothing occurs when `UPDATE` is called then.
`COUT`/`PRINT` does NOT require `UPDATE` to be called. They execute immediately.

___
# Macros & "Functions";
Macros act like C-style `#define` calls.
Example;
```cfab
define %addTwoNumbers $operandA $operandB $result as
	$result = ($operandA + $operandB);
	PRINT "Added two values!";
end

$number1 = #2;
$number2 = #17;
$sum @ r0;
%addTwoNumbers $number1 $number2 $sum;
```
Is equivalent to;
```cfab
$number1 = #2;
$number2 = #17;
$sum @ r0;
$sum = ($number1 + $number2);
PRINT "Added two values!";
```
Where the macro call is directly replaced by the macro contents, with all internal names replaced.

Functions presently do not have a formal syntax, but can be simulated using `JMP` calls around blocks of instructions, or macros, where applicable.
In the future proper function-calls will be implemented.

___
# Other;
Comments use C-style notation (/\*\*/ and //). They are entirely ignored by the parser.
Labels define jump points. They work similar to GCC labels, but with different formatting (:LABEL_NAME)
Aliases are like variable names. They are written `$name`, and are automatically assigned when first used. To manually set where an alias corresponds to, `$name @ rN` can be used to equate `$name` to `rN`.
`SET` does allow higher-level formatting, using brackets to surround a simple instruction (expanding out to running the bracketed portion first, then `MOV` the value into the register `A` given in the original `SET` instruction.)
Semicolons at the end of each line are NOT required, but are reccomended usually. If in the middle of a line, the line is split in two.

Premade aliases;
- $SCREEN_WIDTH/$SCREENHEIGHT (self explanatory)
- $REG_SIZE ("#x40")
- $RAM_SIZE ("#xF00")
- $SCREEN_START_INDEX ("#xE00")

___
# Example program;
This program loops 127 times, uses branching, aliasing, console output and basic arithmetic.
```cfab
/* testloop-new.cfab */

$iter = #0; //Assign new variable $iter with value #0
$numberOfIterations = #127; //Number of times to loop.

PRINT "Looping ";
COUT $numberOfIterations;
PRINT " times...";

:loopMarker

	$iter ++; //Increment
	COUT $iter \n;

if ($iter < $numberOfIterations) then-goto :loopMarker
PRINT "Loop Complete.";
```