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
  if (ResolveInfo::Weak == pInfo.binding())
    return Category::Weak;
  return Category::Global;
}

//===----------------------------------------------------------------------===//
// SymbolCategory
SymbolCategory::SymbolCategory()
{
  m_pFile   = new Category(Category::File);
  m_pLocal  = new Category(Category::Local);
  m_pCommon = new Category(Category::Common);
  m_pWeak   = new Category(Category::Weak);
  m_pGlobal = new Category(Category::Global);

  m_pFile->next   = m_pLocal;
  m_pLocal->next  = m_pCommon;
  m_pCommon->next = m_pWeak;
  m_pWeak->next   = m_pGlobal;

  m_pGlobal->prev = m_pWeak;
  m_pWeak->prev   = m_pCommon;
  m_pCommon->prev = m_pLocal;
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

  Category* current = m_pGlobal;

  // use non-stable bubble sort to arrange the order of symbols.
  while (NULL != current) {
    if (current->type == target) {
      current->end++;
      break;
    }
    else {
      std::swap(m_OutputSymbols[current->begin],
                m_OutputSymbols[current->end]);
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
    size_t rear = current->end - 1;
    if (pos != rear) {
      // if pos is not the rear element, swap pos and rear
      std::swap(m_OutputSymbols[pos], m_OutputSymbols[rear]);
      pos = rear;
    }
    current->end--;

    // downward
    do {
      current = current->next;
      if (current->isLast() || current->type == target) {
        current->begin--;
        break;
      }
      else {
        rear = current->end - 1;
        std::swap(m_OutputSymbols[pos], m_OutputSymbols[rear]);
        pos = rear;
        current->begin--;
        current->end--;
      }
    } while(NULL != current);

    return *this;
  }

  // The distance is negative. It means we should bubble sort upward.
  if (distance < 0) {
    if (pos != current->begin) {
      std::swap(m_OutputSymbols[pos], m_OutputSymbols[current->begin]);
      pos = current->begin;
    }
    current->begin--;

    // upward
    do {
      current = current->prev;
      if (current->isFirst() || current->type == target) {
        current->end++;
        break;
      }
      else {
        std::swap(m_OutputSymbols[current->begin], m_OutputSymbols[pos]);
        pos = current->begin;
        current->begin++;
        current->end++;
      }
    } while(NULL != current);

    return *this;
  }
  return *this;
}

size_t SymbolCategory::numOfSymbols() const
{
  return m_OutputSymbols.size();
}

size_t SymbolCategory::numOfLocals() const
{
  return (m_pFile->size() + m_pLocal->size());
}

size_t SymbolCategory::numOfCommons() const
{
  return m_pCommon->size();
}

size_t SymbolCategory::numOfRegulars() const
{
  return (m_pWeak->size() + m_pGlobal->size());
}

bool SymbolCategory::empty() const
{
  return (emptyLocals() &&
          emptyCommons() &&
          emptyRegulars());
}

bool SymbolCategory::emptyLocals() const
{
  return (m_pFile->empty() && m_pLocal->empty());
}

bool SymbolCategory::emptyCommons() const
{
  return m_pCommon->empty();
}

bool SymbolCategory::emptyRegulars() const
{
  return (m_pWeak->empty() && m_pGlobal->empty());
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

SymbolCategory::iterator SymbolCategory::localBegin()
{
  return m_OutputSymbols.begin();
}

SymbolCategory::iterator SymbolCategory::localEnd()
{
  iterator iter = m_OutputSymbols.begin();
  iter += m_pFile->size();
  iter += m_pLocal->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::localBegin() const
{
  return m_OutputSymbols.begin();
}

SymbolCategory::const_iterator SymbolCategory::localEnd() const
{
  const_iterator iter = m_OutputSymbols.begin();
  iter += m_pFile->size();
  iter += m_pLocal->size();
  return iter;
}

SymbolCategory::iterator SymbolCategory::commonBegin()
{
  return localEnd();
}

SymbolCategory::iterator SymbolCategory::commonEnd()
{
  iterator iter = localEnd();
  iter += m_pCommon->size();
  return iter;
}

SymbolCategory::const_iterator SymbolCategory::commonBegin() const
{
  return localEnd();
}

SymbolCategory::const_iterator SymbolCategory::commonEnd() const
{
  const_iterator iter = localEnd();
  iter += m_pCommon->size();
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

