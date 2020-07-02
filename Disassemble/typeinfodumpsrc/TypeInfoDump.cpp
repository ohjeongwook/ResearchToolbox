///////////////////////////////////////////////////////////////////////////////
//
// TypeInfoDump.cpp 
//
// Author: Oleg Starodumov
// 
// Main application file 
//
//

///////////////////////////////////////////////////////////////////////////////
// Include files 
//

#include <windows.h>
#include <tchar.h>

#define _NO_CVCONST_H
#include <dbghelp.h>

#include <crtdbg.h>
#include <stdio.h>

#pragma warning ( disable: 4786 )

#include <set>
using namespace std; 

#include <atlconv.h>

#include "CTypeInfoDump.h"
#include "CTypeInfoText.h"


///////////////////////////////////////////////////////////////////////////////
// Function declarations 
//

void Dump( LPCTSTR FileName ); 
BOOL CALLBACK EnumCallbackProc( PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID Context ); 

// Information printers 
void PrintSymbolInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ); 
void PrintFunctionInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ); 
void PrintDataInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ); 
void PrintDefaultInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ); 
void PrintUserDefinedTypes( set<ULONG>& Udts, CTypeInfoText* pTypeInfo ); 
void PrintClassInfo( ULONG Index, UdtClassInfo& Info, CTypeInfoText* pTypeInfo ); 

// Helper functions 
bool GetFileParams( const TCHAR* pFileName, DWORD64& BaseAddr, DWORD& FileSize );
bool GetFileSize( const TCHAR* pFileName, DWORD& FileSize );
void ShowSymbolInfo( DWORD64 ModBase ); 


///////////////////////////////////////////////////////////////////////////////
// CMyTypeInfoText class 
// 
// This class extends the base CTypeInfoText to implement some tasks 
// specific to this demo application 
//

class CMyTypeInfoText : public CTypeInfoText 
{
public: 

	CMyTypeInfoText( CTypeInfoDump* pDump ) 
		: CTypeInfoText( pDump ) 
		{}

	void ReportUdt( ULONG Index ) 
		{ m_UDTs.insert( Index ); }

private: 

	// Copy protection 
	CMyTypeInfoText( const CMyTypeInfoText& ); 
	CMyTypeInfoText& operator=( const CMyTypeInfoText& ); 

public: 

	set<ULONG> m_UDTs; 

};


///////////////////////////////////////////////////////////////////////////////
// Global variables 
//

bool bExtraLine = true; 


///////////////////////////////////////////////////////////////////////////////
// main() 
//

int _tmain(int argc, TCHAR* argv[])
{
	// Print logo 

	_tprintf( _T("TypeInfoDump - Type information viewer\n") ); 
	_tprintf( _T("Copyright (C) 2004 Oleg Starodumov\n\n") ); 


	// Check parameters 

	if( ( argc < 2 ) || ( _tcscmp(argv[1], _T("-?")) == 0 ) || ( _tcscmp(argv[1], _T("/?")) == 0 ) ) 
	{
		_tprintf( _T("Usage: TypeInfoDump <FileName>\n") ); 
		_tprintf( _T("You can specify either an executable or a .PDB file name.\n")); 
		return 0; 
	}

	
	// Report the file name 

	_tprintf( _T("File: %s \n\n"), argv[1] ); 


	// Dump type information for the specified file 

	Dump( argv[1] ); 


	// Complete 

	return 0; 

}


///////////////////////////////////////////////////////////////////////////////
// Dump() function 
//

