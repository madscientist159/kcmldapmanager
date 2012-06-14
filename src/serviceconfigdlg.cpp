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
#include <kiconloader.h>

#include "ldapmgr.h"
#include "serviceconfigdlg.h"

ServiceConfigDialog::ServiceConfigDialog(LDAPServiceInfo service, LDAPConfig* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("LDAP Service Properties"), Ok|Cancel, Ok, true), m_service(service), m_ldapconfig(parent)
{
	int i;

	m_base = new LDAPServiceConfigBase(this);
	setMainWidget(m_base);

	// Populate machine list
	LDAPMachineInfoList machineList = m_ldapconfig->machineList();
	LDAPMachineInfoList::Iterator it;
	for (it = machineList.begin(); it != machineList.end(); ++it) {
		LDAPMachineInfo machine = *it;
		m_base->hostMachine->insertItem(machine.name, -1);
	}

	m_base->detailsIcon->setPixmap(SmallIcon("kcmsystem.png"));

	connect(m_base->serviceName, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(processLockouts()));

	// Update fields
	m_base->serviceName->setText(m_service.name);
	for (i=0; i<m_base->hostMachine->count(); i++) {
		if (m_base->hostMachine->text(i).lower() == m_service.machine.lower()) {
			m_base->hostMachine->setCurrentItem(i);
		}
	}

	m_base->serviceName->setFocus();

	processLockouts();
}

void ServiceConfigDialog::slotOk() {
	// Special handler for new service
	if (m_service.distinguishedName == "") {
		m_service.name = m_base->serviceName->text();
		m_service.machine = m_base->hostMachine->currentText();
	}

	accept();
}

void ServiceConfigDialog::processLockouts() {
	// Special handler for new group
	if ((m_service.distinguishedName == "") && (m_base->serviceName->text() == "")) {
		enableButton(KDialogBase::Ok, false);
	}
	else {
		enableButton(KDialogBase::Ok, true);
	}
}

LDAPServiceInfo ServiceConfigDialog::serviceProperties() {
	return m_service;
}

#include "serviceconfigdlg.moc"
