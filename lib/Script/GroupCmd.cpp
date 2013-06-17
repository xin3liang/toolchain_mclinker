//===- GroupCmd.cpp -------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/Script/GroupCmd.h>
#include <mcld/Script/ScriptInput.h>
#include <mcld/Script/InputToken.h>
#include <mcld/MC/InputBuilder.h>
#include <mcld/MC/Attribute.h>
#include <mcld/Support/Path.h>
#include <mcld/Support/raw_ostream.h>
#include <mcld/Support/MsgHandling.h>
#include <mcld/InputTree.h>
#include <mcld/LD/GroupReader.h>
#include <llvm/Support/Casting.h>
#include <cassert>

using namespace mcld;

//===----------------------------------------------------------------------===//
// GroupCmd
//===----------------------------------------------------------------------===//
GroupCmd::GroupCmd(ScriptInput& pScriptInput,
                   InputTree& pInputTree,
                   InputBuilder& pBuilder,
                   GroupReader& pGroupReader,
                   const LinkerConfig& pConfig,
                   const LinkerScript& pScript)
  : ScriptCommand(ScriptCommand::Group),
    m_ScriptInput(pScriptInput),
    m_InputTree(pInputTree),
    m_Builder(pBuilder),
    m_GroupReader(pGroupReader),
    m_Config(pConfig),
    m_Script(pScript)
{
}

GroupCmd::~GroupCmd()
{
}

void GroupCmd::dump() const
{
  mcld::outs() << "GROUP ( ";
  bool prev = false, cur = false;
  for (ScriptInput::const_iterator it = m_ScriptInput.begin(),
    ie = m_ScriptInput.end(); it != ie; ++it) {
    assert((*it)->kind() == StrToken::Input);
    InputToken* input = llvm::cast<InputToken>(*it);
    cur = input->asNeeded();
    if (!prev && cur)
      mcld::outs() << "AS_NEEDED ( ";
    else if (prev && !cur)
      mcld::outs() << " )";

    if (input->type() == InputToken::NameSpec)
      mcld::outs() << "-l";
    mcld::outs() << input->name() << " ";

    prev = cur;
  }

  if (!m_ScriptInput.empty() && prev)
    mcld::outs() << " )";

  mcld::outs() << " )\n";
}

void GroupCmd::activate()
{
  // construct the Group tree
  m_Builder.setCurrentTree(m_InputTree);
  // --start-group
  m_Builder.enterGroup();
  InputTree::iterator group = m_Builder.getCurrentNode();

  for (ScriptInput::const_iterator it = m_ScriptInput.begin(),
    ie = m_ScriptInput.end(); it != ie; ++it) {

    assert((*it)->kind() == StrToken::Input);
    InputToken* token = llvm::cast<InputToken>(*it);
    if (token->asNeeded())
      m_Builder.getAttributes().setAsNeeded();
    else
      m_Builder.getAttributes().unsetAsNeeded();

    switch (token->type()) {
    case InputToken::File: {
      sys::fs::Path path(token->name());
      m_Builder.createNode<InputTree::Positional>(
        path.filename().native(), path, Input::Unknown);
      break;
    }
    case InputToken::NameSpec: {
      const sys::fs::Path* path = NULL;
      // find out the real path of the namespec.
      if (m_Builder.getConstraint().isSharedSystem()) {
        // In the system with shared object support, we can find both archive
        // and shared object.
        if (m_Builder.getAttributes().isStatic()) {
          // with --static, we must search an archive.
          path = m_Script.directories().find(token->name(), Input::Archive);
        } else {
          // otherwise, with --Bdynamic, we can find either an archive or a
          // shared object.
          path = m_Script.directories().find(token->name(), Input::DynObj);
        }
      } else {
        // In the system without shared object support, only look for an archive
        path = m_Script.directories().find(token->name(), Input::Archive);
      }

      if (NULL == path)
        fatal(diag::err_cannot_find_namespec) << token->name();

      m_Builder.createNode<InputTree::Positional>(
        token->name(), *path, Input::Unknown);
      break;
    }
    default:
      assert(0 && "Invalid script token in GROUP!");
      break;
    } // end of switch

    Input* input = *m_Builder.getCurrentNode();
    assert(input != NULL);
    if (!m_Builder.setMemory(*input, FileHandle::ReadOnly))
      error(diag::err_cannot_open_input) << input->name() << input->path();
    m_Builder.setContext(*input);
  }

  // --end-group
  m_Builder.exitGroup();

  // read the group
  m_GroupReader.readGroup(group, m_InputTree.end(), m_Builder, m_Config);

}

