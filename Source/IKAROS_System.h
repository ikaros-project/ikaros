//
//	IKAROS_System.h		Definition of the target system for which to compile
//
//    Copyright (C) 2001-2010  Christian Balkenius
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
//	Target system can be set using a compiler flag, e.g., -DMAC_OS_X or -DWINDOWS
//	Alternatively, the appropriate #define below can be uncommented
//

//#define MINIMAL
//#define MAC_OS_X
//#define LINUX
//#define WINDOWS
//#define WINDOWS32


//
//  IKAROSPATH is the path to the Ikaros root directory relative to the Ikaros binary  
//  OR the absolute path to the Ikaros root directory. A relative path is useful when
//  Ikaros is run from within an IDE. An absolut path MUST be set if the full path 
//  is not used when starting Ikaros. This is the case when the search path for
//  executable files includes the directory with the Ikaros binary.
//
//  An absolute path can be set using a compiler flag, for example: 
//  -DIKAROSPATH="\"/home/me/Ikaros.1.3/\""
//  -DIKAROSPATH="C:\\Users\\me\\ikaros\\" (Windows)
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
//#define USE_THREADED_WEBUI
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
//#define USE_THREADED_WEBUI    // 1.3
#endif



//
// Windows Defines
//

#ifdef WINDOWS
#define PLATFORM  "WINDOWS"

#define USE_WIN_SOCKET
#define USE_LIBJPEG
#define USE_THREADS
//#define USE_LIBPNG
//#define USE_BLAS
#define USE_WIN_SERIAL
//#define USE_THREADED_WEBUI
#include <direct.h> // Include chdir
#define chdir _chdir
#define getcwd _getcwd

#define M_PI 3.14159265358979323846264338328f
#undef min
#undef max

#ifdef WINDOWS32
//#define vsnprintf _vsnprintf
#define snprintf _snprintf
#define PATH_MAX 255
#endif
//#define POSIX
//#define USE_QUICKTIME
//#define USE_MPI
#endif

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE (1)
#define _CRT_NONSTDC_NO_DEPRECATE
#pragma warning( disable : 4305)
#pragma warning( disable : 4244) 
#pragma warning( disable : 4290) 
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
#ifdef USE_WIN_SOCKET
#define USE_SOCKET
#endif

// Fix for missing MSG_NOSIGNAL in OS X to pass compilation

#ifdef MAC_OS_X
#define MSG_NOSIGNAL 0
#endif