void Dump( LPCTSTR FileName ) 
{
	// Set SYMOPT_DEBUG option 

	DWORD Options = SymGetOptions(); 

	Options |= SYMOPT_DEBUG; 

	SymSetOptions( Options ); 


	// Initialize DbgHelp 

		// Note: if FileName is null, SymInitialize() will load symbols for 
		// this executable 

	if( !SymInitialize( GetCurrentProcess(), NULL, (FileName == 0) ? TRUE : FALSE ) ) 
	{
		_tprintf( _T("Error: SymInitialize() failed. Error: %u \n"), GetLastError()); 
		return; 
	}


	do 
	{
		// If file name is specified, load symbols for the specified executable 
		// (or from the specified .PDB file) 

		DWORD64 ModuleBase = 0; 

		if( FileName != 0 ) 
		{
			// Obtain the base address and file size 
			// (this step is needed to support explicit loading of a .PDB file) 

			DWORD64   BaseAddr  = 0; 
			DWORD     FileSize  = 0; 

			if( !GetFileParams( FileName, BaseAddr, FileSize ) ) 
			{
				_tprintf( _T("Error: Cannot obtain file parameters (internal error).\n") ); 
				break; 
			}

			// Load symbols  

			ModuleBase = SymLoadModule64( GetCurrentProcess(), NULL, FileName, NULL, BaseAddr, FileSize ); 

			if( ModuleBase == 0 ) 
			{
				_tprintf( _T("Error: SymLoadModule64() failed. Error code: %u \n"), GetLastError());
				break; 
			}

			_tprintf( _T("Load address: %I64x \n"), ModuleBase ); 


			// Obtain and display information about loaded symbols 

			ShowSymbolInfo( ModuleBase ); 

			_tprintf( _T("\n\n") ); 

		}


		// Create instances of CTypeInfoDump and CMyTypeInfoText 

		CTypeInfoDump TypeInfoDump( GetCurrentProcess(), ModuleBase ); 

		CMyTypeInfoText TypeInfoText( &TypeInfoDump ); 


		// Enumerate symbols 

		if( !SymEnumSymbols( GetCurrentProcess(), ModuleBase, NULL, EnumCallbackProc, &TypeInfoText ) ) 
		{
			_tprintf( _T("Error: SymEnumSymbols() failed. Error code: %u \n"), GetLastError());
			break; 
		}


		// Print information about used defined types 

		PrintUserDefinedTypes( TypeInfoText.m_UDTs, &TypeInfoText ); 

	}
	while( 0 ); 


	// Deinitialize DbgHelp 

	if( !SymCleanup( GetCurrentProcess() ) ) 
	{
		_tprintf( _T("SymCleanup() failed. Error: %u \n"), GetLastError()); 
	}

}


///////////////////////////////////////////////////////////////////////////////
// EnumCallbackProc() function 
//

BOOL CALLBACK EnumCallbackProc( PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID Context ) 
{
	// Check preconditions 

	if( pSymInfo == 0 ) 
	{
		// Try to enumerate other symbols 
		return TRUE; 
	}

	if( Context == 0 ) 
	{
		_ASSERTE( !_T("Context is null.") ); 
		return FALSE; 
	}


	// Print information about the symbol 

	PrintSymbolInfo( pSymInfo, (CTypeInfoText*)Context ); 


	// Continue enumeration 

	return TRUE; 

}


///////////////////////////////////////////////////////////////////////////////
// Information printing functions  
//

void PrintSymbolInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ) 
{
	// Check preconditions 

	if( pSymInfo == 0 ) 
	{
		_ASSERTE( !_T("pSymInfo is null.") ); 
		return; 
	}

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}


	// Print the information 

	switch( pSymInfo->Tag ) 
	{
		case SymTagFunction: 
			PrintFunctionInfo( pSymInfo, pTypeInfo ); 
			break; 

		case SymTagData: 
			PrintDataInfo( pSymInfo, pTypeInfo ); 
			break; 

		default: 
			PrintDefaultInfo( pSymInfo, pTypeInfo ); 
			break; 
	}

	if( bExtraLine ) 
		_tprintf( _T("\n") ); 

}

void PrintDataInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ) 
{
	// Check preconditions 

	if( pSymInfo == 0 ) 
	{
		_ASSERTE( !_T("pSymInfo is null.") ); 
		return; 
	} 

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}

	if( pSymInfo->Tag != SymTagData ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return; 
	}


	// Determine the "data kind" of the variable 

	LPCTSTR pDataKind = pTypeInfo->DataKindStr( *pSymInfo ); 


	// Print the information 

		// Name 

	_tprintf( _T("%s %s \n"), pDataKind, pSymInfo->Name ); 

		// Address, size, indices 

	const int cMaxAddrLen = 64; 
	TCHAR szAddress[cMaxAddrLen+1] = _T(""); 

	pTypeInfo->SymbolLocationStr( *pSymInfo, szAddress ); 

	_tprintf( _T("  Address: %s  Size: %8u bytes  Index: %8u  TypeIndex: %8u \n"), szAddress, pSymInfo->Size, pSymInfo->Index, pSymInfo->TypeIndex ); 

		// Type 

	_tprintf( _T("  Type: ") ); 

	const int cMaxTypeNameLen = 1024; 
	TCHAR szTypeName[cMaxTypeNameLen+1] = _T(""); 

	if( pTypeInfo->GetTypeName( pSymInfo->TypeIndex, pSymInfo->Name, szTypeName, cMaxTypeNameLen ) ) 
	{
		_tprintf( szTypeName ); 
	}
	else 
	{
		_tprintf( _T("n/a") ); 
	}

	_tprintf( _T("\n") ); 

		// Flags 

	_tprintf( _T("Flags: %x \n"), pSymInfo->Flags ); 

}

void PrintFunctionInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ) 
{
	// Check preconditions 

	if( pSymInfo == 0 ) 
	{
		_ASSERTE( !_T("pSymInfo is null.") ); 
		return; 
	}

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}

	if( pSymInfo->Tag != SymTagFunction ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return; 
	}


	// Print the information 

		// Name 

	_tprintf( _T("FUNCTION %s \n"), pSymInfo->Name ); 

		// Address, size, indices 

	_tprintf( _T("  Address: %16I64x  Size: %8u bytes  Index: %8u  TypeIndex: %8u \n"), pSymInfo->Address, pSymInfo->Size, pSymInfo->Index, pSymInfo->TypeIndex ); 

		// Type 

	_tprintf( _T("  Type: ") ); 

	const int cMaxTypeNameLen = 1024; 
	TCHAR szTypeName[cMaxTypeNameLen+1] = _T(""); 

	if( pTypeInfo->GetTypeName( pSymInfo->TypeIndex, pSymInfo->Name, szTypeName, cMaxTypeNameLen ) ) 
	{
		_tprintf( szTypeName ); 
	}
	else 
	{
		_tprintf( _T("n/a") ); 
	}

	_tprintf( _T("\n") ); 

		// Flags 

	_tprintf( _T("Flags: %x \n"), pSymInfo->Flags ); 


	// Print information about parameters and local variables 

	bExtraLine = false; 

		// Set context 

	IMAGEHLP_STACK_FRAME sf; 

	sf.InstructionOffset = pSymInfo->Address; 

	if( !SymSetContext( GetCurrentProcess(), &sf, 0 ) ) 
	{
		_tprintf( _T("SymSetContext() failed. Error: %u \n"), GetLastError()); 
		_ASSERTE( !_T("SymSetContext() failed.") ); 
	}

		// Enumerate parameters and local variables 

	if( !SymEnumSymbols( GetCurrentProcess(), NULL, NULL, EnumCallbackProc, pTypeInfo ) ) 
	{
		_tprintf( _T("Error: SymEnumSymbols() failed. Error code: %u \n"), GetLastError());
		_ASSERTE( !_T("SymEnumSymbols() failed.") ); 
	}

	bExtraLine = true; 

}

void PrintDefaultInfo( SYMBOL_INFO* pSymInfo, CTypeInfoText* pTypeInfo ) 
{
	// Check preconditions 

	if( pSymInfo == 0 ) 
	{
		_ASSERTE( !_T("pSymInfo is null.") ); 
		return; 
	}

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}


	// Print the information 

		// Name 

	_tprintf( _T("Symbol %s : "), pTypeInfo->TagStr((enum SymTagEnum)pSymInfo->Tag)); 
	_tprintf( _T("%s \n"), pSymInfo->Name ); 

		// Size 

	_tprintf( _T("  Address: %16I64x  Size: %8u bytes  Index: %8u  TypeIndex: %8u \n"), pSymInfo->Address, pSymInfo->Size, pSymInfo->Index, pSymInfo->TypeIndex ); 

		// Flags 

	_tprintf( _T("Flags: %x \n"), pSymInfo->Flags ); 

}

