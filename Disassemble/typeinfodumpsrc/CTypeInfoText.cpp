///////////////////////////////////////////////////////////////////////////////
//
// CTypeInfoText.cpp 
//
// Author: Oleg Starodumov
// 
// Implementation of CTypeInfoText class 
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

#include <string>

#include <atlconv.h>

#include "CTypeInfoText.h"


///////////////////////////////////////////////////////////////////////////////
// Type definitions 
//

typedef std::basic_string<TCHAR> TString; 


///////////////////////////////////////////////////////////////////////////////
// Implementation functions 
//

bool GetTypeNameHelper( ULONG           Index, 
                        CTypeInfoText&  Obj, 
                        CTypeInfoDump&  Dump, 
                        const TString&  VarName, 
                        TString&        TypeName 
                       ); 


///////////////////////////////////////////////////////////////////////////////
// Constructors / destructor 
//

CTypeInfoText::CTypeInfoText( CTypeInfoDump* pDump ) 
: m_pDump( pDump ) 
{
	_ASSERTE( pDump != 0 ); 
}

CTypeInfoText::~CTypeInfoText() 
{
	// no actions 
}


///////////////////////////////////////////////////////////////////////////////
// Operations 
//

bool CTypeInfoText::GetTypeName
( 
	ULONG         Index, 
	const TCHAR*  pVarName, 
	TCHAR*        pTypeName, 
	int           MaxChars 
) 
{
	// Check parameters 

	if( pTypeName == 0 ) 
	{
		_ASSERTE( !_T("Type name buffer pointer is null.") ); 
		return false; 
	}

	TString VarName; 

	if( pVarName != 0 ) 
		VarName = pVarName; 


	// Obtain the type name 

	TString TypeName; 

	if( !GetTypeNameHelper( Index, *this, *m_pDump, VarName, TypeName ) ) 
	{
		return false; 
	}

	if( TypeName.empty() ) 
	{
		_ASSERTE( !_T("Empty type string.") ); 
		return false; 
	}


	// Return the type name to the caller 

	_sntprintf( pTypeName, MaxChars, _T("%s"), TypeName.c_str() ); 


	// Complete 

	return true; 

}

