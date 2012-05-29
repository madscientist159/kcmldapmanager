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
#include <kurlrequester.h>
#include <kcombobox.h>
#include <tqradiobutton.h>
#include <tqcheckbox.h>
#include <kdatetimewidget.h>

#include "ldapmgr.h"
#include "userconfigdlg.h"

UserConfigDialog::UserConfigDialog(LDAPUserInfo user, LDAPConfig* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP User Properties"), Ok|Cancel, Ok, true), m_user(user), m_ldapconfig(parent)
{
	m_base = new LDAPUserConfigBase(this);
	setMainWidget(m_base);

	TQStringList availableShells = TQStringList::split(" ", AVAILABLE_SHELLS);
	for ( TQStringList::Iterator it = availableShells.begin(); it != availableShells.end(); ++it ) {
		m_base->shell->insertItem(*it, -1);
	}
	m_base->loginName->setEnabled(false);
	m_base->lastChanged->setEnabled(false);

	connect(m_base->passwordExpireEnabled, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->passwordExpireDisabled, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->requirePasswordAging, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->requirePasswordMinAge, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->primaryGroup, TQT_SIGNAL(activated(const TQString&)), this, TQT_SLOT(processLockouts()));

	// Update fields
// 	KPasswordEdit* passwordEntry;

	if (m_user.status == KRB5_DISABLED_ACCOUNT) {
		m_base->userStatusEnabled->setChecked(false);
		m_base->userStatusDisabled->setChecked(true);
	}
	else {
		m_base->userStatusEnabled->setChecked(true);
		m_base->userStatusDisabled->setChecked(false);
	}
	m_base->loginName->setText(m_user.name);
	m_base->realName->setText(m_user.commonName);
	m_base->UID->setValue(m_user.uid);

	LDAPGroupInfoList groupList = m_ldapconfig->groupList();
	LDAPGroupInfoList::Iterator it;
	for (it = groupList.begin(); it != groupList.end(); ++it) {
		m_base->primaryGroup->insertItem((*it).name, -1);
	}
	m_base->primaryGroup->setCurrentItem(m_ldapconfig->findGroupInfoByGID(TQString("%1").arg(m_user.primary_gid)).name, false, -1);
	m_prevPrimaryGroup = m_base->primaryGroup->currentText();

	m_base->homeDirectory->setURL(m_user.homedir);
	m_base->shell->setEditText(m_user.shell);

	for (it = groupList.begin(); it != groupList.end(); ++it) {
		LDAPGroupInfo group = *it;
		TQCheckListItem* item = new TQCheckListItem(m_base->secondary_group_list, group.name, TQCheckListItem::CheckBox);
		item->setOn(group.userlist.contains(m_user.distinguishedName));
	}

// 	m_base->passwordEntry;
	m_base->lastChanged->setText(m_user.password_last_changed.toString(TQt::TextDate));
	if (m_user.password_expires) {
		m_base->passwordExpireEnabled->setChecked(true);
		m_base->passwordExpireDisabled->setChecked(false);
	}
	else {
		m_base->passwordExpireEnabled->setChecked(false);
		m_base->passwordExpireDisabled->setChecked(true);
	}
	m_base->expirationDate->setDateTime(m_user.password_expiration);
	m_base->requirePasswordAging->setChecked(m_user.password_ages);
	m_base->requirePasswordInterval->setValue(m_user.new_password_interval/24);
	m_base->warnPasswordExpireInterval->setValue(m_user.new_password_warn_interval/24);
	m_base->disablePasswordDelay->setValue(m_user.new_password_lockout_delay/24);
	m_base->requirePasswordMinAge->setChecked(m_user.password_has_minimum_age);
	m_base->passwordMinAge->setValue(m_user.password_minimum_age/24);

	processLockouts();
}

void UserConfigDialog::slotOk() {
	// Update data
	// RAJA FIXME

	accept();
}

void UserConfigDialog::processLockouts() {
	if (m_base->passwordExpireEnabled->isChecked()) {
		m_base->expirationDate->setEnabled(true);
	}
	else {
		m_base->expirationDate->setEnabled(false);
	}

	if (m_base->requirePasswordAging->isChecked()) {
		m_base->requirePasswordInterval->setEnabled(true);
		m_base->warnPasswordExpireInterval->setEnabled(true);
		m_base->disablePasswordDelay->setEnabled(true);
	}
	else {
		m_base->requirePasswordInterval->setEnabled(false);
		m_base->warnPasswordExpireInterval->setEnabled(false);
		m_base->disablePasswordDelay->setEnabled(false);
	}

	if (m_base->requirePasswordMinAge->isChecked()) {
		m_base->passwordMinAge->setEnabled(true);
	}
	else {
		m_base->passwordMinAge->setEnabled(false);
	}

	// Disable the primary group checkbox in the group list
	TQListViewItemIterator it(m_base->secondary_group_list);
	while (it.current()) {
		if (it.current()->text(0) == m_base->primaryGroup->currentText()) {
			dynamic_cast<TQCheckListItem*>(it.current())->setOn(true);
			it.current()->setEnabled(false);
		}
		else {
			it.current()->setEnabled(true);
			if (it.current()->text(0) == m_prevPrimaryGroup) {
				dynamic_cast<TQCheckListItem*>(it.current())->setOn(false);
			}
		}
		++it;
	}

	m_prevPrimaryGroup = m_base->primaryGroup->currentText();
}

LDAPUserInfo UserConfigDialog::userProperties() {
	return m_user;
}

#include "userconfigdlg.moc"
