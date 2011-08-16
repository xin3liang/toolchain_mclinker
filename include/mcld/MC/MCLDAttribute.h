/*****************************************************************************
 *   The MCLinker Project, Copyright (C), 2011 -                             *
 *   Embedded and Web Computing Lab, National Taiwan University              *
 *   MediaTek, Inc.                                                          *
 *                                                                           *
 *   Luba Tang <lubatang@mediatek.com>                                       *
 ****************************************************************************/
#ifndef MCLD_ATTRIBUTE_H
#define MCLD_ATTRIBUTE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif
#include <vector>

namespace mcld
{
class AttributeFactory;

/** \class Attribute
 *  \brief The base class of attributes. Providing the raw operations of an
 *  attributes
 */
class Attribute
{
public:
  Attribute()
  : m_WholeArchive(false),
    m_AsNeeded(false),
    m_Static(false)
  { }

  Attribute(const Attribute& pBase)
  : m_WholeArchive(pBase.m_WholeArchive),
    m_AsNeeded(pBase.m_AsNeeded),
    m_Static(pBase.m_Static)
  { }

  ~Attribute()
  { }

  // ----- observers  ----- //
  bool isWholeArchive() const
  { return m_WholeArchive; }

  bool isAsNeeded() const
  { return m_AsNeeded; }

  bool isStatic() const
  { return m_Static; }

  bool isDynamic() const
  { return !m_Static; }

  // -----  modifiers  ----- //
  void setWholeArchive()
  { m_WholeArchive = true; }

  void unsetWholeArchive()
  { m_WholeArchive = false; }

  void setAsNeeded()
  { m_AsNeeded = true; }

  void unsetAsNeeded()
  { m_AsNeeded = false; }

  void setStatic()
  { m_Static = true; }

  void setDynamic()
  { m_Static = false; }

private:
  bool m_WholeArchive;
  bool m_AsNeeded;
  bool m_Static;
};

/** \class AttributeProxy
 *  \brief AttributeProxys is the attribute of input options.
 */
class AttributeProxy
{
private:
  friend class AttributeFactory;

  explicit AttributeProxy(AttributeFactory& pParent, Attribute& pBase);
  ~AttributeProxy();

public:
  // ----- observers  ----- //
  bool isWholeArchive() const;
  bool isAsNeeded() const;
  bool isStatic() const;
  bool isDynamic() const;
  Attribute* attr()
  { return m_pBase; }
  const Attribute* attr() const
  { return m_pBase; }

  // -----  modifiers  ----- //
  void setWholeArchive();
  void unsetWholeArchive();
  void setAsNeeded();
  void unsetAsNeeded();
  void setStatic();
  void setDynamic();

private:
  AttributeProxy* clone() const;

  void change(Attribute* pBase)
  { m_pBase = pBase; }

private:
  AttributeFactory &m_Parent;
  Attribute *m_pBase;
};

// -----  comparisons  ----- //
inline bool operator== (const Attribute& pLHS, const Attribute& pRHS)
{
  return ((pLHS.isWholeArchive() == pRHS.isWholeArchive()) &&
    (pLHS.isAsNeeded() == pRHS.isAsNeeded()) && 
    (pLHS.isStatic() == pRHS.isStatic()));
}

inline bool operator!= (const Attribute& pLHS, const Attribute& pRHS)
{
  return !(pLHS == pRHS);
}

} // namespace of mcld

#endif