bool GetTypeNameHelper
(
	ULONG           Index, 
	CTypeInfoText&  Obj, 
	CTypeInfoDump&  Dump, 
	const TString&  VarName, 
	TString&        TypeName 
) 
{
	USES_CONVERSION; 

	const int cTempBufSize = 128; 


	// Get the type information 

	TypeInfo Info; 

	if( !Dump.DumpType( Index, Info ) ) 
	{
		return false; 
	}
	else 
	{
		// Is it a pointer ?  

		int NumPointers = 0; 

		if( Info.Tag == SymTagPointerType ) 
		{
			// Yes, get the number of * to show 

			ULONG UndTypeIndex   = 0; 

			if( !Dump.PointerType( Index, UndTypeIndex, NumPointers ) ) 
			{
				return false; 
			}

			// Get more information about the type the pointer points to 
				
			if( !Dump.DumpType( UndTypeIndex, Info ) ) 
			{
				return false; 
			}

			// Save the index of the type the pointer points to 

			Index = UndTypeIndex; 

			// ... and proceed with that type 

		} 


		// Display type name 

		switch( Info.Tag ) 
		{
		case SymTagBaseType: 
			TypeName += Obj.BaseTypeStr(Info.Info.sBaseTypeInfo.BaseType, Info.Info.sBaseTypeInfo.Length); 
			break; 

		case SymTagTypedef: 
			TypeName += W2T(Info.Info.sTypedefInfo.Name); 
			break; 

		case SymTagUDT: 
			{
				// Is it a class/structure or a union ? 

				if( Info.UdtKind ) 
				{
					// A class/structure 
					TypeName += W2T(Info.Info.sUdtClassInfo.Name); 

					// Add the UDT and its base classes to the collection of UDT indexes 
					Obj.ReportUdt( Index ); 

				}
				else 
				{
					// A union 
					TypeName += W2T(Info.Info.sUdtUnionInfo.Name); 
				}
			}
			break; 

		case SymTagEnum: 
			TypeName += W2T(Info.Info.sEnumInfo.Name);
			break; 

		case SymTagFunctionType: 
			{
				// Print whether it is static 

				if( Info.Info.sFunctionTypeInfo.MemberFunction ) 
					if( Info.Info.sFunctionTypeInfo.StaticFunction ) 
						TypeName += _T("static "); 

				// Print return value 

				if( !GetTypeNameHelper( Info.Info.sFunctionTypeInfo.RetTypeIndex, Obj, Dump, VarName, TypeName ) )
					return false; 

				// Print calling convention 

				TypeName += _T(" "); 
				TypeName += Obj.CallConvStr(Info.Info.sFunctionTypeInfo.CallConv); 
				TypeName += _T(" "); 

				// If member function, save the class type index 

				if( Info.Info.sFunctionTypeInfo.MemberFunction ) 
				{
					Obj.ReportUdt( Info.Info.sFunctionTypeInfo.ClassIndex );
					
/*
					// It is not needed to print the class name here, because 
					// it is contained in the function name 
					if( !GetTypeNameHelper( Info.Info.sFunctionTypeInfo.ClassIndex, Obj, Dump, VarName, TypeName ) )
						return false; 

					TypeName += _T("::"); 
*/

				}

				// Print that it is a function 

				TypeName += VarName; 

				// Print parameters 

				TypeName += _T(" ("); 

				for( int i = 0; i < Info.Info.sFunctionTypeInfo.NumArgs; i++ ) 
				{
					if( !GetTypeNameHelper( Info.Info.sFunctionTypeInfo.Args[i], Obj, Dump, VarName, TypeName ) )
						return false; 

					if( i < ( Info.Info.sFunctionTypeInfo.NumArgs - 1 ) ) 
						TypeName += _T(", ");
				}

				TypeName += _T(")"); 

				// Print "this" adjustment value 

				if( Info.Info.sFunctionTypeInfo.MemberFunction ) 
					if( Info.Info.sFunctionTypeInfo.ThisAdjust != 0 ) 
					{
						TypeName += _T("ThisAdjust: "); 

						TCHAR szBuffer[cTempBufSize+1] = _T(""); 
						_sntprintf( szBuffer, cTempBufSize, _T("%u"), Info.Info.sFunctionTypeInfo.ThisAdjust ); 

						TypeName += szBuffer; 
					}

			}

			break; 

		case SymTagFunctionArgType: 
			if( !GetTypeNameHelper( Info.Info.sFunctionArgTypeInfo.TypeIndex, Obj, Dump, VarName, TypeName ) )
				return false; 
			break; 

		case SymTagArrayType: 
			{
				// Print element type name 

				if( !GetTypeNameHelper( Info.Info.sArrayTypeInfo.ElementTypeIndex, Obj, Dump, VarName, TypeName ) )
					return false; 

				TypeName += _T(" "); 
				TypeName += VarName; 

				// Print dimensions 

				for( int i = 0; i < Info.Info.sArrayTypeInfo.NumDimensions; i++ ) 
				{
					TCHAR szBuffer[cTempBufSize+1] = _T(""); 
					_sntprintf( szBuffer, cTempBufSize, _T("[%u]"), Info.Info.sArrayTypeInfo.Dimensions[i] ); 
					TypeName += szBuffer; 
				}

			}
			break; 

		default: 
			{
				TCHAR szBuffer[cTempBufSize+1] = _T(""); 
				_sntprintf( szBuffer, cTempBufSize, _T("Unknown(%d)"), Info.Tag ); 
				TypeName += szBuffer; 
			}
			break; 
		}


		// If it is a pointer, display * characters 

		if( NumPointers != 0 ) 
		{
			for( int i = 0; i < NumPointers; i++ ) 
				TypeName += _T("*"); 
		}

	}


	// Complete 

	return true; 

}


///////////////////////////////////////////////////////////////////////////////
// Data-to-string conversion functions 
//

