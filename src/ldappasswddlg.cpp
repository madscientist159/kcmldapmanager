/***************************************************************************
 *   Copyright (C) 2012 by Timothy Pearson                                 *
 *   kb9vqf@pearsoncomputing.net                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <klocale.h>
#include <klineedit.h>
#include <ktextedit.h>
#include <knuminput.h>
#include <kactionselector.h>
#include <tqlistbox.h>
#include <kpushbutton.h>
#include <tqpixmap.h>
#include <tqiconset.h>
#include <tqlabel.h>

#include "ldappasswddlg.h"

LDAPPasswordDialog::LDAPPasswordDialog(TQWidget* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP Authentication"), Ok|Cancel, Ok, true)
{
	m_base = new LDAPLogin(this);

	m_base->px_introSidebar->hide();
	m_base->yad_string->hide();

	setMainWidget(m_base);
}

void LDAPPasswordDialog::slotOk() {
	accept();
}

#include "ldappasswddlg.moc"
