//===- SymbolCategory.cpp -------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <mcld/MC/SymbolCategory.h>
#include <mcld/LD/LDSymbol.h>
#include <mcld/LD/ResolveInfo.h>
#include <algorithm>
#include <cassert>

using namespace mcld;

//===----------------------------------------------------------------------===//
// Category
SymbolCategory::Category::Type
SymbolCategory::Category::categorize(const ResolveInfo& pInfo)
{
  if (ResolveInfo::File == pInfo.type())
    return Category::File;
  if (ResolveInfo::Local == pInfo.binding())
    return Category::Local;
  if (ResolveInfo::Common == pInfo.desc())
    return Category::Common;
  if (ResolveInfo::Default   == pInfo.visibility() ||
      ResolveInfo::Protected == pInfo.visibility())
    return Category::Dynamic;
  return Category::Regular;
}

//===----------------------------------------------------------------------===//
// SymbolCategory
SymbolCategory::SymbolCategory()
{
  m_pFile    = new Category(Category::File);
  m_pLocal   = new Category(Category::Local);
  m_pTLS     = new Category(Category::TLS);
  m_pCommon  = new Category(Category::Common);
  m_pDynamic = new Category(Category::Dynamic);
  m_pRegular = new Category(Category::Regular);

  m_pFile->next    = m_pLocal;
  m_pLocal->next   = m_pTLS;
  m_pTLS->next     = m_pCommon;
  m_pCommon->next  = m_pDynamic;
  m_pDynamic->next = m_pRegular;

  m_pRegular->prev = m_pDynamic;
  m_pDynamic->prev = m_pCommon;
  m_pCommon->prev  = m_pTLS;
  m_pTLS->prev     = m_pLocal;
  m_pLocal->prev   = m_pFile;
}

SymbolCategory::~SymbolCategory()
{
  Category* current = m_pFile;
  while (NULL != current) {
    Category* tmp = current;
    current = current->next;
    delete tmp;
  }
}

SymbolCategory& SymbolCategory::add(LDSymbol& pSymbol)
{
  m_OutputSymbols.push_back(&pSymbol);

  assert(NULL != pSymbol.resolveInfo());
  Category::Type target = Category::categorize(*pSymbol.resolveInfo());

  Category* current = m_pRegular;

  // use non-stable bubble sort to arrange the order of symbols.
  while (NULL != current) {
    if (current->type == target) {
      current->end++;
      break;
    }
    else {
      if (!current->empty()) {
        std::swap(m_OutputSymbols[current->begin],
                  m_OutputSymbols[current->end]);
      }
      current->end++;
      current->begin++;
      current = current->prev;
    }
  }
  return *this;
}

SymbolCategory& SymbolCategory::forceLocal(LDSymbol& pSymbol)
{
  m_OutputSymbols.insert(localEnd(), &pSymbol);
  m_pLocal->end++;
  m_pTLS->begin++;
  m_pTLS->end++;
  m_pCommon->begin++;
  m_pCommon->end++;
  m_pDynamic->begin++;
  m_pDynamic->end++;
  m_pRegular->begin++;
  m_pRegular->end++;

  return *this;
}

SymbolCategory& SymbolCategory::arrange(LDSymbol& pSymbol, const ResolveInfo& pSourceInfo)
{
  assert(NULL != pSymbol.resolveInfo());
  Category::Type source = Category::categorize(pSourceInfo);
  Category::Type target = Category::categorize(*pSymbol.resolveInfo());

  int distance = target - source;
  if (0 == distance) {
    // in the same category, do not need to re-arrange
    return *this;
  }

  // source and target are not in the same category
  // find the category of source
  Category* current = m_pFile;
  while(NULL != current) {
    if (source == current->type)
      break;
    current = current->next;
  }

  assert(NULL != current);
  assert(!current->empty());

  // find the position of source
  size_t pos = current->begin;
  while (pos != current->end) {
    if (m_OutputSymbols[pos] == &pSymbol)
      break;
    ++pos;
  }

  assert(current->end != pos);

  // The distance is positive. It means we should bubble sort downward.
  if (distance > 0) {
    // downward
    size_t rear;
    do {
      if (current->type == target) {
        break;
      }
      else {
        assert(!current->isLast() && "target category is wrong.");
        rear = current->end - 1;
        std::swap(m_OutputSymbols[pos], m_OutputSymbols[rear]);
        pos = rear;
        current->next->begin--;
        current->end--;
      }
      current = current->next;
    } while(NULL != current);

    return *this;
  } // downward

  // The distance is negative. It means we should bubble sort upward.
  if (distance < 0) {

    // upward
    do {
      if (current->type == target) {
        break;
      }
      else {
        assert(!current->isFirst() && "target category is wrong.");
        std::swap(m_OutputSymbols[current->begin], m_OutputSymbols[pos]);
        pos = current->begin;
        current->begin++;
        current->prev->end++;
      }
      current = current->prev;
    } while(NULL != current);

    return *this;
  } // upward
  return *this;
}

