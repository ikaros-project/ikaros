//
//	UserModules.h		This file is a part of the IKAROS project
//						This file includes the h-files of all user modules
// 						It contains InitUserModules which is called at start-up
//
//    Copyright (C) 2003-2011	Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//
//	Created: 2003-04-20
//
//	Include all your modules here

#include "UserModules/MyModule/MyModule.h"              // Change this to the name of your module
#include "UserModules/MinimalModule/MinimalModule.h"    // Change this to the name of your module

void InitUserModules(Kernel & k);

void
InitUserModules(Kernel & k)
{
//	Add a line for each user module here

    k.AddClass("MyModule", &MyModule::Create, "Source/UserModules/MyModule/");                  // Change this to the name and path of your module
    k.AddClass("MinimalModule", &MinimalModule::Create, "Source/UserModules/MinimalModule/");	// Change this to the name and path of your module
}


