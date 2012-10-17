//===- RelocationData.h ---------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_RELOCATIONDATA_H
#define MCLD_RELOCATIONDATA_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/Support/DataTypes.h>

#include <mcld/Config/Config.h>
#include <mcld/Fragment/Fragment.h>
#include <mcld/Support/Allocators.h>
#include <mcld/Support/GCFactoryListTraits.h>


namespace mcld
{

class LDSection;

/** \class RelocationData
 *  \brief RelocationData is the special SectionData to store Relocation fragments.
 *  Since Relocations are created by GCFactory, we use GCFactoryListTraits for the
 *  FragmentList here to avoid iplist to delete Relocations.
 */
class RelocationData
{
private:
  RelocationData();
  explicit RelocationData(const LDSection &pSection);

  RelocationData(const RelocationData &);            // DO NOT IMPLEMENT
  RelocationData& operator=(const RelocationData &); // DO NOT IMPLEMENT

public:
  typedef llvm::iplist<Fragment,
                       GCFactoryListTraits<Fragment> > FragmentListType;

  typedef FragmentListType::reference reference;
  typedef FragmentListType::const_reference const_reference;

  typedef FragmentListType::iterator iterator;
  typedef FragmentListType::const_iterator const_iterator;

  typedef FragmentListType::reverse_iterator reverse_iterator;
  typedef FragmentListType::const_reverse_iterator const_reverse_iterator;

public:
  static RelocationData* Create(const LDSection& pSection);

  static RelocationData* Create();

  static void Destroy(RelocationData*& pSection);

  const LDSection &getSection() const { return *m_pSection; }

  unsigned int getAlignment() const { return m_Alignment; }
  void setAlignment(unsigned int pValue) { m_Alignment = pValue; }

  FragmentListType &getFragmentList() { return m_Fragments; }
  const FragmentListType &getFragmentList() const { return m_Fragments; }

  size_t size() const { return m_Fragments.size(); }

  bool empty() const { return m_Fragments.empty(); }

  reference              front ()       { return m_Fragments.front();  }
  const_reference        front () const { return m_Fragments.front();  }
  reference              back  ()       { return m_Fragments.back();   }
  const_reference        back  () const { return m_Fragments.back();   }

  const_iterator         begin () const { return m_Fragments.begin();  }
  iterator               begin ()       { return m_Fragments.begin();  }
  const_iterator         end   () const { return m_Fragments.end();    }
  iterator               end   ()       { return m_Fragments.end();    }
  const_reverse_iterator rbegin() const { return m_Fragments.rbegin(); }
  reverse_iterator       rbegin()       { return m_Fragments.rbegin(); }
  const_reverse_iterator rend  () const { return m_Fragments.rend();   }
  reverse_iterator       rend  ()       { return m_Fragments.rend();   }

private:
  FragmentListType m_Fragments;
  const LDSection* m_pSection;
  unsigned int m_Alignment;

};

} // namespace of mcld

#endif
