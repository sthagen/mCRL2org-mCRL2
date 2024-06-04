// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCRL2_DATA_POS_H
#define MCRL2_DATA_POS_H

#ifdef MCRL2_ENABLE_MACHINENUMBERS
    #include "pos64.h"
#else
    #include "pos1.h"
#endif // MCRL2_ENABLE_MACHINENUMBERS

#endif // MCRL2_DATA_POS_H