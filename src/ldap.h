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

#ifndef _LDAP_H_
#define _LDAP_H_

#include <kcmodule.h>
#include <kaboutdata.h>
#include <kpushbutton.h>
#include <klistview.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <tqpushbutton.h>
#include <tqcombobox.h>

#include "ldapconfigbase.h"

class ldap: public KCModule
{
	Q_OBJECT

	public:
		ldap( TQWidget *parent=0, const char *name=0, const TQStringList& = TQStringList() );
		~ldap();
		
		virtual void load();
		virtual void save();
		virtual void defaults();
		virtual int buttons();
		virtual TQString quickHelp() const;
		virtual const KAboutData *aboutData() const { return myAboutData; };

	private:
		KAboutData *myAboutData;
		KGlobalSettings *kgs;
};

#endif