void PrintUserDefinedTypes( set<ULONG>& Udts, CTypeInfoText* pTypeInfo ) 
{
	// Check parameters 

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}


	// Print information about user defined types 

	set<ULONG>::const_iterator ps = Udts.begin(); 

	while( ps != Udts.end() ) 
	{
		ULONG Index = *ps; 

		TypeInfo Info; 

		if( !pTypeInfo->DumpObj()->DumpType( Index, Info ) ) 
		{
			_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
		}
		else 
		{
			if( Info.Tag == SymTagUDT ) 
			{
				if( Info.UdtKind ) 
				{
					// Print information about the class 

					PrintClassInfo( Index, Info.Info.sUdtClassInfo, pTypeInfo ); 

					// Print information about its base classes 

					for( int i = 0; i < Info.Info.sUdtClassInfo.NumBaseClasses; i++ ) 
					{
						TypeInfo BaseInfo; 

						if( !pTypeInfo->DumpObj()->DumpType( Info.Info.sUdtClassInfo.BaseClasses[i], BaseInfo ) ) 
						{
							_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
							// Continue with the next base class 
						}
						else if( BaseInfo.Tag != SymTagBaseClass ) 
						{
							_ASSERTE( !_T("Unexpected symbol tag.") ); 
							// Continue with the next base class 
						}
						else 
						{
							// Obtain information about the base class 

							TypeInfo BaseUdtInfo; 

							if( !pTypeInfo->DumpObj()->DumpType( BaseInfo.Info.sBaseClassInfo.TypeIndex, BaseUdtInfo ) ) 
							{
								_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
								// Continue with the next base class 
							} 
							else if( BaseUdtInfo.Tag != SymTagUDT ) 
							{
								_ASSERTE( !_T("Unexpected symbol tag.") ); 
								// Continue with the next base class 
							} 
							else 
							{
								// Print information about the base class 

								PrintClassInfo( BaseInfo.Info.sBaseClassInfo.TypeIndex, BaseUdtInfo.Info.sUdtClassInfo, pTypeInfo ); 

							}
						}
					}

				}
				else 
				{
					// Union 

						// UDT kind 
					_tprintf( _T("%s"), pTypeInfo->UdtKindStr( Info.Info.sUdtClassInfo.UDTKind ) ); 

						// Name 
					wprintf( L" %s", Info.Info.sUdtClassInfo.Name ); 

						// Size 
					_tprintf( _T("  Size: %I64u"), Info.Info.sUdtClassInfo.Length ); 

						// Nested 
					if( Info.Info.sUdtClassInfo.Nested ) 
						_tprintf( _T("  Nested") ); 

						// Number of members  
					_tprintf( _T("  Members: %d"), Info.Info.sUdtClassInfo.NumVariables ); 

					_tprintf( _T("\n\n") ); 

				}

			}
			else 
			{
				_ASSERTE( !_T("Symbol is not UDT.") ); 
			}
		}

		ps++; 
	}
}

