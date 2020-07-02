///////////////////////////////////////////////////////////////////////////////
//
// TypeInfoDump.cpp 
//
// Author: Oleg Starodumov
// 
// Implementation of type information functions 
// 
//


///////////////////////////////////////////////////////////////////////////////
// Include files 
//

#include <windows.h>
#include <tchar.h>

#include <crtdbg.h>
#include <malloc.h>

#include "CTypeInfoDump.h"


///////////////////////////////////////////////////////////////////////////////
// Helpers 
//

#define ARRAY_SIZE(Array) \
	(sizeof(Array) / sizeof((Array)[0]))


///////////////////////////////////////////////////////////////////////////////
// Constructors 
//

CTypeInfoDump::CTypeInfoDump()
: m_hProcess( NULL ), m_ModBase( 0 ) 
{
}

CTypeInfoDump::CTypeInfoDump( HANDLE hProcess, DWORD64 ModBase ) 
: m_hProcess( hProcess ), m_ModBase( ModBase ) 
{
}


///////////////////////////////////////////////////////////////////////////////
// Type information functions  
//

// This is a demo function for the accompanying article. 
/* 
bool GetChildren
( 
	HANDLE   hProcess,    // [in]  Process handle 
	DWORD64  ModuleBase,  // [in]  Module base address 
	ULONG    Index,       // [in]  Index of the symbol whose children are needed 
	ULONG*   pChildren,   // [out] Points to the buffer that will receive the child indices 
	DWORD&   NumChildren, // [out] Number of children found 
	DWORD    MaxChildren  // [in]  Maximal number of child indices the buffer can store 
) 
{
	// Check parameters 

	if( pChildren == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxChildren == 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pChildren, MaxChildren * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD ChildCount = 0; 

	if( !SymGetTypeInfo( hProcess, ModuleBase, Index, TI_GET_CHILDRENCOUNT, &ChildCount ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( ChildCount == 0 ) 
	{
		// No children 
		NumChildren = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + ChildCount*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = ChildCount; 

	if( !SymGetTypeInfo( hProcess, ModuleBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}


	// Copy children to the [out] parameter 

	DWORD ChildIndex = 0; 

	for( DWORD i = 0; i < ChildCount; i++ ) 
	{
		// If we are only interested in children with a specific tag 
		// (e.g. if we are looking for members of a class), 
		// we can also check the tag here and decide whether to copy 
		// the child id (index) to the [out] array or not 

		pChildren[ChildIndex] = pFC->ChildId[i];

		ChildIndex++; 

		if( ChildIndex == MaxChildren ) 
			break; 

	}

	NumChildren = ChildIndex; 


	// Complete 

	return true; 

}
*/

bool CTypeInfoDump::DumpBasicType( ULONG Index, BaseTypeInfo& Info ) 
{
	// Check if it is really SymTagBaseType 

	if( !CheckTag( Index, SymTagBaseType ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Basic type ("basicType" in DIA)

	DWORD BaseType = btNoType; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_BASETYPE, &BaseType ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_BASETYPE) failed.") ); 
		return false; 
	}

	Info.BaseType = (BasicType)BaseType; 

		// Length ("length" in DIA) 

	ULONG64 Length = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
		return false; 
	}

	Info.Length = Length; 

	
	// Success 

	return true; 

}