LPCTSTR CTypeInfoText::TagStr( enum SymTagEnum Tag ) 
{
	switch( Tag ) 
	{
    case SymTagNull:
			return _T("Null"); 
    case SymTagExe:
			return _T("Exe"); 
    case SymTagCompiland:
			return _T("Compiland"); 
    case SymTagCompilandDetails:
			return _T("CompilandDetails"); 
    case SymTagCompilandEnv:
			return _T("CompilandEnv"); 
    case SymTagFunction:
			return _T("Function"); 
    case SymTagBlock:
			return _T("Block"); 
    case SymTagData:
			return _T("Data"); 
    case SymTagAnnotation:
			return _T("Annotation"); 
    case SymTagLabel:
			return _T("Label"); 
    case SymTagPublicSymbol:
			return _T("PublicSymbol"); 
    case SymTagUDT:
			return _T("UDT"); 
    case SymTagEnum:
			return _T("Enum"); 
    case SymTagFunctionType:
			return _T("FunctionType"); 
    case SymTagPointerType:
			return _T("PointerType"); 
    case SymTagArrayType:
			return _T("ArrayType"); 
    case SymTagBaseType:
			return _T("BaseType"); 
    case SymTagTypedef:
			return _T("Typedef"); 
    case SymTagBaseClass:
			return _T("BaseClass"); 
    case SymTagFriend:
			return _T("Friend"); 
    case SymTagFunctionArgType:
			return _T("FunctionArgType"); 
    case SymTagFuncDebugStart:
			return _T("FuncDebugStart"); 
    case SymTagFuncDebugEnd:
			return _T("FuncDebugEnd"); 
    case SymTagUsingNamespace:
			return _T("UsingNamespace"); 
    case SymTagVTableShape:
			return _T("VTableShape"); 
    case SymTagVTable:
			return _T("VTable"); 
    case SymTagCustom:
			return _T("Custom"); 
    case SymTagThunk:
			return _T("Thunk"); 
    case SymTagCustomType:
			return _T("CustomType"); 
    case SymTagManagedType:
			return _T("ManagedType"); 
    case SymTagDimension:
			return _T("Dimension"); 
    case SymTagMax: 
			_ASSERTE( !_T("Unknown tag.") ); 
			return _T("Unknown"); 
		default: 
			_ASSERTE( !_T("Unknown tag.") ); 
			return _T("Unknown"); 
	}

	_ASSERTE( !_T("Unreachable code.") ); 

	return _T(""); 

}

LPCTSTR CTypeInfoText::BaseTypeStr( enum BasicType Type, ULONG64 Length ) 
{
	switch( Type ) 
	{
		case btNoType: 
			return _T("NoType"); 
    case btVoid: 
			return _T("void"); 
    case btChar: 
			return _T("char"); 
    case btWChar: 
			return _T("wchar_t"); 
    case btInt: 
			if( Length == 0 ) 
			{
				return _T("int"); 
			}
			else 
			{
				if( Length == 1 ) 
					return _T("char"); 
				else if( Length == 2 ) 
					return _T("short"); 
				else 
					return _T("int"); 
			}
    case btUInt: 
			if( Length == 0 ) 
			{
				return _T("unsigned int"); 
			}
			else 
			{
				if( Length == 1 ) 
					return _T("unsigned char"); 
				else if( Length == 2 ) 
					return _T("unsigned short"); 
				else 
					return _T("unsigned int"); 
			}
    case btFloat: 
			if( Length == 0 ) 
			{
				return _T("float"); 
			}
			else 
			{
				if( Length == 4 ) 
					return _T("float"); 
				else 
					return _T("double"); 
			}
    case btBCD: 
			return _T("BCD"); 
    case btBool: 
			return _T("bool"); 
    case btLong: 
			return _T("long"); 
    case btULong: 
			return _T("unsigned long"); 
    case btCurrency: 
			return _T("Currency"); 
    case btDate: 
			return _T("Date"); 
    case btVariant: 
			return _T("Variant"); 
    case btComplex: 
			return _T("Complex"); 
    case btBit: 
			return _T("Bit"); 
    case btBSTR: 
			return _T("BSTR"); 
    case btHresult: 
			return _T("HRESULT"); 
		default: 
			_ASSERTE( !_T("Unknown basic type.") ); 
			return _T("Unknown"); 
	}

	_ASSERTE( !_T("Unreachable code.") ); 

	return _T(""); 

}

