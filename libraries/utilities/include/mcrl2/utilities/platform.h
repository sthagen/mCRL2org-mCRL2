// Author(s): Maurice Laveaux
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MCRL2_UTILITIES_PLATFORM_H_
#define MCRL2_UTILITIES_PLATFORM_H_

/// \brief Define macros to conditionally enable platform specific code.
#ifdef _WIN32
#define MCRL2_PLATFORM_WINDOWS 1
#endif

#ifdef __linux
#define MCRL2_PLATFORM_LINUX 1
#endif

#ifdef __FreeBSD__
#define MCRL2_PLATFORM_FREEBSD 1
#endif

#ifdef __APPLE__
#define MCRL2_PLATFORM_MAC 1
#endif

#endif // MCRL2_UTILITIES_PLATFORM_H_
