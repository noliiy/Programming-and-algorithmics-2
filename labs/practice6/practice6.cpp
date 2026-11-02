/**
 * PA2 Lab 6 - CLinkedSet
 * 
 * Singly linked string set with deep copy, rule of three and manual memory management.
 */

#ifndef __PROGTEST__
#include <cstring>
using namespace std;

class CLinkedSetTester;
#endif /* __PROGTEST__ */

#include <cstring>

class CLinkedSet
{
private:
    struct CNode
    {
        CNode * m_Next;
        char * m_Str;

        const char * Value () const;
    };

    CNode * m_Begin;

    static char * dupStr ( const char * s );
    static void   freeList ( CNode * h );
    static CNode * cloneChain ( const CNode * h );

public:
    CLinkedSet ();
    CLinkedSet ( const CLinkedSet & other );
    CLinkedSet & operator = ( const CLinkedSet & other );
    ~CLinkedSet ();

    bool Insert ( const char * value );

    bool Remove ( const char * value );

    bool Empty () const;

    size_t Size () const;

    bool Contains ( const char * value ) const;

    friend class ::CLinkedSetTester;
};

char * CLinkedSet::dupStr ( const char * s )
{
    size_t n = strlen ( s );
    char * p = new char[n + 1];
    memcpy ( p, s, n + 1 );
    return p;
}

void CLinkedSet::freeList ( CNode * h )
{
    while ( h )
    {
        CNode * t = h->m_Next;
        delete [] h->m_Str;
        delete h;
        h = t;
    }
}

CLinkedSet::CNode * CLinkedSet::cloneChain ( const CNode * h )
{
    CNode * out = nullptr;
    CNode ** tail = &out;
    while ( h )
    {
        CNode * n = new CNode;
        n->m_Next = nullptr;
        n->m_Str = dupStr ( h->m_Str );
        *tail = n;
        tail = & ( n->m_Next );
        h = h->m_Next;
    }
    return out;
}

CLinkedSet::CLinkedSet ()
  : m_Begin ( nullptr )
{
}

CLinkedSet::CLinkedSet ( const CLinkedSet & other )
  : m_Begin ( cloneChain ( other.m_Begin ) )
{
}

CLinkedSet & CLinkedSet::operator = ( const CLinkedSet & other )
{
    if ( this == &other )
        return *this;
    freeList ( m_Begin );
    m_Begin = cloneChain ( other.m_Begin );
    return *this;
}

CLinkedSet::~CLinkedSet ()
{
    freeList ( m_Begin );
}

bool CLinkedSet::Insert ( const char * value )
{
    CNode ** pp = &m_Begin;
    while ( *pp )
    {
        int c = strcmp ( value, ( *pp )->Value () );
        if ( c == 0 )
            return false;
        if ( c < 0 )
            break;
        pp = & ( ( *pp )->m_Next );
    }
    CNode * n = new CNode;
    n->m_Str = dupStr ( value );
    n->m_Next = *pp;
    *pp = n;
    return true;
}

bool CLinkedSet::Remove ( const char * value )
{
    CNode ** pp = &m_Begin;
    while ( *pp )
    {
        int c = strcmp ( value, ( *pp )->Value () );
        if ( c == 0 )
        {
            CNode * d = *pp;
            *pp = d->m_Next;
            delete [] d->m_Str;
            delete d;
            return true;
        }
        if ( c < 0 )
            return false;
        pp = & ( ( *pp )->m_Next );
    }
    return false;
}

bool CLinkedSet::Empty () const
{
    return m_Begin == nullptr;
}

size_t CLinkedSet::Size () const
{
    size_t n = 0;
    for ( CNode * p = m_Begin; p; p = p->m_Next )
        n ++;
    return n;
}

bool CLinkedSet::Contains ( const char * value ) const
{
    for ( CNode * p = m_Begin; p; p = p->m_Next )
    {
        int c = strcmp ( value, p->Value () );
        if ( c == 0 )
            return true;
        if ( c < 0 )
            return false;
    }
    return false;
}

const char * CLinkedSet::CNode::Value () const
{
    return m_Str;
}

#ifndef __PROGTEST__
#include <cassert>

struct CLinkedSetTester
{
public:
    static void test0()
    {
        CLinkedSet x0;
        assert( x0 . Insert( "Jerabek Michal" ) );
        assert( x0 . Insert( "Pavlik Ales" ) );
        assert( x0 . Insert( "Dusek Zikmund" ) );
        assert( x0 . Size() == 3 );
        assert( x0 . Contains( "Dusek Zikmund" ) );
        assert( !x0 . Contains( "Pavlik Jan" ) );
        assert( x0 . Remove( "Jerabek Michal" ) );
        assert( !x0 . Remove( "Pavlik Jan" ) );
        assert( !x0 . Empty() );
    }

    static void test1()
    {
        CLinkedSet x0;
        assert( x0 . Insert( "Jerabek Michal" ) );
        assert( x0 . Insert( "Pavlik Ales" ) );
        assert( x0 . Insert( "Dusek Zikmund" ) );
        assert( x0 . Size() == 3 );
        assert( x0 . Contains( "Dusek Zikmund" ) );
        assert( !x0 . Contains( "Pavlik Jan" ) );
        assert( x0 . Remove( "Jerabek Michal" ) );
        assert( !x0 . Remove( "Pavlik Jan" ) );
        CLinkedSet x1 ( x0 );
        assert( x0 . Insert( "Vodickova Saskie" ) );
        assert( x1 . Insert( "Kadlecova Kvetslava" ) );
        assert( x0 . Size() == 3 );
        assert( x1 . Size() == 3 );
        assert( x0 . Contains( "Vodickova Saskie" ) );
        assert( !x1 . Contains( "Vodickova Saskie" ) );
        assert( !x0 . Contains( "Kadlecova Kvetslava" ) );
        assert( x1 . Contains( "Kadlecova Kvetslava" ) );
    }

    static void test2()
    {
        CLinkedSet x0;
        CLinkedSet x1;
        assert( x0 . Insert( "Jerabek Michal" ) );
        assert( x0 . Insert( "Pavlik Ales" ) );
        assert( x0 . Insert( "Dusek Zikmund" ) );
        assert( x0 . Size() == 3 );
        assert( x0 . Contains( "Dusek Zikmund" ) );
        assert( !x0 . Contains( "Pavlik Jan" ) );
        assert( x0 . Remove( "Jerabek Michal" ) );
        assert( !x0 . Remove( "Pavlik Jan" ) );
        x1 = x0;
        assert( x0 . Insert( "Vodickova Saskie" ) );
        assert( x1 . Insert( "Kadlecova Kvetslava" ) );
        assert( x0 . Size() == 3 );
        assert( x1 . Size() == 3 );
        assert( x0 . Contains( "Vodickova Saskie" ) );
        assert( !x1 . Contains( "Vodickova Saskie" ) );
        assert( !x0 . Contains( "Kadlecova Kvetslava" ) );
        assert( x1 . Contains( "Kadlecova Kvetslava" ) );
    }

};

int main ()
{
    CLinkedSetTester::test0();
    CLinkedSetTester::test1();
    CLinkedSetTester::test2();
    return 0;
}
#endif /* __PROGTEST__ */
