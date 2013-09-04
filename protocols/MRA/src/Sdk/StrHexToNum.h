/*
 * Copyright (c) 2005 Rozhuk Ivan <rozhuk.im@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */



#if !defined(AFX_STRHEXTONUM__H__INCLUDED_)
#define AFX_STRHEXTONUM__H__INCLUDED_



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


__inline DWORD StrHexToUNum32(LPCSTR lpcszString,size_t dwStringLen)
{
	DWORD dwNum=0;
	BYTE bCurentFigure;


	while(dwStringLen)
	{
		bCurentFigure=(*lpcszString);
		if ('0'<=bCurentFigure && bCurentFigure<='9')
		{
			bCurentFigure-='0';
		}else
		if ('a'<=bCurentFigure && bCurentFigure<='f')
		{
			bCurentFigure-=('a'+10);
		}else
		if ('A'<=bCurentFigure && bCurentFigure<='F')
		{
			bCurentFigure-=('A'+10);
		}else{
			bCurentFigure=255;
		}

		if (bCurentFigure!=255)
		{
			dwNum*=16;// �������� ���������� ����� �� ���� ������ ���� �������� � ������� ������ ����� �����
			dwNum+=bCurentFigure;// ��������� ����� � ������� ������
		}
		lpcszString++;// ���������� ��������� �� ��������� �������
		dwStringLen--;// ��������� ������
	}

return(dwNum);
}


#endif // !defined(AFX_STRHEXTONUM__H__INCLUDED_)