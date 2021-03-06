//===- OptimizationOptions.cpp --------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <lite/OptimizationOptions.h>
#include <mcld/LinkerConfig.h>
#include <mcld/Support/CommandLine.h>
#include <mcld/Support/MsgHandling.h>

namespace {

// Not supported yet
bool ArgGCSections;

llvm::cl::opt<bool, true> ArgGCSectionsFlag("gc-sections",
  llvm::cl::ZeroOrMore,
  llvm::cl::location(ArgGCSections),
  llvm::cl::desc("Enable garbage collection of unused input sections."),
  llvm::cl::init(false));

llvm::cl::opt<bool, true, llvm::cl::FalseParser> ArgNoGCSectionsFlag("no-gc-sections",
  llvm::cl::ZeroOrMore,
  llvm::cl::location(ArgGCSections),
  llvm::cl::desc("disable garbage collection of unused input sections."),
  llvm::cl::init(false));

bool ArgGenUnwindInfo;

llvm::cl::opt<bool, true, llvm::cl::FalseParser>
ArgNoGenUnwindInfoFlag("no-ld-generated-unwind-info",
                       llvm::cl::ZeroOrMore,
                       llvm::cl::location(ArgGenUnwindInfo),
                       llvm::cl::desc("Don't create unwind info for linker"
                                      " generated sections to save size"),
                       llvm::cl::init(false),
                       llvm::cl::ValueDisallowed);
llvm::cl::opt<bool, true>
ArgGenUnwindInfoFlag("ld-generated-unwind-info",
                     llvm::cl::ZeroOrMore,
                     llvm::cl::location(ArgGenUnwindInfo),
                     llvm::cl::desc("Request creation of unwind info for linker"
                                    " generated code sections like PLT."),
                     llvm::cl::init(true),
                     llvm::cl::ValueDisallowed);

llvm::cl::opt<mcld::OptimizationOptions::ICF> ArgICF("icf",
  llvm::cl::ZeroOrMore,
  llvm::cl::desc("Identical Code Folding"),
  llvm::cl::init(mcld::OptimizationOptions::ICF_None),
  llvm::cl::values(
    clEnumValN(mcld::OptimizationOptions::ICF_None, "none",
      "do not perform cold folding"),
    clEnumValN(mcld::OptimizationOptions::ICF_All, "all",
      "always preform cold folding"),
    clEnumValN(mcld::OptimizationOptions::ICF_Safe, "safe",
      "Folds those whose pointers are definitely not taken."),
    clEnumValEnd));

llvm::cl::list<std::string> ArgPlugin("plugin",
  llvm::cl::desc("Load a plugin library."),
  llvm::cl::value_desc("plugin"));

llvm::cl::list<std::string> ArgPluginOpt("plugin-opt",
  llvm::cl::desc("Pass an option to the plugin."),
  llvm::cl::value_desc("option"));

} // anonymous namespace

using namespace mcld;

//===----------------------------------------------------------------------===//
// OptimizationOptions
//===----------------------------------------------------------------------===//
OptimizationOptions::OptimizationOptions()
  : m_GCSections(ArgGCSections),
    m_GenUnwindInfo(ArgGenUnwindInfo),
    m_ICF(ArgICF),
    m_Plugin(ArgPlugin),
    m_PluginOpt(ArgPluginOpt) {
}

bool OptimizationOptions::parse(LinkerConfig& pConfig)
{
  // set --gc-sections
  if (m_GCSections)
    pConfig.options().setGCSections();

  // set --ld-generated-unwind-info (or not)
  pConfig.options().setGenUnwindInfo(m_GenUnwindInfo);

  // set --icf [mode]
  switch (m_ICF) {
    case ICF_None:
      break;
    case ICF_All:
    case ICF_Safe:
    default:
      warning(mcld::diag::warn_unsupported_option) << m_ICF.ArgStr;
      break;
  }

  return true;
}
