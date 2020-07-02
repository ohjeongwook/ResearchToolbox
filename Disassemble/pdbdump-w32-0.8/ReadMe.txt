pdbdump (c) 2002 Andrew de Quincey
Distributed under a BSD-style license. See LICENSE.TXT for more details.
http://pdbdump.sourceforge.net/



Most people assume that PDB files contain just function names. However 
many (although by no means all) contain much more information than this, 
including full class definitions, and local variable names of functions. 

It is possible to strip this "private" information from your PDB files 
(reducing them back to simple function names) when using Microsoft's 
linker, by using the /PDBSTRIPPED flag.

You will need a copy of msdia20.dll in the same directory as pdbdump.exe. 
This file is distributed in the DIA SDK with Visual Studio.NET.


Usage: pdbdump <PDB filename>
The contents of the supplied PDB file will be dumped to stdout.
