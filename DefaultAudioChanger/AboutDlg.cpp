/******************************************************************************
* This file is part of DefaultAudioChanger.
* Copyright (c) 2011 Sergiu Giurgiu 
*
* DefaultAudioChanger is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DefaultAudioChanger is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DefaultAudioChanger.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());
	gplLink.SubclassWindow(GetDlgItem(IDC_GPL_LINK_STATIC));
	gplLink.SetHyperLink(L"http://www.gnu.org/licenses/gpl-3.0-standalone.html");
	gplLink.SetHyperLinkExtendedStyle(HLINK_UNDERLINED|HLINK_AUTOCREATELINKFONT);
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