void PrintClassInfo( ULONG Index, UdtClassInfo& Info, CTypeInfoText* pTypeInfo ) 
{
	USES_CONVERSION; 

	if( pTypeInfo == 0 ) 
	{
		_ASSERTE( !_T("pTypeInfo is null.") ); 
		return; 
	}

	// UDT kind 
	_tprintf( _T("%s"), pTypeInfo->UdtKindStr( Info.UDTKind ) ); 

	// Name 
	wprintf( L" %s \n", Info.Name ); 

	// Size 
	_tprintf( _T("Size: %I64u"), Info.Length ); 

	// Nested 
	if( Info.Nested ) 
		_tprintf( _T("  Nested") ); 

	// Number of member variables 
	_tprintf( _T("  Variables: %d"), Info.NumVariables ); 

	// Number of member functions 
	_tprintf( _T("  Functions: %d"), Info.NumFunctions ); 

	// Number of base classes 
	_tprintf( _T("  Base classes: %d"), Info.NumBaseClasses ); 

	// Extended information about member variables 

	_tprintf( _T("\n") ); 

	int i; 

	for( i = 0; i < Info.NumVariables; i++ ) 
	{
		TypeInfo VarInfo; 

		if( !pTypeInfo->DumpObj()->DumpType( Info.Variables[i], VarInfo ) ) 
		{
			_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
			// Continue with the next variable 
		} 
		else if( VarInfo.Tag != SymTagData ) 
		{
			_ASSERTE( !_T("Unexpected symbol tag.") ); 
			// Continue with the next variable 
		}
		else 
		{
			// Display information about the variable 

				// Data kind 

			_tprintf( _T("%s"), pTypeInfo->DataKindStr( VarInfo.Info.sDataInfo.dataKind ) ); 

				// Name 

			wprintf( L" %s \n", VarInfo.Info.sDataInfo.Name ); 

				// Type name 

			_tprintf( _T("  Type:  ") ); 

			{
				const int cMaxTypeNameLen = 1024; 
				TCHAR szTypeName[cMaxTypeNameLen+1] = _T(""); 

				if( pTypeInfo->GetTypeName( VarInfo.Info.sDataInfo.TypeIndex, W2T(VarInfo.Info.sDataInfo.Name), szTypeName, cMaxTypeNameLen ) ) 
					_tprintf( szTypeName ); 
				else 
					_tprintf( _T("n/a") ); 

			}

			_tprintf( _T("\n") ); 

				// Location 

			switch( VarInfo.Info.sDataInfo.dataKind ) 
			{
				case DataIsGlobal: 
				case DataIsStaticLocal: 
				case DataIsFileStatic: 
				case DataIsStaticMember: 
					{
						// Use Address 
						_tprintf( _T("  Address: %16I64x"), VarInfo.Info.sDataInfo.Address ); 
					}
					break; 

				case DataIsLocal:
				case DataIsParam: 
				case DataIsObjectPtr: 
				case DataIsMember: 
					{
						// Use Offset 
						_tprintf( _T("  Offset: %8d"), (long)VarInfo.Info.sDataInfo.Offset ); 
					}
					break; 

				default: 
					break; // <OS-TODO> Add support for constants 

			}

				// Indices 

			_tprintf( _T("  Index: %8u  TypeIndex: %8u"), Index, VarInfo.Info.sDataInfo.TypeIndex ); 

			_tprintf( _T("\n") ); 

		}
	}

	// Extended information about member functions 

		// <OS-TODO> Implement 

	// Extended information about base classes 

		// <OS-TODO> Implement 

	for( i = 0; i < Info.NumBaseClasses; i++ ) 
	{
		TypeInfo BaseInfo; 

		if( !pTypeInfo->DumpObj()->DumpType( Info.BaseClasses[i], BaseInfo ) ) 
		{
			_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
			// Continue with the next base class 
		}
		else if( BaseInfo.Tag != SymTagBaseClass ) 
		{
			_ASSERTE( !_T("Unexpected symbol tag.") ); 
			// Continue with the next base class 
		}
		else 
		{
			// Obtain the name of the base class 

			TypeInfo BaseUdtInfo; 

			if( !pTypeInfo->DumpObj()->DumpType( BaseInfo.Info.sBaseClassInfo.TypeIndex, BaseUdtInfo ) ) 
			{
				_ASSERTE( !_T("CTypeInfoDump::DumpType() failed.") ); 
				// Continue with the next base class 
			} 
			else if( BaseUdtInfo.Tag != SymTagUDT ) 
			{
				_ASSERTE( !_T("Unexpected symbol tag.") ); 
				// Continue with the next base class 
			} 
			else 
			{
				// Print the name of the base class 

				if( BaseInfo.Info.sBaseClassInfo.Virtual ) 
				{
					wprintf( L"VIRTUAL_BASE_CLASS %s \n", BaseUdtInfo.Info.sUdtClassInfo.Name ); 
				}
				else 
				{
					wprintf( L"BASE_CLASS %s \n", BaseUdtInfo.Info.sUdtClassInfo.Name ); 
				}

			}
		}
	}


	// Complete 

	_tprintf( _T("\n\n") ); 

}


///////////////////////////////////////////////////////////////////////////////
// Helper functions 
//

bool GetFileParams( const TCHAR* pFileName, DWORD64& BaseAddr, DWORD& FileSize ) 
{
	// Check parameters 

	if( pFileName == 0 ) 
	{
		return false; 
	}


	// Determine the extension of the file 

	TCHAR szFileExt[_MAX_EXT] = {0}; 

	_tsplitpath( pFileName, NULL, NULL, NULL, szFileExt ); 

	
	// Is it .PDB file ? 

	if( _tcsicmp( szFileExt, _T(".PDB") ) == 0 ) 
	{
		// Yes, it is a .PDB file 

		// Determine its size, and use a dummy base address 

		BaseAddr = 0x10000000; // it can be any non-zero value, but if we load symbols 
		                       // from more than one file, memory regions specified 
		                       // for different files should not overlap 
		                       // (region is "base address + file size") 
		
		if( !GetFileSize( pFileName, FileSize ) ) 
		{
			return false; 
		}

	}
	else 
	{
		// It is not a .PDB file 

		// Base address and file size can be 0 

		BaseAddr = 0; 
		FileSize = 0; 
	}


	// Complete 

	return true; 

}

