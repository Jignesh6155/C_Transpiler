# C_Transpiler
C Transpiler that converts a made up language with specific syntax rules into C for compilation and execution. 

Project Description
This project involves creating a C11 program called runml, which processes programs written in a custom mini-language named ml. Unlike large-scale programming languages like Python, Java, or C, mini-languages are lightweight and often embedded in other programs. Examples of mini-languages include macros in MS-Excel or the Unix command-line program bc.

ml is unrelated to the well-known ML (Meta Language) or Machine Learning. Instead, it serves as a simplified language for this project, showcasing the concept of transpilation—translating a program from one language (ml) to another (C11) before compiling and executing it.


The goal of this project is to:

Validate the syntax of an ml program.
Translate valid ml code to C11.
Compile the generated C11 code.
Execute the compiled program.

Features of the ml Language
General Syntax
Programs are written in plain text files with a .ml extension.
Statements are written one per line (no terminating semicolons).
Comments start with # and extend to the end of the line.
Only real numbers (e.g., 2.71828) are supported as a data type.

Identifiers
Consist of 1 to 12 lowercase alphabetic characters (e.g., variable1).
A maximum of 50 unique identifiers can be used in a program.
Variables are implicitly initialized to 0.0 and do not require prior declaration.

Special Variables
Command-line arguments can be accessed as variables arg0, arg1, etc., containing real numbers.

Functions
Must be defined before being called.
Parameters and identifiers within a function are local to that function.
Each statement in a function body is indented with a tab character.

Program Execution
Statements are executed top-to-bottom.
Function calls are the only control-flow mechanism (no loops or conditionals).

Steps to Compile and Execute an ml Program
Create an ml program (e.g., program.ml).
Pass the .ml file as a command-line argument to the runml program.
runml validates the syntax of the ml program, reporting any errors.
If valid, runml generates a C11 source file (e.g., ml-12345.c).
The generated C11 file is compiled using the system's C11 compiler.
The compiled program is executed, passing any optional command-line arguments as real numbers.
runml removes any temporary files it created.


Project Requirements
Language and Structure:

The program must be written in C11.
Implemented in a single source file named runml.c.
Command-Line Behavior:

The program must validate its arguments.
Display a usage message on error.
Print standard output to stdout and errors to stderr.
Terminate with an appropriate exit status.
Libraries:

Use only system-provided libraries (C11, POSIX, OS functions).
Third-party libraries are not allowed.
Error Handling:

Report syntax errors in invalid ml programs via stderr, with lines starting with !.
Detect all invalid ml programs except invalid expressions (expression validation is not required).
Output:

The only "true" output of the compiled program is from executing ml's print statements.
Debug output should start with @.
Print numbers:
Exact integers: no decimal places.
Non-integers: exactly 6 decimal places.
Memory Management:

Dynamic memory allocation (e.g., malloc) is optional but not required.
The project can be completed without using dynamic memory.


input File: example.ml

# This is a sample ml program
x <- 3.14159
y <- 2.0
result <- x * y
print result

Execution
./runml example.ml