LPCTSTR CTypeInfoText::CallConvStr( enum CV_call_e CallConv ) 
{
	switch( CallConv ) 
	{
		case CV_CALL_NEAR_C: 
			return _T("NEAR_C"); 
    case CV_CALL_FAR_C: 
			return _T("FAR_C"); 
    case CV_CALL_NEAR_PASCAL: 
			return _T("NEAR_PASCAL"); 
    case CV_CALL_FAR_PASCAL: 
			return _T("FAR_PASCAL"); 
    case CV_CALL_NEAR_FAST: 
			return _T("NEAR_FAST"); 
    case CV_CALL_FAR_FAST: 
			return _T("FAR_FAST"); 
    case CV_CALL_SKIPPED: 
			return _T("SKIPPED"); 
    case CV_CALL_NEAR_STD: 
			return _T("NEAR_STD"); 
    case CV_CALL_FAR_STD: 
			return _T("FAR_STD"); 
    case CV_CALL_NEAR_SYS: 
			return _T("NEAR_SYS"); 
    case CV_CALL_FAR_SYS: 
			return _T("FAR_SYS"); 
    case CV_CALL_THISCALL: 
			return _T("THISCALL"); 
    case CV_CALL_MIPSCALL: 
			return _T("MIPSCALL"); 
    case CV_CALL_GENERIC: 
			return _T("GENERIC"); 
    case CV_CALL_ALPHACALL: 
			return _T("ALPHACALL"); 
    case CV_CALL_PPCCALL: 
			return _T("PPCCALL"); 
    case CV_CALL_SHCALL: 
			return _T("SHCALL"); 
    case CV_CALL_ARMCALL: 
			return _T("ARMCALL"); 
    case CV_CALL_AM33CALL: 
			return _T("AM33CALL"); 
    case CV_CALL_TRICALL: 
			return _T("TRICALL"); 
    case CV_CALL_SH5CALL: 
			return _T("SH5CALL"); 
    case CV_CALL_M32RCALL: 
			return _T("M32RCALL"); 
    case CV_CALL_RESERVED: 
			return _T("RESERVED"); 
		default: 
			_ASSERTE( !_T("Unknown calling convention.") ); 
			return _T("UNKNOWN"); 
	}

	_ASSERTE( !_T("Unreachable code.") ); 

	return _T(""); 

}

LPCTSTR CTypeInfoText::DataKindStr( SYMBOL_INFO& rSymbol ) 
{
	DWORD Dk = 0; 

	if( !SymGetTypeInfo( GetCurrentProcess(), rSymbol.ModBase, rSymbol.Index, TI_GET_DATAKIND, &Dk ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_DATAKIND) failed.") ); 
		return _T("UNKNOWN"); 
	}

	return DataKindStr( (DataKind)Dk ); 

}

LPCTSTR CTypeInfoText::DataKindStr( enum DataKind dataKind ) 
{
	switch( dataKind ) 
	{
	case DataIsLocal: 
		return _T("LOCAL_VAR"); 

	case DataIsStaticLocal: 
		return _T("STATIC_LOCAL_VAR"); 

	case DataIsParam: 
		return _T("PARAMETER"); 

	case DataIsObjectPtr: 
		return _T("OBJECT_PTR"); 

	case DataIsFileStatic: 
		return _T("STATIC_VAR"); 

	case DataIsGlobal: 
		return _T("GLOBAL_VAR"); 

	case DataIsMember: 
		return _T("MEMBER"); 

	case DataIsStaticMember: 
		return _T("STATIC_MEMBER"); 

	case DataIsConstant: 
		return _T("CONSTANT"); 

	default: 
		_ASSERTE( !_T("Unknown data kind.") ); 
		return _T("UNKNOWN"); 
	}

	_ASSERTE( !_T("Unreachable code.") ); 

	return _T("UNKNOWN"); 

}