bool CTypeInfoDump::DumpPointerType( ULONG Index, PointerTypeInfo& Info ) 
{
	// Check if it is really SymTagPointerType 

	if( !CheckTag( Index, SymTagPointerType ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Type index ("typeId" in DIA)

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 
	
		// Length ("length" in DIA) 

	ULONG64 Length = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
		return false; 
	}

	Info.Length = Length; 

		// Reference ("reference" in DIA) 

	// This property is not available via SymGetTypeInfo(), 
	// therefore it is not possible to determine if the pointer is a pointer 
	// or a reference 


	// Success 

	return true; 

}

bool CTypeInfoDump::DumpTypedef( ULONG Index, TypedefInfo& Info ) 
{
	// Check if it is really SymTagTypedef 

	if( !CheckTag( Index, SymTagTypedef ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Type index ("typeId" in DIA)

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 

		// Name ("name" in DIA) 

	WCHAR* pName = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMNAME, &pName ) || ( pName == 0 ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMNAME) failed.") ); 
		return false; 
	}

	wcsncpy( Info.Name, pName, ARRAY_SIZE(Info.Name) ); 
	Info.Name[ ARRAY_SIZE(Info.Name) - 1 ] = 0; 

	LocalFree( pName ); 

	
	// Success 

	return true; 

}

bool CTypeInfoDump::DumpEnum( ULONG Index, EnumInfo& Info ) 
{
	// Check if it is really SymTagEnum 

	if( !CheckTag( Index, SymTagEnum ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Name ("name" in DIA) 

	WCHAR* pName = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMNAME, &pName ) || ( pName == 0 ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMNAME) failed.") ); 
		return false; 
	}

	wcsncpy( Info.Name, pName, ARRAY_SIZE(Info.Name) ); 
	Info.Name[ ARRAY_SIZE(Info.Name) - 1 ] = 0; 

	LocalFree( pName ); 

		// Type index ("typeId" in DIA)

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 

		// Nested ("nested" in DIA) 

	DWORD Nested = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_NESTED, &Nested ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_NESTED) failed.") ); 
		return false; 
	}

	Info.Nested = ( Nested != 0 ); 

		// Enumerators 

	if( !Enumerators( Index, Info.Enums, Info.NumEnums, ARRAY_SIZE(Info.Enums) ) ) 
	{
		_ASSERTE( !_T("Enumerators() failed.") ); 
		return false; 
	}
	
	
	// Success 

	return true; 

}

bool CTypeInfoDump::DumpArrayType( ULONG Index, ArrayTypeInfo& Info ) 
{
	// Check if it is really SymTagArrayType 

	if( !CheckTag( Index, SymTagArrayType ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Element type index 

	ULONG ElementTypeIndex = 0; 

	if( !ArrayElementTypeIndex( Index, ElementTypeIndex ) ) 
	{
		_ASSERTE( !_T("ArrayElementTypeIndex() failed.") ); 
		return false; 
	}

	Info.ElementTypeIndex = ElementTypeIndex; 

		// Length ("length" in DIA) 

	ULONG64 Length = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
		return false; 
	}

	Info.Length = Length; 

		// Type index of the array index element 

	DWORD IndexTypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_ARRAYINDEXTYPEID, &IndexTypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_ARRAYINDEXTYPEID) failed.") ); 
		return false; 
	}

	Info.IndexTypeIndex = IndexTypeIndex; 

		// Dimensions 

	if( Length > 0 ) 
	{
		if( !ArrayDims( Index, Info.Dimensions, Info.NumDimensions, ARRAY_SIZE(Info.Dimensions) ) || ( Info.NumDimensions == 0 ) ) 
		{
			_ASSERTE( !_T("ArrayDims() failed.") ); 
			return false; 
		}
	}
	else 
	{
		// No dimensions 
		Info.NumDimensions = 0; 
	}
	
	
	// Success 

	return true; 

}

bool CTypeInfoDump::DumpUDT( ULONG Index, TypeInfo& Info ) 
{
	// Check if it is really SymTagUDT 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}


	// Determine UDT kind (is it a class/structure or a union ?)

	DWORD UDTKind = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_UDTKIND, &UDTKind ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_UDTKIND) failed.") ); 
		return false; 
	}


	// Call the worker function depending on the UDT kind 

	bool bResult = false; 

	switch( UDTKind ) 
	{
		case UdtStruct: 
			Info.UdtKind = true; 
			bResult = DumpUDTClass( Index, Info.Info.sUdtClassInfo ); 
			break; 

		case UdtClass: 
			Info.UdtKind = true; 
			bResult = DumpUDTClass( Index, Info.Info.sUdtClassInfo ); 
			break; 

		case UdtUnion: 
			Info.UdtKind = false; 
			bResult = DumpUDTUnion( Index, Info.Info.sUdtUnionInfo ); 
			break; 

		default: 
			_ASSERTE( !_T("Unknown UdtKind.") ); 
			break; 
	}
	

	// Complete 

	return bResult; 

}

