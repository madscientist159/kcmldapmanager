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

#ifndef _MACHINECONFIGDIALOG_H_
#define _MACHINECONFIGDIALOG_H_

#include <kdialogbase.h>

#include "libtdeldap.h"
#include "machineconfigbase.h"

class MachineConfigDialog : public KDialogBase
{
	Q_OBJECT

public:
	MachineConfigDialog(LDAPMachineInfo machine, TQString realmName, LDAPConfig* parent = 0, const char* name = 0);
	LDAPMachineInfo machineProperties();

public slots:
	void slotOk();
	void processLockouts();
	void manuallySpecifyPasswordClicked();

public:
	LDAPMachineConfigBase *m_base;
	LDAPMachineInfo m_machine;

private:
	LDAPConfig* m_ldapconfig;
};

#endif // _MACHINECONFIGDIALOG_H_