void CTypeInfoText::SymbolLocationStr( SYMBOL_INFO& rSymbol, TCHAR* pBuffer ) 
{
	// Note: The caller must pass a big-enough buffer 
	// 
	// Note: The function must guarantee that a valid string 
	//       is placed into pBuffer in all possible executaion paths 
	//       (so that the caller does not have to check the returned 
	//       string for correctness) (if pBuffer is correct, of course) 
	// 

	if( pBuffer == 0 ) 
	{
		_ASSERTE( !_T("Buffer pointer is null.") ); 
		return; 
	}

	if( rSymbol.Flags & SYMFLAG_REGISTER ) 
	{
		LPCTSTR pRegStr = RegisterStr( (enum CH_HREG_e) rSymbol.Register ); 

		if( pRegStr != 0 ) 
			_tcscpy( pBuffer, pRegStr ); 
		else 
			_stprintf( pBuffer, _T("Reg%u"), rSymbol.Register); 

		return; 
	}
	else if( rSymbol.Flags & SYMFLAG_REGREL ) 
	{
		TCHAR szReg[32]; 

		LPCTSTR pRegStr = RegisterStr( (enum CH_HREG_e) rSymbol.Register ); 

		if( pRegStr != 0 ) 
			_tcscpy( szReg, pRegStr ); 
		else 
			_stprintf( szReg, _T("Reg%u"), rSymbol.Register); 

		_stprintf( pBuffer, _T("%s%+d"), szReg, (long)rSymbol.Address ); 

		return; 
	}
	else if( rSymbol.Flags & SYMFLAG_FRAMEREL ) 
	{
		_tcscpy( pBuffer, _T("n/a") ); // not supported 
		return; 
	}
	else 
	{
		_stprintf( pBuffer, _T("%16I64x"), rSymbol.Address ); 
		return; 
	}
}

LPCTSTR CTypeInfoText::RegisterStr( enum CH_HREG_e RegCode ) 
{
	switch( RegCode ) 
	{
		case CV_REG_EAX: 
			return _T("EAX"); 
		case CV_REG_ECX: 
			return _T("ECX"); 
		case CV_REG_EDX: 
			return _T("EDX"); 
		case CV_REG_EBX: 
			return _T("EBX"); 
		case CV_REG_ESP: 
			return _T("ESP"); 
		case CV_REG_EBP: 
			return _T("EBP"); 
		case CV_REG_ESI: 
			return _T("ESI"); 
		case CV_REG_EDI: 
			return _T("EDI"); 
		default: 
			return 0; 
	}
}

LPCTSTR CTypeInfoText::UdtKindStr( enum UdtKind Kind ) 
{
	switch( Kind ) 
	{
		case UdtStruct: 
			return _T("STRUCT"); 
		case UdtClass: 
			return _T("CLASS"); 
		case UdtUnion: 
			return _T("UNION"); 
		default: 
			_ASSERTE( !_T("Unknown UDT kind.") ); 
			return _T("UNKNOWN_UDT"); 
	}
}

LPCTSTR CTypeInfoText::LocationTypeStr( enum LocationType Loc ) 
{
	switch( Loc ) 
	{
	case LocIsNull: 
		return _T("Null"); 
	case LocIsStatic: 
		return _T("Static"); 
	case LocIsTLS: 
		return _T("TLS"); 
	case LocIsRegRel: 
		return _T("RegRel"); 
	case LocIsThisRel: 
		return _T("ThisRel"); 
	case LocIsEnregistered: 
		return _T("Enregistered"); 
	case LocIsBitField: 
		return _T("BitField"); 
	case LocIsSlot: 
		return _T("Slot"); 
	case LocIsIlRel: 
		return _T("IlRel"); 
	case LocInMetaData: 
		return _T("MetaData"); 
	case LocIsConstant: 
		return _T("Constant"); 
	default: 
		_ASSERTE( !_T("Unknown location type.") ); 
		return _T("Unknown"); 
	}
}