bool GetFileSize( const TCHAR* pFileName, DWORD& FileSize )
{
	// Check parameters 

	if( pFileName == 0 ) 
	{
		return false; 
	}


	// Open the file 

	HANDLE hFile = ::CreateFile( pFileName, GENERIC_READ, FILE_SHARE_READ, 
	                             NULL, OPEN_EXISTING, 0, NULL ); 

	if( hFile == INVALID_HANDLE_VALUE ) 
	{
		_tprintf( _T("CreateFile() failed. Error: %u \n"), ::GetLastError() ); 
		return false; 
	}


	// Obtain the size of the file 

	FileSize = ::GetFileSize( hFile, NULL ); 

	if( FileSize == INVALID_FILE_SIZE ) 
	{
		_tprintf( _T("GetFileSize() failed. Error: %u \n"), ::GetLastError() ); 
		// and continue ... 
	}


	// Close the file 

	if( !::CloseHandle( hFile ) ) 
	{
		_tprintf( _T("CloseHandle() failed. Error: %u \n"), ::GetLastError() ); 
		// and continue ... 
	}


	// Complete 

	return ( FileSize != INVALID_FILE_SIZE ); 

}

void ShowSymbolInfo( DWORD64 ModBase ) 
{
	// Get module information 

	IMAGEHLP_MODULE64 ModuleInfo; 

	memset(&ModuleInfo, 0, sizeof(ModuleInfo) ); 

	ModuleInfo.SizeOfStruct = sizeof(ModuleInfo); 

	BOOL bRet = ::SymGetModuleInfo64( GetCurrentProcess(), ModBase, &ModuleInfo ); 

	if( !bRet ) 
	{
		_tprintf(_T("Error: SymGetModuleInfo64() failed. Error code: %u \n"), ::GetLastError());
		return; 
	}


	// Display information about symbols 

		// Kind of symbols 

	switch( ModuleInfo.SymType ) 
	{
		case SymNone: 
			_tprintf( _T("No symbols available for the module.\n") ); 
			break; 

		case SymExport: 
			_tprintf( _T("Loaded symbols: Exports\n") ); 
			break; 

		case SymCoff: 
			_tprintf( _T("Loaded symbols: COFF\n") ); 
			break; 

		case SymCv: 
			_tprintf( _T("Loaded symbols: CodeView\n") ); 
			break; 

		case SymSym: 
			_tprintf( _T("Loaded symbols: SYM\n") ); 
			break; 

		case SymVirtual: 
			_tprintf( _T("Loaded symbols: Virtual\n") ); 
			break; 

		case SymPdb: 
			_tprintf( _T("Loaded symbols: PDB\n") ); 
			break; 

		case SymDia: 
			_tprintf( _T("Loaded symbols: DIA\n") ); 
			break; 

		case SymDeferred: 
			_tprintf( _T("Loaded symbols: Deferred\n") ); // not actually loaded 
			break; 

		default: 
			_tprintf( _T("Loaded symbols: Unknown format.\n") ); 
			break; 
	}

		// Image name 

	if( _tcslen( ModuleInfo.ImageName ) > 0 ) 
	{
		_tprintf( _T("Image name: %s \n"), ModuleInfo.ImageName ); 
	}

		// Loaded image name 

	if( _tcslen( ModuleInfo.LoadedImageName ) > 0 ) 
	{
		_tprintf( _T("Loaded image name: %s \n"), ModuleInfo.LoadedImageName ); 
	}

		// Loaded PDB name 

	if( _tcslen( ModuleInfo.LoadedPdbName ) > 0 ) 
	{
		_tprintf( _T("PDB file name: %s \n"), ModuleInfo.LoadedPdbName ); 
	}

		// Is debug information unmatched ? 
		// (It can only happen if the debug information is contained 
		// in a separate file (.DBG or .PDB) 

	if( ModuleInfo.PdbUnmatched || ModuleInfo.DbgUnmatched ) 
	{
		_tprintf( _T("Warning: Unmatched symbols. \n") ); 
	}

		// Contents 

			// Line numbers available ? 

	_tprintf( _T("Line numbers: %s \n"), ModuleInfo.LineNumbers ? _T("Available") : _T("Not available") ); 

			// Global symbols available ? 

	_tprintf( _T("Global symbols: %s \n"), ModuleInfo.GlobalSymbols ? _T("Available") : _T("Not available") ); 

			// Type information available ? 

	_tprintf( _T("Type information: %s \n"), ModuleInfo.TypeInfo ? _T("Available") : _T("Not available") ); 

			// Source indexing available ? 

	_tprintf( _T("Source indexing: %s \n"), ModuleInfo.SourceIndexed ? _T("Yes") : _T("No") ); 

			// Public symbols available ? 

	_tprintf( _T("Public symbols: %s \n"), ModuleInfo.Publics ? _T("Available") : _T("Not available") ); 


}

