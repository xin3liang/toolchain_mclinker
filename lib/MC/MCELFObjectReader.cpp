/*****************************************************************************
 *   The MCLinker Project, Copyright (C), 2011 -                             *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   Duo <pinronglu@gmail.com>                                               *
 ****************************************************************************/
#include <mcld/MC/MCELFObjectReader.h>

using namespace mcld;

//==========================
// MCELFObjectReader
MCELFObjectReader::MCELFObjectReader(const MCELFObjectTargetReader *pTargetReader)
  : m_pTargetReader(pTargetReader) {
}

MCELFObjectReader::~MCELFObjectReader()
{
}