bool CTypeInfoDump::DumpUDTClass( ULONG Index, UdtClassInfo& Info ) 
{
	// Check if it is really SymTagUDT 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	// Check if it is really a class or structure UDT ? 

	DWORD UDTKind = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_UDTKIND, &UDTKind ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_UDTKIND) failed.") ); 
		return false; 
	}

	if( ( UDTKind != UdtStruct ) && ( UDTKind != UdtClass ) )
	{
		_ASSERTE( !_T("Incorrect UdtKind.") ); 
		return false; 
	}

	
	// Get properties 

		// UdtKind 

	Info.UDTKind = (UdtKind)UDTKind; 

		// Name ("name" in DIA) 

	WCHAR* pName = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMNAME, &pName ) || ( pName == 0 ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMNAME) failed.") ); 
		return false; 
	}

	wcsncpy( Info.Name, pName, ARRAY_SIZE(Info.Name) ); 
	Info.Name[ ARRAY_SIZE(Info.Name) - 1 ] = 0; 

	LocalFree( pName ); 

		// Length ("length" in DIA) 

	ULONG64 Length = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
		return false; 
	}

	Info.Length = Length; 

		// Nested ("nested" in DIA) 

	DWORD Nested = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_NESTED, &Nested ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_NESTED) failed.") ); 
		return false; 
	}

	Info.Nested = ( Nested != 0 ); 

	
	// Dump class members and base classes 

		// Member variables 

	if( !UdtVariables( Index, Info.Variables, Info.NumVariables, ARRAY_SIZE(Info.Variables) ) ) 
	{
		_ASSERTE( !_T("UdtVariables() failed.") ); 
		return false; 
	}

		// Member functions 

	if( !UdtFunctions( Index, Info.Functions, Info.NumFunctions, ARRAY_SIZE(Info.Functions) ) ) 
	{
		_ASSERTE( !_T("UdtFunctions() failed.") ); 
		return false; 
	}

		// Base classes 

	if( !UdtBaseClasses( Index, Info.BaseClasses, Info.NumBaseClasses, ARRAY_SIZE(Info.BaseClasses) ) ) 
	{
		_ASSERTE( !_T("UdtBaseClasses() failed.") ); 
		return false; 
	}


	// Success 

	return true; 

}

bool CTypeInfoDump::DumpUDTUnion( ULONG Index, UdtUnionInfo& Info ) 
{
	// Check if it is really SymTagUDT 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	// Check if it is really a union UDT ? 

	DWORD UDTKind = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_UDTKIND, &UDTKind ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_UDTKIND) failed.") ); 
		return false; 
	}

	if( UDTKind != UdtUnion ) 
	{
		_ASSERTE( !_T("Incorrect UdtKind.") ); 
		return false; 
	}

	
	// Get properties 

		// UdtKind 

	Info.UDTKind = (UdtKind)UDTKind; 

		// Name ("name" in DIA) 

	WCHAR* pName = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMNAME, &pName ) || ( pName == 0 ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMNAME) failed.") ); 
		return false; 
	}

	wcsncpy( Info.Name, pName, ARRAY_SIZE(Info.Name) ); 
	Info.Name[ ARRAY_SIZE(Info.Name) - 1 ] = 0; 

	LocalFree( pName ); 

		// Length ("length" in DIA) 

	ULONG64 Length = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
		return false; 
	}

	Info.Length = Length; 

		// Nested ("nested" in DIA) 

	DWORD Nested = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_NESTED, &Nested ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_NESTED) failed.") ); 
		return false; 
	}

	Info.Nested = ( Nested != 0 ); 

		// Union members 

	if( !UdtUnionMembers( Index, Info.Members, Info.NumMembers, ARRAY_SIZE(Info.Members) ) ) 
	{
		_ASSERTE( !_T("UdtUnionMembers() failed.") ); 
		return false; 
	}

	
	// Success 

	return true; 

}

