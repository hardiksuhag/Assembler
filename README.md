# SIC/XE Assembler

## Compilation Environment
Use any linux distribution with GNU compiler on or after C++ 14 ( >= GCC 6.3).  

## Usage
    $g++ assembler.cpp -o assembler  
    $./assembler COPY.txt  

## Intermediate Files
    $cat intermediate_file.txt  
    $cat assembly_listing.txt  
    $cat output_object_program.txt  
Use these to view 'Intermediate File', 'Assembly listing' or 'Output Object Program'.  

## ASSEMBLER SETTINGS
    1) Warn for blank lines
    2) Tab size
    3) Default program name
    4) Interm. File name
    5) Object program name
All these settings can be set in macros section of the assembler code.

## ERROR SYNOPSIS
My assembler shows a wide variety errors and warnings. They are mentioned here in brief.

### File Related Errors
    1) If no input file provided
    2) If can't find input file
    3) If can't locate intermediate file

### Pass - 1 Errors
    1) Start instruction errors
    2) End instruction errors
    3) If mnemonic is invalid
    4) If no mnemonic is provided
    5) If more than one mnemonics are provided
    6) Symbol formatting errors
    7) If String exceeds 30 bytes
    8) Clashing symbol names

### Pass - 2 Errors
    1) If no input file provided
    2) If can't find input file
    3) If can't locate intermediate file
    4) ALL STRUCTURAL AND SYNTACTICAL ERRORS

### Other errors
    1) If the program overflows the RAM
