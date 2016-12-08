//
//    Logger.cc		This file is a part of the IKAROS project
//
//    Copyright (C) 2016  Christian Balkenius
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


#include "Logger.h"
#include "IKAROS.h"

using namespace ikaros;

void
Logger::Init()
{
    Socket socket;
    char b[2014];
    socket.Get("www.ikaros-project.org", 80, "GET /start/ HTTP/1.1\r\nHost: www.ikaros-project.org\r\nConnection: close\r\n\r\n", b, 1024);
    socket.Close();
}



static InitClass init("Logger", &Logger::Create, "Source/Modules/UtilityModules/Logger/");
