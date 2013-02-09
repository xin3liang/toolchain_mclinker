//===- X86PLT.cpp ---------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "X86GOTPLT.h"
#include "X86PLT.h"

#include <llvm/Support/ELF.h>
#include <llvm/Support/Casting.h>

#include <mcld/LD/LDSection.h>
#include <mcld/LinkerConfig.h>
#include <mcld/Support/MsgHandling.h>

using namespace mcld;

//===----------------------------------------------------------------------===//
// PLT entry data
//===----------------------------------------------------------------------===//
X86_32DynPLT0::X86_32DynPLT0(SectionData& pParent)
  : PLT::Entry<sizeof(x86_32_dyn_plt0)>(pParent)
{
}

X86_32DynPLT1::X86_32DynPLT1(SectionData& pParent)
  : PLT::Entry<sizeof(x86_32_dyn_plt1)>(pParent)
{
}

X86_32ExecPLT0::X86_32ExecPLT0(SectionData& pParent)
  : PLT::Entry<sizeof(x86_32_exec_plt0)>(pParent)
{
}

X86_32ExecPLT1::X86_32ExecPLT1(SectionData& pParent)
  : PLT::Entry<sizeof(x86_32_exec_plt1)>(pParent)
{
}

X86_64PLT0::X86_64PLT0(SectionData& pParent)
  : PLT::Entry<sizeof(x86_64_plt0)>(pParent)
{
}

X86_64PLT1::X86_64PLT1(SectionData& pParent)
  : PLT::Entry<sizeof(x86_64_plt1)>(pParent)
{
}

//===----------------------------------------------------------------------===//
// X86PLT
//===----------------------------------------------------------------------===//
X86PLT::X86PLT(LDSection& pSection,
	       const LinkerConfig& pConfig,
	       int got_size)
  : PLT(pSection),
    m_Config(pConfig)
{
  assert(LinkerConfig::DynObj == m_Config.codeGenType() ||
         LinkerConfig::Exec   == m_Config.codeGenType() ||
         LinkerConfig::Binary == m_Config.codeGenType());

  if (got_size == 32) {
    if (LinkerConfig::DynObj == m_Config.codeGenType()) {
      m_PLT0 = x86_32_dyn_plt0;
      m_PLT1 = x86_32_dyn_plt1;
      m_PLT0Size = sizeof (x86_32_dyn_plt0);
      m_PLT1Size = sizeof (x86_32_dyn_plt1);
      // create PLT0
      new X86_32DynPLT0(*m_SectionData);
    }
    else {
      m_PLT0 = x86_32_exec_plt0;
      m_PLT1 = x86_32_exec_plt1;
      m_PLT0Size = sizeof (x86_32_exec_plt0);
      m_PLT1Size = sizeof (x86_32_exec_plt1);
      // create PLT0
      new X86_32ExecPLT0(*m_SectionData);
    }
  }
  else {
    assert(got_size == 64);
    m_PLT0 = x86_64_plt0;
    m_PLT1 = x86_64_plt1;
    m_PLT0Size = sizeof (x86_64_plt0);
    m_PLT1Size = sizeof (x86_64_plt1);
    // create PLT0
    new X86_64PLT0(*m_SectionData);
    m_Last = m_SectionData->begin();
  }
  m_Last = m_SectionData->begin();
}

X86PLT::~X86PLT()
{
}

void X86PLT::finalizeSectionSize()
{
  uint64_t size = 0;
  // plt0 size
  size = getPLT0()->size();

  // get first plt1 entry
  X86PLT::iterator it = begin();
  ++it;
  if (end() != it) {
    // plt1 size
    PLTEntryBase* plt1 = &(llvm::cast<PLTEntryBase>(*it));
    size += (m_SectionData->size() - 1) * plt1->size();
  }
  m_Section.setSize(size);

  uint32_t offset = 0;
  SectionData::iterator frag, fragEnd = m_SectionData->end();
  for (frag = m_SectionData->begin(); frag != fragEnd; ++frag) {
    frag->setOffset(offset);
    offset += frag->size();
  }
}

bool X86PLT::hasPLT1() const
{
  return (m_SectionData->size() > 1);
}

void X86PLT::reserveEntry(size_t pNum)
{
  PLTEntryBase* plt1_entry = NULL;

  for (size_t i = 0; i < pNum; ++i) {

    if (LinkerConfig::DynObj == m_Config.codeGenType())
      plt1_entry = new X86_32DynPLT1(*m_SectionData);
    else
      plt1_entry = new X86_32ExecPLT1(*m_SectionData);

    if (NULL == plt1_entry)
      fatal(diag::fail_allocate_memory_plt);
  }
}

PLTEntryBase* X86PLT::consume()
{
  // This will skip PLT0.
  ++m_Last;
  assert(m_Last != m_SectionData->end() &&
         "The number of PLT Entries and ResolveInfo doesn't match");
  return llvm::cast<PLTEntryBase>(&(*m_Last));
}

PLTEntryBase* X86PLT::getPLT0() const
{
  iterator first = m_SectionData->getFragmentList().begin();

  assert(first != m_SectionData->getFragmentList().end() &&
         "FragmentList is empty, getPLT0 failed!");

  PLTEntryBase* plt0 = &(llvm::cast<PLTEntryBase>(*first));

  return plt0;
}

