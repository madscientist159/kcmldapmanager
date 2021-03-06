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

#ifndef _SERVICECONFIGDIALOG_H_
#define _SERVICECONFIGDIALOG_H_

#include <kdialogbase.h>

#include "libtdeldap.h"
#include "serviceconfigbase.h"

class ServiceConfigDialog : public KDialogBase
{
	Q_OBJECT

public:
	ServiceConfigDialog(LDAPServiceInfo service, LDAPConfig* parent = 0, const char* name = 0);
	LDAPServiceInfo serviceProperties();

public slots:
	void slotOk();
	void processLockouts();

public:
	LDAPServiceConfigBase *m_base;
	LDAPServiceInfo m_service;

private:
	LDAPConfig* m_ldapconfig;
};

#endif // _SERVICECONFIGDIALOG_H_
