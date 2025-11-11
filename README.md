
# CFAB-Interpreter
_Simple mock-up 16 instruction assembly language with interpreter._

## General Information;
- All instructions use either register locations or immediates for A and B, unless it is SET or MOV.
- All values stored in registers are -128 → 127 (signed 8 bit int)
- There are 64 registers to use.
- CFAB Files have a 65,536 instruction limit (past that value, JMP, BRN and EXT stop functioning correctly.)
- 1 CFAB Command ≠ 1 instruction. Some lines may "expand" and become multiple, unless explicitly listed in the instruction set.
- Capitalisation of instruction/command words is optional. `SET` == `set` == `sET` == etc...

## Important Registers;
- __Result Register [rOP/r63]:__ Contains result of previous operation. Reccomended not to store values here as it will usually be overwritten.

## Data-processing Commands;
| _Name_ | _Shorthand_ | _Description_ |
| :---: | :---: | :--- |
| `SET` | `A = #B` | Sets a register, A, to the immediate signed 8-bit integer, B. If B is a register, copies contents. |
| `MOV` | `A ~ B` | Moves the contents of register A to register B. A and B cannot be immediate. |
| `AND` | `A & B` | Bitwise A and B. |
| `OR ` | `A \| B` | Bitwise A or B. |
| `NOT` | `! A` | Logical not A. |
| `XOR` | `A ^ B` | Bitwise A xor B. |
| `XNOR` | `A ~^ B` | Bitwise A xnor B. |
| `EQU` | `A == B` | If A is equal to B. |
| `NEQ` | `A != B` | If A is not equal to B. |
| `GRT` | `A > B` | If A is greater than B. |
| `LSS` | `A < B` | If A is less than B. |
| `GTE` | `A >= B` | If A is greater than OR equal to B. |
| `LSE` | `A <= B` | If A is less than OR equal to B. |
| `INV` | `inv A` | Inverts the value of A. (int -> -int) |
| `ADD` | `A + B` | Adds the values of A and B. |
| `SUB` | `A - B` | Subtracts B from A. |
| `MUL` | `A * B` | Multiplies A and B. |
| `DIV` | `A / B` | Divides A by B (rounds DOWN) |
| `MOD` | `A % B` | Finds A mod B. (Residue of A / B) |
| `ABS` | `abs A` | Absolute value of A. |
| `SGN` | `sgn A` | Sign of A. |
| `BRN` | `brn :marker A` | Branches if A is true, to the line :marker is on. |
| `JMP` | `jmp :marker` | Jumps unconditionally to :marker. |
| `HALT` | `halt` | Immediately` exits the processing. |

## QOL/Advanced commands;
| _Name_ | _Notation_ | _Description_ |
| :---: | :---: | :--- |
| `BRANCH` | `IF (condition) then(-goto)` | BRN but with more friendly formatting. Optional usage of "then" or "then-goto". |
| `MACRO` | `DEFINE %macroName $arg0 $arg1 .. $argN as ... end` | Defines a macro, which when called such as `%macroName r0 r1 .. rN` will replace said line and expand into the ... lines. |
| `MARKER` | `:markerName` | Marker used for BRN/JMP/IF()THEN. Will always jump to the instruction following the marker. |


## The Instruction-set;
Flag-bits are not currently definable in files; usually will be present as alternate names. For instance, SHF+FB1 is `RSH` in CFab.
Default FB0 commands can be referenced by directly writing [Mneumonic] [operand A] [Operand B]
| _Mneumonic_ | _Hex_ | _Function_ |
| :---: | :---: | :--- |
| `NOP` |  0  | Blank, No-Op instruction |
| `SET` |  1  | Sets a register to some value. If 2nd value is reg, copy data. |
| `MOV` |  2  | Moves contents of 1 register to another, resets source register to 0. |
| `ADD` |  3  | FB0, Adds 2 values. Can also be used as logical OR. FB1, bitwise OR. |
| `SUB` |  4  | Subtracts 2 values. |
| `MUL` |  5  | FB0, Multiplies 2 values. Can also be used as logical AND. FB1, bitwise AND. |
| `DIV` |  6  | Int-Divides 2 values. If denominator is 0, result is 0. (later -> div0 flag?) |
| `NOT` |  7  | FB0, logical NOT. FB1, bitwise NOT. FB2, invert number. FB3, abs(number) |
| `EQU` |  8  | FB0, A==B. FB1, A!=B. (!= is equivalent to XOR.) FB2, bitwise XOR. FB3, bitwise XNOR. |
| `GRT` |  9  | FB0, A>B. FB1, A<B. FB2, A>=B. FB3 A<=B. (bit 2 changes inclusivity, bit 1 changes func .) |
| `BRN` |  A  | FB0, Branch to (A<<8)|B if rOP!=0. FB1, unconditional branch to (A<<8)|B. FB2, same as FB0 but rOP==0. |
| `I_O` |  B  | FB0, Gets input 16, writing 8 to A and 8 to B [A/B MUST BE REGISTERS]. FB1, same but outputs immediate/register 16b. FB2, prints some value to console. FB3, prints ASCII char by index. |
| `SHF` |  C  | FB0, Left-shifts A by B bits. FB1, Right-shifts A by B bits. |
| `EXT` |  D  | Extra; FB0, halt. FB1, Clear all registers. FB2, write to RAM. FB3, read from RAM. [RAM uses rOP for read/write value.] |
| `SLP` |  E  | Sleep; FB0, sleep for (A<<8)|B milliseconds. FB1, sleeps until user input (should be paired with I_O call after) |
| `__F` |  F  | Presently Unused. |


## Command arguments (for compiled interpreter program)
| _Name_ | _Shorthand_ | _Function |
| :---: | :---: | :--- |
| `--file` | `-f` | Specifies fabricated CFAB data file to be used, relative to dir (data/) beside the interpreter.
| `--verbose` | `-v` | Lists every instruction executed by the interpreter. |
| `--rate` | `-r` | Shows total instructions executed, time taken and instr/sec. |
| `--test` | `-t` | Runs standardised unit tests. |
| `--debug` | `-d` | Dev-only debug mode. Runs hardcoded func in interpreter. |
| `--peek` | `-p` | Allows you to read memory addresses (such as `r 0` or `ram 12`) once program `HALT`s. |
