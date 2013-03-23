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

#include <tdelocale.h>
#include <klineedit.h>
#include <ktextedit.h>
#include <knuminput.h>
#include <tdeactionselector.h>
#include <tqlistbox.h>
#include <kpushbutton.h>
#include <tqpixmap.h>
#include <tqiconset.h>
#include <tqlabel.h>
#include <kurlrequester.h>
#include <kcombobox.h>
#include <tqradiobutton.h>
#include <tqcheckbox.h>
#include <kdatetimewidget.h>
#include <kiconloader.h>

#include "ldapmgr.h"
#include "groupconfigdlg.h"

GroupConfigDialog::GroupConfigDialog(LDAPGroupInfo group, LDAPConfig* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP Group Properties"), Ok|Cancel, Ok, true), m_group(group), m_ldapconfig(parent)
{
	m_base = new LDAPGroupConfigBase(this);
	setMainWidget(m_base);

	m_base->addToGroup->setText(i18n("-->"));
	m_base->removeFromGroup->setText(i18n("<--"));
	if (group.distinguishedName != "") {
		m_base->groupName->setEnabled(false);
	}

	m_base->detailsIcon->setPixmap(SmallIcon("tdmconfig.png"));

	connect(m_base->addToGroup, TQT_SIGNAL(clicked()), this, TQT_SLOT(addSelectedUserToGroup()));
	connect(m_base->removeFromGroup, TQT_SIGNAL(clicked()), this, TQT_SLOT(removeSelectedUserFromGroup()));
	connect(m_base->groupName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));

	// Update fields
	m_base->groupName->setText(m_group.name);
	m_base->groupID->setValue(m_group.gid);

	LDAPUserInfoList userList = m_ldapconfig->userList();
	LDAPUserInfoList::Iterator it;
	for (it = userList.begin(); it != userList.end(); ++it) {
		LDAPUserInfo user = *it;
		if (group.userlist.contains(user.distinguishedName)) {
			(void)new TQListBoxText(m_base->selectedAccounts, user.name);
		}
		else {
			(void)new TQListBoxText(m_base->availableAccounts, user.name);
		}
	}
	m_base->availableAccounts->sort(true);
	m_base->selectedAccounts->sort(true);

	processLockouts();
}

void GroupConfigDialog::slotOk() {
	unsigned int i;

	// Update data
	m_group.gid = m_base->groupID->value();
	TQStringList userlist;
	for (i=0;i<m_base->selectedAccounts->count();i++) {
		TQListBoxText* itm = dynamic_cast<TQListBoxText*>(m_base->selectedAccounts->item(i));
		if (itm) {
			userlist.append(m_ldapconfig->findUserInfoByName(itm->text()).distinguishedName);
		}
	}
	m_group.userlist = userlist;

	// Special handler for new group
	if (m_group.distinguishedName == "") {
		m_group.name = m_base->groupName->text();
	}

	accept();
}

void GroupConfigDialog::processLockouts() {
	// Special handler for new group
	if ((m_group.distinguishedName == "") && (m_base->groupName->text() == "")) {
		enableButton(KDialogBase::Ok, false);
	}
	else {
		enableButton(KDialogBase::Ok, true);
	}
}

void GroupConfigDialog::addSelectedUserToGroup() {
	TQListBoxText* itm = dynamic_cast<TQListBoxText*>(m_base->availableAccounts->selectedItem());
	if (itm) {
		(void)new TQListBoxText(m_base->selectedAccounts, itm->text());
		delete itm;
	}
	m_base->availableAccounts->sort(true);
	m_base->selectedAccounts->sort(true);
}

void GroupConfigDialog::removeSelectedUserFromGroup() {
	TQListBoxText* itm = dynamic_cast<TQListBoxText*>(m_base->selectedAccounts->selectedItem());
	if (itm) {
		(void)new TQListBoxText(m_base->availableAccounts, itm->text());
		delete itm;
	}
	m_base->availableAccounts->sort(true);
	m_base->selectedAccounts->sort(true);
}

LDAPGroupInfo GroupConfigDialog::groupProperties() {
	return m_group;
}

#include "groupconfigdlg.moc"