bool CTypeInfoDump::DumpFunctionType( ULONG Index, FunctionTypeInfo& Info ) 
{
	// Check if it is really SymTagFunctionType 

	if( !CheckTag( Index, SymTagFunctionType ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Index of the return type symbol ("typeId" in DIA)

	DWORD RetTypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &RetTypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.RetTypeIndex = RetTypeIndex; 

		// Number of arguments ("count" in DIA) 
		// Note: For non-static member functions, it includes "this" 

	DWORD NumArgs = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_COUNT, &NumArgs ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_COUNT) failed.") ); 
		return false; 
	}
	// Do not save it in the data structure, since we obtain the number of arguments 
	// again using FunctionArguments() (see below). 
	// But later we will use this value to determine whether the function 
	// is static or not (if it is a member function) 

		// Calling convention ("callingConvention" in DIA) 

	DWORD CallConv = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CALLING_CONVENTION, &CallConv ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CALLING_CONVENTION) failed.") ); 
		return false; 
	}

	Info.CallConv = (CV_call_e)CallConv; 

		// Parent class type index ("classParent" in DIA) 

	ULONG ClassIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CLASSPARENTID, &ClassIndex ) ) 
	{
		// The function is not a member function 
		DWORD ErrCode = GetLastError(); 
		ClassIndex = 0; 
		Info.ClassIndex = 0; 
		Info.MemberFunction = false; 
	}
	else 
	{
		// This is a member function of a class 
		Info.ClassIndex = ClassIndex; 
		Info.MemberFunction = true; 
	}

	// If this is a member function, obtain additional data 

	if( Info.MemberFunction ) 
	{
		// "this" adjustment ("thisAdjust" in DIA) 

		LONG ThisAdjust = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_THISADJUST, &ThisAdjust ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_THISADJUST) failed.") ); 
			return false; 
		}

		Info.ThisAdjust = ThisAdjust; 
	}


		// <OS-TEST> Test for GetChildren() function 
/*
	const DWORD cMaxChildren = 4; 
	ULONG Children[cMaxChildren]; 
	DWORD NumChildren = 0; 

	if( !GetChildren( m_hProcess, m_ModBase, Index, Children, NumChildren, cMaxChildren ) ) 
	{
		DWORD ErrCode = 0; 
		_ASSERTE( !_T("GetChildren() failed.") ); 
	}
	else 
	{
		DWORD ErrCode = 0; 
	}
*/
		// <OS-TEST> End of test 


		// Dump function arguments 

	if( !FunctionArguments( Index, Info.Args, Info.NumArgs, ARRAY_SIZE(Info.Args) ) ) 
	{
		_ASSERTE( !_T("FunctionArguments() failed.") ); 
		return false; 
	}

		// Is the function static ? (If it is a member function) 

	if( Info.MemberFunction ) 
	{
		// The function is static if the value of Count property 
		// it the same as the number of child FunctionArgType symbols 

		Info.StaticFunction = ( NumArgs == Info.NumArgs ); 
	}


	// Success 

	return true; 

}

bool CTypeInfoDump::DumpFunctionArgType( ULONG Index, FunctionArgTypeInfo& Info ) 
{
	// Check if it is really SymTagFunctionArgType 

	if( !CheckTag( Index, SymTagFunctionArgType ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Index of the argument type ("typeId" in DIA)

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 


	// Success 

	return true; 

}

bool CTypeInfoDump::DumpBaseClass( ULONG Index, BaseClassInfo& Info ) 
{
	// Check if it is really SymTagBaseClass 

	if( !CheckTag( Index, SymTagBaseClass ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}


	// Get properties 

		// Base class UDT 

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 

		// Is this base class virtual ? 

	DWORD VirtualBase = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_VIRTUALBASECLASS, &VirtualBase ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_VIRTUALBASECLASS) failed.") ); 
		return false; 
	}

	Info.Virtual = ( VirtualBase != 0 ); 

		// Location in the class 

	if( VirtualBase == 0 ) 
	{
		// Offset in the parent UDT 

		DWORD Offset = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_OFFSET, &Offset ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_OFFSET) failed.") ); 
			return false; 
		}

		Info.Offset = Offset; 

		Info.VirtualBasePointerOffset = 0; 
	}
	else 
	{
		// Virtual base pointer offset 

		DWORD VirtualBasePtrOffset = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_VIRTUALBASEPOINTEROFFSET, &VirtualBasePtrOffset ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_VIRTUALBASEPOINTEROFFSET) failed.") ); 
			return false; 
		}

		Info.VirtualBasePointerOffset = VirtualBasePtrOffset; 

		Info.Offset = 0; 
	}


	// Success 

	return true; 

}