SymbolCategory& SymbolCategory::changeCommonsToGlobal()
{
  // Change Common to Dynamic/Regular
  while (!emptyCommons()) {
    size_t pos = m_pCommon->end - 1;
    switch (Category::categorize(*(m_OutputSymbols[pos]->resolveInfo()))) {
    case Category::Dynamic:
      m_pCommon->end--;
      m_pDynamic->begin--;
      break;
    case Category::Regular:
      std::swap(m_OutputSymbols[pos], m_OutputSymbols[m_pDynamic->end - 1]);
      m_pCommon->end--;
      m_pDynamic->begin--;
      m_pDynamic->end--;
      m_pRegular->begin--;
      break;
    default:
      assert(0);
      break;
    }
  }
  return *this;
}

SymbolCategory& SymbolCategory::changeLocalToTLS(const LDSymbol& pSymbol)
{
  // find the position of pSymbol from local category
  size_t pos = m_pLocal->begin;
  while (pos != m_pLocal->end) {
    if (m_OutputSymbols[pos] == &pSymbol)
      break;
    ++pos;
  }

  // if symbol is not in Local, then do nothing
  if (m_pLocal->end == pos)
    return *this;

  // bubble sort downward to TLS
  std::swap(m_OutputSymbols[pos], m_OutputSymbols[m_pLocal->end - 1]);
  m_pLocal->end--;
  m_pTLS->begin--;
  return *this;
}

size_t SymbolCategory::numOfSymbols() const
{
  return m_OutputSymbols.size();
}

size_t SymbolCategory::numOfFiles() const
{
  return m_pFile->size();
}

size_t SymbolCategory::numOfLocals() const
{
  return m_pLocal->size();
}

size_t SymbolCategory::numOfTLSs() const
{
  return m_pTLS->size();
}

size_t SymbolCategory::numOfCommons() const
{
  return m_pCommon->size();
}

size_t SymbolCategory::numOfDynamics() const
{
  return m_pDynamic->size();
}

size_t SymbolCategory::numOfRegulars() const
{
  return m_pRegular->size();
}

bool SymbolCategory::empty() const
{
  return m_OutputSymbols.empty();
}

bool SymbolCategory::emptyFiles() const
{
  return m_pFile->empty();
}

bool SymbolCategory::emptyLocals() const
{
  return m_pLocal->empty();
}

bool SymbolCategory::emptyTLSs() const
{
  return m_pTLS->empty();
}

bool SymbolCategory::emptyCommons() const
{
  return m_pCommon->empty();
}

bool SymbolCategory::emptyDynamics() const
{
  return m_pDynamic->empty();
}

bool SymbolCategory::emptyRegulars() const
{
  return m_pRegular->empty();
}

SymbolCategory::iterator SymbolCategory::begin()
{
  return m_OutputSymbols.begin();
}

SymbolCategory::iterator SymbolCategory::end()
{
  return m_OutputSymbols.end();
}

SymbolCategory::const_iterator SymbolCategory::begin() const
{
  return m_OutputSymbols.begin();
}

SymbolCategory::const_iterator SymbolCategory::end() const
{
  return m_OutputSymbols.end();
}

SymbolCategory::iterator SymbolCategory::fileBegin()
{
  return m_OutputSymbols.begin();
}

SymbolCategory::iterator SymbolCategory::fileEnd()
{
  iterator iter = fileBegin();
  iter += m_pFile->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::fileBegin() const
{
  return m_OutputSymbols.begin();
}

SymbolCategory::const_iterator SymbolCategory::fileEnd() const
{
  const_iterator iter = fileBegin();
  iter += m_pFile->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::localBegin()
{
  return fileEnd();
}

SymbolCategory::iterator SymbolCategory::localEnd()
{
  iterator iter = localBegin();
  iter += m_pLocal->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::localBegin() const
{
  return fileEnd();
}

SymbolCategory::const_iterator SymbolCategory::localEnd() const
{
  const_iterator iter = localBegin();
  iter += m_pLocal->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::tlsBegin()
{
  return localEnd();
}

SymbolCategory::iterator SymbolCategory::tlsEnd()
{
  iterator iter = tlsBegin();
  iter += m_pTLS->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::tlsBegin() const
{
  return localEnd();
}

SymbolCategory::const_iterator SymbolCategory::tlsEnd() const
{
  const_iterator iter = tlsBegin();
  iter += m_pTLS->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::commonBegin()
{
  return tlsEnd();
}

SymbolCategory::iterator SymbolCategory::commonEnd()
{
  iterator iter = commonBegin();
  iter += m_pCommon->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::commonBegin() const
{
  return tlsEnd();
}

SymbolCategory::const_iterator SymbolCategory::commonEnd() const
{
  const_iterator iter = commonBegin();
  iter += m_pCommon->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::dynamicBegin()
{
  return commonEnd();
}

SymbolCategory::iterator SymbolCategory::dynamicEnd()
{
  iterator iter = dynamicBegin();
  iter += m_pDynamic->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::dynamicBegin() const
{
  return commonEnd();
}

SymbolCategory::const_iterator SymbolCategory::dynamicEnd() const
{
  const_iterator iter = dynamicBegin();
  iter += m_pDynamic->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::regularBegin()
{
  return commonEnd();
}

SymbolCategory::iterator SymbolCategory::regularEnd()
{
  return m_OutputSymbols.end();
}

SymbolCategory::const_iterator SymbolCategory::regularBegin() const
{
  return commonEnd();
}

SymbolCategory::const_iterator SymbolCategory::regularEnd() const
{
  return m_OutputSymbols.end();
}

