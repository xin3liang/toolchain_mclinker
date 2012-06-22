//===- SystemUtils.h ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_SYSTEM_UTILS_H
#define MCLD_SYSTEM_UTILS_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

namespace mcld {
namespace sys {

typedef uint8_t* Address;
typedef off_t Offset;

/** \fn strerror
 *  \brief system error message
 */
char *strerror(int pErrnum);

} // namespace of sys
} // namespace of mcld

#endif