bool CTypeInfoDump::DumpData( ULONG Index, DataInfo& Info ) 
{
	// Check if it is really SymTagData 

	if( !CheckTag( Index, SymTagData ) ) 
	{
		_ASSERTE( !_T("Incorrect tag.") ); 
		return false; 
	}

	
	// Get properties 

		// Name ("name" in DIA) 

	WCHAR* pName = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMNAME, &pName ) || ( pName == 0 ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMNAME) failed.") ); 
		return false; 
	}

	wcsncpy( Info.Name, pName, ARRAY_SIZE(Info.Name) ); 
	Info.Name[ ARRAY_SIZE(Info.Name) - 1 ] = 0; 

	LocalFree( pName ); 

		// Index of type symbol ("typeId" in DIA)

	DWORD TypeIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	Info.TypeIndex = TypeIndex; 

		// Data kind ("dataKind" in DIA)

	DWORD dataKind = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_DATAKIND, &dataKind ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_DATAKIND) failed.") ); 
		return false; 
	}

	Info.dataKind = (DataKind)dataKind; 

		// Location, depending on the data kind 

	switch( dataKind ) 
	{
		case DataIsGlobal: 
		case DataIsStaticLocal: 
		case DataIsFileStatic: 
		case DataIsStaticMember: 
			{
				// Use Address; Offset is not defined

        // Note: If it is DataIsStaticMember, then this is a static member 
				// of a class defined in another module 
        // (it does not have an address in this module) 

				ULONG64 Address = 0; 

				if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_ADDRESS, &Address ) ) 
				{
					DWORD ErrCode = GetLastError(); 
					_ASSERTE( !_T("SymGetTypeInfo(TI_GET_ADDRESS) failed.") ); 
					return false; 
				}

				Info.Address  = Address; 
				Info.Offset   = 0; 

			}
			break; 

		case DataIsLocal:
		case DataIsParam: 
		case DataIsObjectPtr: 
		case DataIsMember: 
			{
				// Use Offset; Address is not defined

				ULONG Offset = 0; 

				if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_OFFSET, &Offset ) ) 
				{
					DWORD ErrCode = GetLastError(); 
					_ASSERTE( !_T("SymGetTypeInfo(TI_GET_OFFSET) failed.") ); 
					return false; 
				}

				Info.Offset   = Offset; 
				Info.Address  = 0; 

			}
			break; 

		default: 
			// Unknown location 
			Info.Address   = 0; 
			Info.Offset    = 0; 
			break; 
	}


	// Success 

	return true; 

}


bool CTypeInfoDump::DumpType( ULONG Index, TypeInfo& Info ) 
{
	// Get the symbol's tag 

	DWORD Tag = SymTagNull; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMTAG, &Tag ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMTAG) failed.") ); 
		return false; 
	}

	Info.Tag = (enum SymTagEnum)Tag; 


	// Dump information about the symbol, depending on its tag 

	bool bResult = false; 

	switch( Tag ) 
	{
		case SymTagBaseType: 
			bResult = DumpBasicType( Index, Info.Info.sBaseTypeInfo ); 
			break; 

		case SymTagPointerType: 
			bResult = DumpPointerType( Index, Info.Info.sPointerTypeInfo ); 
			break; 

		case SymTagTypedef: 
			bResult = DumpTypedef( Index, Info.Info.sTypedefInfo ); 
			break; 

		case SymTagEnum: 
			bResult = DumpEnum( Index, Info.Info.sEnumInfo ); 
			break; 

		case SymTagArrayType: 
			bResult = DumpArrayType( Index, Info.Info.sArrayTypeInfo ); 
			break; 

		case SymTagUDT: 
			bResult = DumpUDT( Index, Info ); 
			break; 

		case SymTagFunctionType: 
			bResult = DumpFunctionType( Index, Info.Info.sFunctionTypeInfo ); 
			break; 

		case SymTagFunctionArgType: 
			bResult = DumpFunctionArgType( Index, Info.Info.sFunctionArgTypeInfo ); 
			break; 

		case SymTagBaseClass: 
			bResult = DumpBaseClass( Index, Info.Info.sBaseClassInfo ); 
			break; 

		case SymTagData: 
			bResult = DumpData( Index, Info.Info.sDataInfo ); 
			break; 

		default: 
			_ASSERTE( !_T("Unsupported symbol.") ); 
			break; 
	}


	// Complete 

	return bResult; 
}

