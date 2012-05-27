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

#ifndef _LIBTDELDAP_H_
#define _LIBTDELDAP_H_

#include <unistd.h>

#include <tqstring.h>
#include <tqdatetime.h>
#include <tqvaluelist.h>

enum LDAPUserStatus {
	USER_STATUS_ENABLED,
	USER_STATUS_DISABLED
};

typedef TQValueList<uid_t> UserList;
typedef TQValueList<gid_t> GroupList;

class LDAPUserInfo
{
	public:
		TQString name;
		uid_t uid;
		TQString shell;
		TQString homedir;
		gid_t primary_gid;
		GroupList grouplist;
		LDAPUserStatus status;
		TQDate password_last_changed;
		bool password_expires;
		TQDate password_expiration;
		bool password_ages;
		int new_password_interval;
		int new_password_warn_interval;
		int new_password_lockout_delay;
		bool password_has_minimum_age;
		int password_minimum_age;

		TQString realName;
		TQString organization;
		// FIXME
		// Add other attributes (cubicle, phone number, etc)
};

class LDAPGroupInfo
{
	public:
		TQString name;
		gid_t gid;
		UserList userlist;
};

#endif // _LIBTDELDAP_H_