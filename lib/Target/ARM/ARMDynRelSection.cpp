//===- ARMDynRelSection.cpp ------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/LD/LDSection.h>
#include "ARMDynRelSection.h"

using namespace mcld;

//==========================
// ARMDynRelSection


ARMDynRelSection::ARMDynRelSection(LDSection& pSection,
                                   llvm::MCSectionData& pSectionData,
                                   const unsigned int pEntrySize)
  : m_pSection(&pSection),
    m_pSectionData(&pSectionData),
    m_EntryBytes(pEntrySize){
}

ARMDynRelSection::~ARMDynRelSection()
{
}

void ARMDynRelSection::reserveEntry(RelocationFactory& pRelFactory,
                                    int pNum)
{
  m_pSectionData->getFragmentList().push_back(pRelFactory.produceEmptyEntry());
  // update section size
  m_pSection->setSize(m_pSection->size() + m_EntryBytes);
}

Relocation* ARMDynRelSection::getEntry(const ResolveInfo& pSymbol,
                                       bool isForGOT,
                                       bool& pExist)
{
  // first time visit this function, set m_pEmpty to Fragments.begin()
  if(m_SymRelMap.empty()) {
      assert( !m_pSectionData->getFragmentList().empty() &&
             "DynRelSection contains no entries.");
    m_pEmpty = m_pSectionData->getFragmentList().begin();
  }

  // if this relocation is used to relocate GOT (.got or .got.plt),
  // check if we've gotton an entry for this symbol before. If yes,
  // retunrn the found entry in map.
  // Otherwise, this relocation is used to relocate general section
  // (data or text section), return an empty entry directly.
  Relocation* result;

  if(isForGOT) {
    // get or create entry in m_SymRelMap
    Relocation *&Entry = m_SymRelMap[&pSymbol];
    pExist = 1;

    if(!Entry) {
      pExist = 0;
      assert(m_pEmpty != m_pSectionData->end() &&
             "No empty relocation entry for the incoming symbol.");
      Entry = llvm::cast<Relocation>(&(*m_pEmpty));
      ++m_pEmpty;
    }
    result = Entry;
  }
  else {
    pExist = 0;
    result = llvm::cast<Relocation>(&(*m_pEmpty));
    ++m_pEmpty;
  }
  return result;
}