bool CTypeInfoDump::DumpSymbolType( ULONG Index, TypeInfo& Info, ULONG& TypeIndex ) 
{
	// Obtain the index of the symbol's type symbol 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_TYPEID, &TypeIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}

	// Dump the type symbol 

	return DumpType( TypeIndex, Info ); 

}


///////////////////////////////////////////////////////////////////////////////
// Helper functions 
//

bool CTypeInfoDump::CheckTag( ULONG Index, ULONG Tag ) 
{
	DWORD SymTag = SymTagNull; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_SYMTAG, &SymTag ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		//_ASSERTE( !_T("SymGetTypeInfo(TI_GET_SYMTAG) failed.") ); 
		return false; 
	}

	return ( SymTag == Tag ); 

}

bool CTypeInfoDump::SymbolSize( ULONG Index, ULONG64& Size ) 
{
	// Does the symbol support "length" property ? 

	ULONG64 Length = 0; 

	if( SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_LENGTH, &Length ) ) 
	{
		// Yes, it does - return it 

		Size = Length; 
		return true; 
	}
	else 
	{
		// No, it does not - it can be SymTagTypedef 

		if( !CheckTag( Index, SymTagTypedef ) ) 
		{
			// No, this symbol does not have length 
			return false; 
		}
		else 
		{
			// Yes, it is a SymTagTypedef - skip to its underlying type 

			DWORD CurIndex = Index; 

			do 
			{
				DWORD TempIndex = 0; 

				if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_TYPEID, &TempIndex ) ) 
				{
					DWORD ErrCode = GetLastError(); 
					_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
					return false; 
				}

				CurIndex = TempIndex; 
			}
			while( CheckTag( CurIndex, SymTagTypedef ) ); 

			// And get the length 

			if( SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_LENGTH, &Length ) ) 
			{
				Size = Length; 
				return true; 
			}
		}
	}

	// The symbol does not have length 

	return false; 

}

bool CTypeInfoDump::ArrayElementTypeIndex( ULONG ArrayIndex, ULONG& ElementTypeIndex ) 
{
	// Is the symbol really SymTagArrayType ? 

	if( !CheckTag( ArrayIndex, SymTagArrayType ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}


	// Get the array element type 

	DWORD ElementIndex = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, ArrayIndex, TI_GET_TYPEID, &ElementIndex ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
		return false; 
	}


	// If the array element type is SymTagArrayType, 
	// skip to its type 
	
	DWORD CurIndex = ElementIndex; 

	while( CheckTag( ElementIndex, SymTagArrayType ) ) 
	{
		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_TYPEID, &ElementIndex ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
			return false; 
		}

		CurIndex = ElementIndex; 
	}


	// We have found the ultimate type of the array element 

	ElementTypeIndex = ElementIndex; 

	return true; 

}