//===----------------------------------------------------------------------===//
// X86_32PLT
//===----------------------------------------------------------------------===//
X86_32PLT::X86_32PLT(LDSection& pSection,
		     X86_32GOTPLT& pGOTPLT,
		     const LinkerConfig& pConfig)
  : X86PLT(pSection, pConfig, 32),
    m_GOTPLT(pGOTPLT) {
}

// FIXME: It only works on little endian machine.
void X86_32PLT::applyPLT0()
{
  PLTEntryBase* plt0 = getPLT0();

  unsigned char* data = 0;
  data = static_cast<unsigned char*>(malloc(plt0->size()));

  if (!data)
    fatal(diag::fail_allocate_memory_plt);

  memcpy(data, m_PLT0, plt0->size());

  if (m_PLT0 == x86_32_exec_plt0) {
    uint32_t *offset = reinterpret_cast<uint32_t*>(data + 2);
    *offset = m_GOTPLT.addr() + 4;
    offset = reinterpret_cast<uint32_t*>(data + 8);
    *offset = m_GOTPLT.addr() + 8;
  }

  plt0->setValue(data);
}

// FIXME: It only works on little endian machine.
void X86_32PLT::applyPLT1()
{
  assert(m_Section.addr() && ".plt base address is NULL!");

  X86PLT::iterator it = m_SectionData->begin();
  X86PLT::iterator ie = m_SectionData->end();
  assert(it != ie && "FragmentList is empty, applyPLT1 failed!");

  uint64_t GOTEntrySize = X86_32GOTEntry::EntrySize;

  // Skip GOT0
  uint64_t GOTEntryOffset = GOTEntrySize * X86GOTPLT0Num;
  if (LinkerConfig::Exec == m_Config.codeGenType())
    GOTEntryOffset += m_GOTPLT.addr();

  //skip PLT0
  uint64_t PLTEntryOffset = m_PLT0Size;
  ++it;

  PLTEntryBase* plt1 = 0;

  uint64_t PLTRelOffset = 0;

  while (it != ie) {
    plt1 = &(llvm::cast<PLTEntryBase>(*it));
    unsigned char *data;
    data = static_cast<unsigned char*>(malloc(plt1->size()));

    if (!data)
      fatal(diag::fail_allocate_memory_plt);

    memcpy(data, m_PLT1, plt1->size());

    uint32_t* offset;

    offset = reinterpret_cast<uint32_t*>(data + 2);
    *offset = GOTEntryOffset;
    GOTEntryOffset += GOTEntrySize;

    offset = reinterpret_cast<uint32_t*>(data + 7);
    *offset = PLTRelOffset;
    PLTRelOffset += sizeof (llvm::ELF::Elf32_Rel);

    offset = reinterpret_cast<uint32_t*>(data + 12);
    *offset = -(PLTEntryOffset + 12 + 4);
    PLTEntryOffset += m_PLT1Size;

    plt1->setValue(data);
    ++it;
  }
}

//===----------------------------------------------------------------------===//
// X86_64PLT
//===----------------------------------------------------------------------===//
X86_64PLT::X86_64PLT(LDSection& pSection,
		     X86_64GOTPLT& pGOTPLT,
		     const LinkerConfig& pConfig)
  : X86PLT(pSection, pConfig, 64),
    m_GOTPLT(pGOTPLT) {
}

// FIXME: It only works on little endian machine.
void X86_64PLT::applyPLT0()
{
  PLTEntryBase* plt0 = getPLT0();

  unsigned char* data = 0;
  data = static_cast<unsigned char*>(malloc(plt0->size()));

  if (!data)
    fatal(diag::fail_allocate_memory_plt);

  memcpy(data, m_PLT0, plt0->size());

  assert(0 && "Update PLT0");

  plt0->setValue(data);
}

// FIXME: It only works on little endian machine.
void X86_64PLT::applyPLT1()
{
  assert(m_Section.addr() && ".plt base address is NULL!");

  X86PLT::iterator it = m_SectionData->begin();
  X86PLT::iterator ie = m_SectionData->end();
  assert(it != ie && "FragmentList is empty, applyPLT1 failed!");

  uint64_t GOTEntrySize = X86_64GOTEntry::EntrySize;

  // Skip GOT0
  uint64_t GOTEntryOffset = GOTEntrySize * X86GOTPLT0Num;
  if (LinkerConfig::Exec == m_Config.codeGenType())
    GOTEntryOffset += m_GOTPLT.addr();

  //skip PLT0
  uint64_t PLTEntryOffset = m_PLT0Size;
  ++it;

  PLTEntryBase* plt1 = 0;

  uint64_t PLTRelOffset = 0;

  while (it != ie) {
    plt1 = &(llvm::cast<PLTEntryBase>(*it));
    unsigned char *data;
    data = static_cast<unsigned char*>(malloc(plt1->size()));

    if (!data)
      fatal(diag::fail_allocate_memory_plt);

    memcpy(data, m_PLT1, plt1->size());

    assert(0 && "Update PLT1");

    plt1->setValue(data);
    ++it;
  }
}

