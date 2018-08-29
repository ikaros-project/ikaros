//
//	IKAROS_System.h		Definition of the target system for which to compile
//
//    Copyright (C) 2001-2018  Christian Balkenius
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
//
//    Created: August 22, 2002 
//



//
//	Target system can be set using a compiler flag, e.g., -DMAC_OS_X
//	Alternatively, the appropriate #define below can be uncommented
//

//#define MINIMAL
//#define MAC_OS_X
//#define LINUX



//
//  IKAROSPATH is the path to the Ikaros root directory relative to the Ikaros binary  
//  OR the absolute path to the Ikaros root directory. A relative path is useful when
//  Ikaros is run from within an IDE. An absolut path MUST be set if the full path 
//  is not used when starting Ikaros. This is the case when the search path for
//  executable files includes the directory with the Ikaros binary.
//
//  An absolute path can be set using a compiler flag, for example: 
//  -DIKAROSPATH="\"/home/me/Ikaros.1.3/\""
//

#ifndef IKAROSPATH
#define IKAROSPATH	"../"
#endif


//
// Minimal Defines
//

// If Ikaros does not compile, try starting with the minmal version by declaring MINIMAL
// This will compile a minmal Ikaros version without support for sockets, WebUI, or threads.
// The minimal version is plain C++ using only the standard libraries and should compile on
// any system.

#ifdef MINIMAL
#define PLATFORM  "MINIMAL-UNKOWN"
//#define POSIX
//#define USE_BSD_SOCKET
//#define USE_LIBJPEG
//#define USE_THREADS
//#define USE_BLAS
//#define USE_MPI
#endif



//
// Linux Defines
//

#ifdef LINUX
#define PLATFORM  "LINUX"
#define POSIX
#define USE_BSD_SOCKET
//#define USE_QUICKTIME
#define USE_LIBJPEG
//#define USE_V4L2
//#define USE_KEEPON
#define USE_THREADS
#define USE_BLAS
#define USE_LIBPNG
//#define USE_MPI
#endif



//
// Mac OS X Defines
//

#ifdef MAC_OS_X
#define PLATFORM  "MAC OS X"
#define POSIX
#define USE_BSD_SOCKET
#define USE_QUICKTIME
//#define USE_QUICKTIME_OLD
#define USE_LIBJPEG
#define USE_LIBPNG
//#define USE_VDSP
#define USE_VIMAGE
#define USE_VFORCE
#define USE_BLAS
#define USE_LAPACK
#define USE_THREADS
#define USE_MPI
#endif


//
// Unix includes (some distributions lack PATH_MAX)
//

#ifdef LINUX
#include <limits.h>
#endif

#ifdef MAC_OS_X
#include <limits.h>
#endif


// Include the socket header file if some socket library is used

#ifdef USE_BSD_SOCKET
#define USE_SOCKET
#endif

// Fix for missing MSG_NOSIGNAL in OS X to pass compilation

#ifdef MAC_OS_X
#define MSG_NOSIGNAL 0
#endif