bool CTypeInfoDump::ArrayDims( ULONG Index, ULONG64* pDims, int& Dims, int MaxDims ) 
{
	// Is the symbol really SymTagArrayType ? 

	if( !CheckTag( Index, SymTagArrayType ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pDims == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxDims <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pDims, MaxDims * sizeof(ULONG64) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate array dimensions 

	DWORD CurIndex = Index; 

	int NumDims = 0; 

	for( int i = 0; i < MaxDims; i++ ) 
	{
		// Length ("length" in DIA) 

		ULONG64 Length = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_LENGTH, &Length ) || ( Length == 0 ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_LENGTH) failed.") ); 
			return false; 
		}

		
		// Size of the current dimension 
		// (it is the size of its SymTagArrayType symbol divided by the size of its 
		// type symbol, which can be another SymTagArrayType symbol or the array element symbol) 

			// Its type 

		DWORD TypeIndex = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_TYPEID, &TypeIndex ) ) 
		{
			// No type - we are done 
			break; 
		}

			// Size of its type 

		ULONG64 TypeSize = 0; 

		if( !SymbolSize( TypeIndex, TypeSize ) || ( TypeSize == 0 ) ) 
		{
			_ASSERTE( !_T("Array type size unknown.") ); 
			return false; 
		}

			// Size of the dimension 

		pDims[i] = Length / TypeSize; 

		NumDims++; 

			// <OS-TEST> Test only 
/*
		DWORD ElemCount = 0; 

		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_COUNT, &ElemCount ) )
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_COUNT) failed.") ); 
			// and continue ... 
		}
		else if( ElemCount != pDims[i] ) 
		{
			_ASSERTE( !_T("TI_GET_COUNT does not match.") ); 
		}
		else 
		{
			//_ASSERTE( !_T("TI_GET_COUNT works!") );
		}
*/

			// If the type symbol is not SymTagArrayType, we are done 

		if( !CheckTag( TypeIndex, SymTagArrayType ) ) 
		{
			break; 
		}


		// Proceed to the next dimension 

		CurIndex = TypeIndex; 

	}


	// Have we got correct results ? 

	if( NumDims == 0 ) 
	{
		// No, something went wrong 
		_ASSERTE( !_T("Number of array dimensions is zero.") ); 
		return false; 
	}


	// Save the number of dimensions 

	Dims = NumDims; 


	// Success 

	return true; 

}

