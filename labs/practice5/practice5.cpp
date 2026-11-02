/**
 * PA2 Lab 5 - CTime
 * 
 * Countdown / time value class with arithmetic and comparison operators.
 */

#ifndef __PROGTEST__
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
using namespace std;

struct CTimeTester;
#endif /* __PROGTEST__ */

class CTime
{
private:
    int m_Hour;
    int m_Minute;
    int m_Second;

public:
    CTime ( void )
        : m_Hour ( 0 ), m_Minute ( 0 ), m_Second ( 0 )
    {
    }

    CTime ( int hour, int minute )
        : m_Hour ( hour ), m_Minute ( minute ), m_Second ( 0 )
    {
        if ( ! isValid ( m_Hour, m_Minute, m_Second ) )
            throw invalid_argument ( "invalid time" );
    }

    CTime ( int hour, int minute, int second )
        : m_Hour ( hour ), m_Minute ( minute ), m_Second ( second )
    {
        if ( ! isValid ( m_Hour, m_Minute, m_Second ) )
            throw invalid_argument ( "invalid time" );
    }

    CTime operator + ( int sec ) const
    {
        CTime res = *this;
        res += sec;
        return res;
    }

    CTime & operator += ( int sec )
    {
        long long total = toSeconds () + static_cast<long long> ( sec );
        fromSeconds ( total );
        return *this;
    }

    CTime operator - ( int sec ) const
    {
        CTime res = *this;
        res -= sec;
        return res;
    }

    CTime & operator -= ( int sec )
    {
        long long total = toSeconds () - static_cast<long long> ( sec );
        fromSeconds ( total );
        return *this;
    }

    int operator - ( const CTime & other ) const
    {
        int a = toSeconds ();
        int b = other.toSeconds ();
        int diff = a - b;
        if ( diff < 0 )
            diff = -diff;
        if ( diff > 86400 - diff )
            diff = 86400 - diff;
        return diff;
    }

    CTime & operator ++ ( void )
    {
        *this += 1;
        return *this;
    }

    CTime operator ++ ( int )
    {
        CTime old = *this;
        ++ ( *this );
        return old;
    }

    CTime & operator -- ( void )
    {
        *this -= 1;
        return *this;
    }

    CTime operator -- ( int )
    {
        CTime old = *this;
        -- ( *this );
        return old;
    }

    bool operator < ( const CTime & other ) const
    {
        return toSeconds () < other.toSeconds ();
    }

    bool operator <= ( const CTime & other ) const
    {
        return ! ( other < *this );
    }

    bool operator > ( const CTime & other ) const
    {
        return other < *this;
    }

    bool operator >= ( const CTime & other ) const
    {
        return ! ( *this < other );
    }

    bool operator == ( const CTime & other ) const
    {
        return m_Hour == other.m_Hour
            && m_Minute == other.m_Minute
            && m_Second == other.m_Second;
    }

    bool operator != ( const CTime & other ) const
    {
        return ! ( *this == other );
    }

    friend ostream & operator << ( ostream & os, const CTime & x )
    {
        char oldFill = os.fill ();

        os << setw ( 2 ) << setfill ( ' ' ) << x.m_Hour << ":"
           << setw ( 2 ) << setfill ( '0' ) << x.m_Minute << ":"
           << setw ( 2 ) << setfill ( '0' ) << x.m_Second;

        os.fill ( oldFill );
        return os;
    }

    friend struct ::CTimeTester;
private:
    static bool isValid ( int h, int m, int s )
    {
        return ( h >= 0 && h <= 23 )
            && ( m >= 0 && m <= 59 )
            && ( s >= 0 && s <= 59 );
    }

    long long toSeconds ( void ) const
    {
        return static_cast<long long> ( m_Hour ) * 3600
             + static_cast<long long> ( m_Minute ) * 60
             + m_Second;
    }

    void fromSeconds ( long long total )
    {
        total %= 86400;
        if ( total < 0 )
            total += 86400;

        m_Hour = static_cast<int> ( total / 3600 );
        total %= 3600;
        m_Minute = static_cast<int> ( total / 60 );
        m_Second = static_cast<int> ( total % 60 );
    }
};

CTime operator + ( int sec, const CTime & x )
{
    return x + sec;
}

#ifndef __PROGTEST__
struct CTimeTester
{
    static bool test ( const CTime & time, int hour, int minute, int second )
    {
        return time.m_Hour == hour 
            && time.m_Minute == minute
            && time.m_Second == second;
    }
};

#include <cassert>
#include <sstream>

int main ()
{
    CTime a { 12, 30 };
    assert( CTimeTester::test( a, 12, 30, 0 ) );

    CTime b { 13, 30 };
    assert( CTimeTester::test( b, 13, 30, 0 ) );

    assert( b - a == 3600 );

    assert( CTimeTester::test( a + 60, 12, 31, 0 ) );
    assert( CTimeTester::test( a - 60, 12, 29, 0 ) );

    assert( a < b );
    assert( a <= b );
    assert( a != b );
    assert( !( a > b ) );
    assert( !( a >= b ) );
    assert( !( a == b ) );

    while ( ++a != b );
    assert( a == b );

    std::ostringstream output;
    assert( static_cast<bool>( output << a ) );
    assert( output.str() == "13:30:00" );

    assert( a++ == b++ );
    assert( a == b );

    assert( --a == --b );
    assert( a == b );

    assert( a-- == b-- );
    assert( a == b );

    return 0;
}
#endif /* __PROGTEST__ */
