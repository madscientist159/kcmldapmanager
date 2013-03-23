/***************************************************************************
 *   Copyright (C) 2013 by Timothy Pearson                                 *
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
#include "machineconfigdlg.h"

MachineConfigDialog::MachineConfigDialog(LDAPMachineInfo machine, TQString realmName, LDAPConfig* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP Machine Properties"), Ok|Cancel, Ok, true), m_machine(machine), m_ldapconfig(parent)
{
	m_base = new LDAPMachineConfigBase(this);
	setMainWidget(m_base);

	m_base->detailsIcon->setPixmap(SmallIcon("system.png"));

	m_base->realmNameLabel->setText("."+realmName.lower());

	connect(m_base->machineName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->specifiedPassword, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));
	connect(m_base->autoGeneratePassword, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
	connect(m_base->manuallySpecifyPassword, TQT_SIGNAL(clicked()), this, TQT_SLOT(manuallySpecifyPasswordClicked()));

	m_base->autoGeneratePassword->setChecked(true);
	m_base->manuallySpecifyPassword->setChecked(false);

	m_base->machineName->setFocus();

	processLockouts();
}

void MachineConfigDialog::slotOk() {
	m_machine.name = m_base->machineName->text();
	if (m_base->autoGeneratePassword->isOn() == true) {
		m_machine.newPassword = TQString();
	}
	else if (m_base->manuallySpecifyPassword->isOn() == true) {
		m_machine.newPassword = m_base->specifiedPassword->password();
	}

	accept();
}

void MachineConfigDialog::processLockouts() {
	m_base->specifiedPassword->setEnabled(m_base->manuallySpecifyPassword->isOn());

	if (((m_base->manuallySpecifyPassword->isOn() == true) && (strcmp(m_base->specifiedPassword->password(), "") == 0)) || (m_base->machineName->text() == "")) {
		enableButton(KDialogBase::Ok, false);
	}
	else {
		enableButton(KDialogBase::Ok, true);
	}
}

void MachineConfigDialog::manuallySpecifyPasswordClicked() {
	processLockouts();

	if (m_base->specifiedPassword->isEnabled()) {
		m_base->specifiedPassword->setFocus();
	}
}

LDAPMachineInfo MachineConfigDialog::machineProperties() {
	return m_machine;
}

#include "machineconfigdlg.moc"
