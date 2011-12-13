//===- StaticResolver.cpp -------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "mcld/LD/StaticResolver.h"
#include "mcld/LD/LDSymbol.h"
#include <cassert>

using namespace std;
using namespace mcld;


//==========================
// StaticResolver
StaticResolver::StaticResolver()
{
}

StaticResolver::~StaticResolver()
{
}

unsigned int StaticResolver::resolve(ResolveInfo & __restrict__ pOld,
                                     const ResolveInfo & __restrict__ pNew,
                                     bool &pOverride)
{

  /* The state table itself.
   * The first index is a link_row and the second index is a bfd_link_hash_type.
   *
   * Cs -> all rest kind of common (d_C, wd_C)
   * Is -> all kind of indeirect
   */
  static const enum LinkAction link_action[LAST_ORD][LAST_ORD] =
  {
    /* new\old  D      w_D    d_D    wd_D   U       w_U     d_U    wd_U   C      w_C,   Cs,    Is */
    /* D    */ {MDEF,  DEF,   DEF,   DEF,   DEF,    DEF,    DEF,   DEF,   CDEF,  CDEF,  CDEF,  MDEF },
    /* w_D  */ {NOACT, NOACT, DEFW,  DEFW,  DEFW,   DEFW,   DEFW,  DEFW,  NOACT, NOACT, NOACT, NOACT},
    /* d_D  */ {NOACT, NOACT, NOACT, NOACT, MDEFD,  MDEFD,  DEFD,  DEFD,  NOACT, NOACT, NOACT, MDEF },
    /* wd_D */ {NOACT, NOACT, NOACT, NOACT, MDEFWD, MDEFWD, DEFWD, DEFWD, NOACT, NOACT, NOACT, NOACT},
    /* U    */ {NOACT, NOACT, DUND,  DUNDW, NOACT,  UND,    UND,   UND,   NOACT, NOACT, NOACT, REFC },
    /* w_U  */ {NOACT, NOACT, DUND,  DUNDW, NOACT,  NOACT,  NOACT, WEAK,  NOACT, NOACT, NOACT, REFC },
    /* d_U  */ {NOACT, NOACT, NOACT, NOACT, NOACT,  NOACT,  NOACT, NOACT, NOACT, NOACT, NOACT, REFC },
    /* wd_U */ {NOACT, NOACT, NOACT, NOACT, NOACT,  NOACT,  NOACT, NOACT, NOACT, NOACT, NOACT, REFC },
    /* C    */ {CREF,  COM,   COM,   COM,   COM,    COM,    COM,   COM,   MBIG,  COM,   BIG,   REFC },
    /* w_C  */ {NOACT, NOACT, NOACT, NOACT, COM,    COM,    COM,   COM,   NOACT, NOACT, NOACT, REFC },
    /* Cs   */ {NOACT, NOACT, NOACT, NOACT, COM,    COM,    COM,   COM,   MBIG,  MBIG,  MBIG,  REFC },
    /* Is   */ {MDEF,  IND,   IND,   IND,   IND,    IND,    IND,   IND,   CIND,  CIND,  CIND,  MIND }
  };

  // Special cases:
  // * when a dynamic defined symbol meets a dynamic weak defined symbol, act
  //   noting.
  // * when a undefined symbol meets a dynamic defined symbol, override by
  //   dynamic defined first, then recover back to undefined symbol later.
  // * when a dynamic defined symbol meets a undefined symbol or a weak
  //   undefined symbol, do not override, instead of marking.
  // * When a undefined symbol meets a dynamic defined symbol or a weak
  //   undefined symbol meets a dynamic defined symbol, should override.
  // * When a common symbol meets a weak common symbol, adjust the size of
  //   common symbol (ref: Google gold linker: resolve.cc)

  unsigned int row = getOrdinate(pNew);
  unsigned int col = getOrdinate(pOld);

  bool cycle = false;
  bool pOverride = false;
  ResolveInfo* old = &pOld;
  do {
    enum LinkerAction action;
    unsigned int result = Resolver::Success;

    cycle = false;
    action = link_action[row][col];
    switch(action) {
      case FAIL: {       /* abort.  */
        result = Resolver::Abort;
        break;
      }
      case NOACT: {      /* no action.  */
        pOverride = false;
        old->overrideVisibility(pNew);
        result = Resolver::Success;
        break;
      }
      case UND:          /* override by symbol undefined symbol.  */
      case WEAK:         /* override by symbol weak undefined.  */
      case DEF:          /* override by symbol defined.  */
      case DEFW:         /* override by symbol weak defined.  */
      case DEFD:         /* override by symbol dynamic defined.  */
      case DEFWD:        /* override by symbol dynamic weak defined. */
      case COM: {        /* override by symbol common defined.  */
        pOverride = true;
        old->override(pNew);
        result = Resolver::Success;
        break;
      }
      case MDEFD:        /* mark symbol dynamic defined.  */
      case MDEFWD: {     /* mark symbol dynamic weak defined.  */
        uint32_t binding = old->binding();
        old->override(pNew);
        old->setBinding(ResolveInfo::Weak);
        m_Mesg = "symbol `"+ old->name() + "' uses the type, dynamic, size and type in the dynamic symbol.";
        pOverride = true;
        result = Resolver::Warning;
        break;
      }
      case DUND:
      case DUNDW: {
        old->setBinding(pNew.binding());
        old->overrideVisibility(pNew);
        m_Mesg = "dynamic symbol `"+ old->name() +"' uses the binding in the regular file.";
        pOverride = false;
        result = Resolver::Warning;
        break;
      }
      case CREF: {       /* Possibly warn about common reference to defined symbol.  */
        // A common symbol does not override a definition.
        m_Mesg = "common '" +pNew.name()+"' overriden by previous definition.";
        pOverride = false;
        result = Resolver::Warning;
        break;
      }
      case CDEF: {       /* redefine existing common symbol.  */
        // We've seen a common symbol and now we see a definition.  The
        // definition overrides.
        m_Mesg = "definition of '"+old->name()+"' is overriding common.";
        old->override(pNew);
        pOverride = true;
        result = Resolver::Warning;
        break;
      }
      case BIG: {        /* override by symbol common using largest size.  */
        if (old->size() < pNew.size())
          old->setSize(pNew.size());
        old->setValue(pNew.value());
        old->overrideAttributes(pNew);
        old->overrideVisibility(pNew);
        pOverride = true;
        result = Resolver::Success;
        break;
      }
      case MBIG: {       /* mark common symbol by larger size. */
        if (old->size() < pNew.size())
          old->setSize(pNew.size());
        old->overrideVisibility(pNew);
        pOverride = false;
        result = Resolver::Success;
        break;
      }
      case CIND: {       /* mark indirect symbol from existing common symbol.  */
         m_Mesg = "indirect symbol `"+pNew.name()+"' point to a common symbol.\n";
         result = Resolver::Warning;
      }
      /* Fall through */
      case IND: {        /* override by indirect symbol.  */
        if (0 == pNew.link()) {
          m_Mesg = "indirect symbol `"+pNew.name()+"' point to a inexistent symbol.";
          result = Resolver::Abort;
          break;
        }

        /** Should detect the loop of indirect symbol during file reading **/
        // if (pNew.link()->isIndirect() && pNew.link()->link() == &pNew) {
        //  m_Mesg = "indirect symbol `"+pNew.name()+"' to `"+pNew.link()->name()+"' is a loop.";
        //  return Resolver::Abort;
        //}

        // change the old symbol to the indirect symbol
        old->setLink(pNew.link());
        pOverride = true;
        break;
      }
      case MIND: {       /* multiple indirect symbols.  */
        // it is OK if they both point to the same symbol
        if (old->link() == pNew.link()) {
          pOverride = false;
          break;
        }
      }
      /* Fall through */
      case MDEF: {       /* multiple definition error.  */
        Resolver::m_Mesg = "multiple definition of `" + pNew.name() + "'.";
        result = Resolver::Abort;
        break;
      }
      case REFC: {       /* Mark indirect symbol referenced and then CYCLE.  */
        if (0 == old->link()) {
          m_Mesg = "indirect symbol `"+old->name()+"' point to a inexistent symbol.";
          result = Resolver::Abort;
          break;
        }

        old = old->link();
        cycle = true;
        break;
      }
    } // end of the big switch (action)
  } while(cycle);
  return result;
}


