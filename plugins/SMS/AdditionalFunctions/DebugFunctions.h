#if !defined(AFX_DEBUG_FUNCTIONS__H__INCLUDED_)
#define AFX_DEBUG_FUNCTIONS__H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef szCRLF
#define szCRLF						TEXT("\r\n")
#endif
//////////////////////////////////////////////////////////////////////////
////////////////////////////DebugPrint////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// DebugBuildPrint(Helo world);
// ����������� ������ � Debug ���� �� ����� �������
#ifdef _DEBUG
	#define DebugPrintA(szText) OutputDebugStringA((szText))
	#define DebugPrintW(szText) OutputDebugStringW((szText))
	#define DebugPrintCRLFA(szText) OutputDebugStringA((szText));OutputDebugStringA("\r\n")
	#define DebugPrintCRLFW(szText) OutputDebugStringW((szText));OutputDebugStringW(L"\r\n")
#else
	#define DebugPrintA(szText)
	#define DebugPrintW(szText)
	#define DebugPrintCRLFA(szText)
	#define DebugPrintCRLFW(szText)
#endif //_DEBUG


#ifdef UNICODE
	#define DebugPrint DebugPrintW
	#define DebugPrintCRLF DebugPrintCRLFW
#else
	#define DebugPrint DebugPrintA
	#define DebugPrintCRLF DebugPrintCRLFA
#endif
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
////////////////////////////DebugBuildPrint///////////////////////////////
//////////////////////////////////////////////////////////////////////////
// #pragma DebugBuildPrint(Helo world);
// ����������� ��������� � Build ���� �� ����� ����������
#ifdef _DEBUG
	#pragma warning(disable:4081)
	#define chSTR2(x)	#x
	#define chSTR(x)	chSTR2(x)
	#define DebugBuildPrint(szText) message(__FILE__ "(" chSTR(__LINE__) "): " #szText)
	#pragma warning(default:4081)
#else
	#define DebugBuildPrint(szText)
#endif //_DEBUG
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
////////////////////////////DebugBufferFill///////////////////////////////
//////////////////////////////////////////////////////////////////////////
// DebugBufferFill(szString,sizeof(szString));
// ��������� ��������� ���������� ������ �������� "A", �����������
// ��� ��������� ������� �� ����� � �������.
#ifdef _DEBUG
	#define DebugBufferFill(lpBuffer,dwSize) memset(lpBuffer,'A',dwSize)
#else
	#define DebugBufferFill(lpBuffer,dwSize)
#endif //_DEBUG
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
////////////////////////////DebugBreak////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// DebugBreak();
// ����� ��������, ����� ������� ������������ API
#if defined(_DEBUG) && defined(_X86_)
	#define DebugBreak() _asm{int 3}
#else
	#define DebugBreak()
#endif //_DEBUG
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////






#endif // !defined(AFX_DEBUG_FUNCTIONS__H__INCLUDED_)