bool CTypeInfoDump::UdtVariables( ULONG Index, ULONG* pVars, int& Vars, int MaxVars ) 
{
	// Is the symbol really SymTagUDT ? 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pVars == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxVars <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pVars, MaxVars * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no member variables 
		Vars = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for variables, 
		// and copy their indexes into the client buffer 

	Vars = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagData ) ) 
		{
			// Yes, this is a member variable 

			pVars[Vars] = pFC->ChildId[i]; 

			Vars++; 

			if( Vars == MaxVars ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::UdtFunctions( ULONG Index, ULONG* pFuncs, int& Funcs, int MaxFuncs ) 
{
	// Is the symbol really SymTagUDT ? 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pFuncs == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxFuncs <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pFuncs, MaxFuncs * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no member variables 
		Funcs = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for functions, 
		// and copy their indexes into the client buffer 

	Funcs = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagFunction ) ) 
		{
			// Yes, this is a member function 

			pFuncs[Funcs] = pFC->ChildId[i]; 

			Funcs++; 

			if( Funcs == MaxFuncs ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::UdtBaseClasses( ULONG Index, ULONG* pBases, int& Bases, int MaxBases ) 
{
	// Is the symbol really SymTagUDT ? 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pBases == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxBases <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pBases, MaxBases * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no member variables 
		Bases = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for base classes, 
		// and copy their indexes into the client buffer 

	Bases = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagBaseClass ) ) 
		{
			// Yes, this is a base class 

			pBases[Bases] = pFC->ChildId[i]; 

			Bases++; 

			if( Bases == MaxBases ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::UdtUnionMembers( ULONG Index, ULONG* pMembers, int& Members, int MaxMembers ) 
{
	// Is the symbol really SymTagUDT ? 

	if( !CheckTag( Index, SymTagUDT ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pMembers == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxMembers <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pMembers, MaxMembers * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no members 
		Members = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for union members, 
		// and copy their indexes into the client buffer 

	Members = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagData ) ) 
		{
			// Yes, this is a member 

			pMembers[Members] = pFC->ChildId[i]; 

			Members++; 

			if( Members == MaxMembers ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::FunctionArguments( ULONG Index, ULONG* pArgs, int& Args, int MaxArgs ) 
{
	// Is the symbol really SymTagFunctionType ? 

	if( !CheckTag( Index, SymTagFunctionType ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pArgs == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxArgs <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pArgs, MaxArgs * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no member variables 
		Args = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for function arguments, 
		// and copy their indexes into the client buffer 

	Args = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagFunctionArgType ) ) 
		{
			// Yes, this is a function argument 

			pArgs[Args] = pFC->ChildId[i]; 

			Args++; 

			if( Args == MaxArgs ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::Enumerators( ULONG Index, ULONG* pEnums, int& Enums, int MaxEnums ) 
{
	// Is the symbol really SymTagEnum ? 

	if( !CheckTag( Index, SymTagEnum ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Check parameters 

	if( pEnums == 0 ) 
	{
		_ASSERTE( !_T("Array pointer is null.") ); 
		return false; 
	}
	
	if( MaxEnums <= 0 ) 
	{
		_ASSERTE( !_T("Array size is invalid.") ); 
		return false; 
	}

#ifdef _DEBUG
	if( IsBadWritePtr( pEnums, MaxEnums * sizeof(ULONG) ) ) 
	{
		_ASSERTE( !_T("Array pointer is invalid.") ); 
		return false; 
	}
#endif // _DEBUG


	// Enumerate children 

		// Get the number of children 

	DWORD NumChildren = 0; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_GET_CHILDRENCOUNT, &NumChildren ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_GET_CHIDRENCOUNT) failed.") ); 
		return false; 
	}

	if( NumChildren == 0 ) 
	{
		// No children -> no enumerators 
		Enums = 0; 
		return true; 
	}

		// Get the children 

	int FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + NumChildren*sizeof(ULONG); 

	TI_FINDCHILDREN_PARAMS* pFC = (TI_FINDCHILDREN_PARAMS*)_alloca( FindChildrenSize ); 

	memset( pFC, 0, FindChildrenSize ); 

	pFC->Count = NumChildren; 

	if( !SymGetTypeInfo( m_hProcess, m_ModBase, Index, TI_FINDCHILDREN, pFC ) ) 
	{
		DWORD ErrCode = GetLastError(); 
		_ASSERTE( !_T("SymGetTypeInfo(TI_FINDCHILDREN) failed.") ); 
		return false; 
	}

		// Enumerate children, looking for enumerators, 
		// and copy their indexes into the client buffer 

	Enums = 0; 

	for( DWORD i = 0; i < NumChildren; i++ ) 
	{
		if( CheckTag( pFC->ChildId[i], SymTagData ) ) 
		{
			// Yes, this is an enumerator 

			pEnums[Enums] = pFC->ChildId[i]; 

			Enums++; 

			if( Enums == MaxEnums ) 
				break; 
		}
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::TypeDefType( ULONG Index, ULONG& UndTypeIndex ) 
{
	// Is the symbol really SymTagTypedef ? 

	if( !CheckTag( Index, SymTagTypedef ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Skip to the type behind the type definition 
	
	DWORD CurIndex = Index; 

	while( CheckTag( CurIndex, SymTagTypedef ) ) 
	{
		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_TYPEID, &UndTypeIndex ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
			return false; 
		}

		CurIndex = UndTypeIndex; 
	}


	// Complete 

	return true; 

}

bool CTypeInfoDump::PointerType( ULONG Index, ULONG& UndTypeIndex, int& NumPointers ) 
{
	// Is the symbol really SymTagPointerType ? 

	if( !CheckTag( Index, SymTagPointerType ) ) 
	{
		_ASSERTE( !_T("Invalid tag.") ); 
		return false; 
	}

	
	// Skip to the type pointer points to 

	NumPointers = 0; 
	
	DWORD CurIndex = Index; 

	while( CheckTag( CurIndex, SymTagPointerType ) ) 
	{
		if( !SymGetTypeInfo( m_hProcess, m_ModBase, CurIndex, TI_GET_TYPEID, &UndTypeIndex ) ) 
		{
			DWORD ErrCode = GetLastError(); 
			_ASSERTE( !_T("SymGetTypeInfo(TI_GET_TYPEID) failed.") ); 
			return false; 
		}

		NumPointers++; 

		CurIndex = UndTypeIndex; 
	}


	// Complete 

	return true; 

}

