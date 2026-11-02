#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <compare>
#endif /* __PROGTEST__ */

//=================================================================================================
// unique slot index for storing format string in stream
static int getDateFmtIdx ( void )
{
  static int idx = std::ios_base::xalloc ();
  return idx;
}

// tracks if callback was already registered on this stream
static int getDateCbRegistered ( void )
{
  static int idx = std::ios_base::xalloc ();
  return idx;
}

// cleanup callback - frees format string when stream dies, deep copies when stream is copied
static void dateFmtCallback ( std::ios_base::event ev, std::ios_base & ios, int )
{
  int idx = getDateFmtIdx ();
  if ( ev == std::ios_base::erase_event )
  {
    delete static_cast<std::string *> ( ios . pword ( idx ) );
    ios . pword ( idx ) = nullptr;
  }
  else if ( ev == std::ios_base::copyfmt_event )
  {
    std::string * old = static_cast<std::string *> ( ios . pword ( idx ) );
    if ( old != nullptr )
      ios . pword ( idx ) = new std::string ( *old );
  }
}

// helper class to carry the format string from date_format() into << / >>
class CDateFormat
{
  public:
    std::string m_Fmt;
    CDateFormat ( const char * f ) : m_Fmt ( f ) {}
};

CDateFormat date_format ( const char * fmt )
{
  return CDateFormat ( fmt );
}

// saves the format into stream's pword, registers callback once
static void setStreamFmt ( std::ios_base & ios, const std::string & fmt )
{
  int fmtIdx = getDateFmtIdx ();
  int cbIdx  = getDateCbRegistered ();

  delete static_cast<std::string *> ( ios . pword ( fmtIdx ) );
  ios . pword ( fmtIdx ) = new std::string ( fmt );

  if ( ! ios . iword ( cbIdx ) )
  {
    ios . register_callback ( dateFmtCallback, 0 );
    ios . iword ( cbIdx ) = 1;
  }
}

// oss << date_format(...) stores the format into oss
std::ostream & operator << ( std::ostream & os, const CDateFormat & df )
{
  setStreamFmt ( os, df . m_Fmt );
  return os;
}

// iss >> date_format(...) stores the format into iss
std::istream & operator >> ( std::istream & is, const CDateFormat & df )
{
  setStreamFmt ( is, df . m_Fmt );
  return is;
}
//=================================================================================================
class CDate
{
  public:
    // thrown when date is invalid
    class InvalidDateException : public std::invalid_argument
    {
      public:
        InvalidDateException ( void )
          : std::invalid_argument ( "invalid date" )
        {
        }
    };

    CDate ( int y, int m, int d )
      : m_Year ( y ), m_Month ( m ), m_Day ( d )
    {
      if ( ! isValidDate ( y, m, d ) )
        throw InvalidDateException ();
    }

    // date + days: convert to day number, add, convert back
    CDate operator + ( int days ) const
    {
      int serial = daysFromCivil ( m_Year, m_Month, m_Day );
      serial += days;

      int y, m, d;
      civilFromDays ( serial, y, m, d );
      return CDate ( y, m, d );
    }

    // date - days: just add negative
    CDate operator - ( int days ) const
    {
      return *this + ( -days );
    }

    // date - date: difference in days
    int operator - ( const CDate & other ) const
    {
      return daysFromCivil ( m_Year, m_Month, m_Day )
           - daysFromCivil ( other . m_Year, other . m_Month, other . m_Day );
    }

    // ++date
    CDate & operator ++ ( void )
    {
      *this = *this + 1;
      return *this;
    }

    // date++
    CDate operator ++ ( int )
    {
      CDate tmp = *this;
      ++ ( *this );
      return tmp;
    }

    // --date
    CDate & operator -- ( void )
    {
      *this = *this - 1;
      return *this;
    }

    // date--
    CDate operator -- ( int )
    {
      CDate tmp = *this;
      -- ( *this );
      return tmp;
    }

    bool operator == ( const CDate & other ) const
    {
      return m_Year == other . m_Year
          && m_Month == other . m_Month
          && m_Day == other . m_Day;
    }

    bool operator != ( const CDate & other ) const
    {
      return ! ( *this == other );
    }

    bool operator < ( const CDate & other ) const
    {
      return ( *this - other ) < 0;
    }

    bool operator <= ( const CDate & other ) const
    {
      return ( *this - other ) <= 0;
    }

    bool operator > ( const CDate & other ) const
    {
      return other < *this;
    }

    bool operator >= ( const CDate & other ) const
    {
      return other <= *this;
    }

    // output - build into tmp first so setw() pads the whole date string
    friend std::ostream & operator << ( std::ostream & os, const CDate & x )
    {
      // get format from stream or use default ISO
      std::string fmt = "%Y-%m-%d";
      std::string * stored = static_cast<std::string *> ( os . pword ( getDateFmtIdx () ) );
      if ( stored != nullptr )
        fmt = *stored;

      std::ostringstream tmp;
      tmp << std::setfill ( '0' );

      for ( size_t i = 0; i < fmt . size (); i++ )
      {
        if ( fmt[i] == '%' && i + 1 < fmt . size () )
        {
          i++;
          if ( fmt[i] == 'Y' )
            tmp << std::setw ( 4 ) << x . m_Year;
          else if ( fmt[i] == 'm' )
            tmp << std::setw ( 2 ) << x . m_Month;
          else if ( fmt[i] == 'd' )
            tmp << std::setw ( 2 ) << x . m_Day;
          else
            tmp << fmt[i];  // %% or %x -> literal char
        }
        else
        {
          tmp << fmt[i];
        }
      }

      os << tmp . str ();
      return os;
    }

    // input - reads char by char according to format, only modifies x if everything is ok
    friend std::istream & operator >> ( std::istream & is, CDate & x )
    {
      std::string fmt = "%Y-%m-%d";
      std::string * stored = static_cast<std::string *> ( is . pword ( getDateFmtIdx () ) );
      if ( stored != nullptr )
        fmt = *stored;

      // format must have exactly one %Y, %m, %d otherwise its ambiguous
      int cntY = 0, cntM = 0, cntD = 0;
      for ( size_t i = 0; i < fmt . size (); i++ )
      {
        if ( fmt[i] == '%' && i + 1 < fmt . size () )
        {
          i++;
          if ( fmt[i] == 'Y' )       cntY++;
          else if ( fmt[i] == 'm' )   cntM++;
          else if ( fmt[i] == 'd' )   cntD++;
        }
      }

      if ( cntY != 1 || cntM != 1 || cntD != 1 )
      {
        is . setstate ( std::ios::failbit );
        return is;
      }

      is >> std::ws;

      int y = 0, m = 0, d = 0;
      bool ok = true;

      // parse input char by char matching the format
      for ( size_t i = 0; i < fmt . size () && ok; i++ )
      {
        if ( fmt[i] == '%' && i + 1 < fmt . size () )
        {
          i++;
          if ( fmt[i] == 'Y' )
            ok = readDigits ( is, 4, y );
          else if ( fmt[i] == 'm' )
            ok = readDigits ( is, 2, m );
          else if ( fmt[i] == 'd' )
            ok = readDigits ( is, 2, d );
          else
          {
            char c;
            if ( ! is . get ( c ) || c != fmt[i] )
              ok = false;
          }
        }
        else
        {
          // literal char in format must match input
          char c;
          if ( ! is . get ( c ) || c != fmt[i] )
            ok = false;
        }
      }

      if ( ! ok || ! isValidDate ( y, m, d ) )
      {
        is . setstate ( std::ios::failbit );
        return is;
      }

      x = CDate ( y, m, d );
      return is;
    }

  private:
    int m_Year;
    int m_Month;
    int m_Day;

    // divisible by 400 -> leap, by 100 -> not leap, by 4 -> leap
    static bool isLeapYear ( int year )
    {
      if ( year % 400 == 0 )
        return true;
      if ( year % 100 == 0 )
        return false;
      return year % 4 == 0;
    }

    static int daysInMonth ( int year, int month )
    {
      static const int monthDays[12] =
      {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
      };

      int res = monthDays[month - 1];
      if ( month == 2 && isLeapYear ( year ) )
        res = 29;

      return res;
    }

    static bool isValidDate ( int year, int month, int day )
    {
      if ( month < 1 || month > 12 )
        return false;

      if ( day < 1 || day > daysInMonth ( year, month ) )
        return false;

      return true;
    }

    // reads exactly 'count' digit chars from stream, builds the int
    static bool readDigits ( std::istream & is, int count, int & result )
    {
      result = 0;
      for ( int j = 0; j < count; j++ )
      {
        char c;
        if ( ! is . get ( c ) || ! std::isdigit ( static_cast<unsigned char> ( c ) ) )
          return false;
        result = result * 10 + ( c - '0' );
      }
      return true;
    }

    // converts y/m/d to a single day number (makes +/- easy)
    static int daysFromCivil ( int year, int month, int day )
    {
      year -= month <= 2;
      const int era = ( year >= 0 ? year : year - 399 ) / 400;
      const unsigned yoe = static_cast<unsigned> ( year - era * 400 );
      const unsigned doy = ( 153 * ( month + ( month > 2 ? -3 : 9 ) ) + 2 ) / 5 + day - 1;
      const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
      return era * 146097 + static_cast<int> ( doe ) - 719468;
    }

    // inverse of above - day number back to y/m/d
    static void civilFromDays ( int z, int & year, int & month, int & day )
    {
      z += 719468;
      const int era = ( z >= 0 ? z : z - 146096 ) / 146097;
      const unsigned doe = static_cast<unsigned> ( z - era * 146097 );
      const unsigned yoe = ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365;
      year = static_cast<int> ( yoe ) + era * 400;
      const unsigned doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
      const unsigned mp = ( 5 * doy + 2 ) / 153;
      day = static_cast<int> ( doy - ( 153 * mp + 2 ) / 5 + 1 );
      month = static_cast<int> ( mp + ( mp < 10 ? 3 : -9 ) );
      year += month <= 2;
    }
};

#ifndef __PROGTEST__
int main ()
{
  std::ostringstream oss;
  std::istringstream iss;

  CDate a ( 2000, 1, 2 );
  CDate b ( 2010, 2, 3 );
  CDate c ( 2004, 2, 10 );
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2000-01-02" );
  oss . str ("");
  oss << b;
  assert ( oss . str () == "2010-02-03" );
  oss . str ("");
  oss << c;
  assert ( oss . str () == "2004-02-10" );
  a = a + 1500;
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2004-02-10" );
  b = b - 2000;
  oss . str ("");
  oss << b;
  assert ( oss . str () == "2004-08-13" );
  assert ( b - a == 185 );
  assert ( ( b == a ) == false );
  assert ( ( b != a ) == true );
  assert ( ( b <= a ) == false );
  assert ( ( b < a ) == false );
  assert ( ( b >= a ) == true );
  assert ( ( b > a ) == true );
  assert ( ( c == a ) == true );
  assert ( ( c != a ) == false );
  assert ( ( c <= a ) == true );
  assert ( ( c < a ) == false );
  assert ( ( c >= a ) == true );
  assert ( ( c > a ) == false );
  a = ++c;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-11 2004-02-11" );
  a = --c;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-10 2004-02-10" );
  a = c++;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-10 2004-02-11" );
  a = c--;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-11 2004-02-10" );
  iss . clear ();
  iss . str ( "2015-09-03" );
  assert ( ( iss >> a ) );
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2015-09-03" );
  a = a + 70;
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2015-11-12" );
  oss . str ("");
  oss << std::setw ( 20 ) << a;
  assert ( oss . str () == "          2015-11-12" );

  CDate d ( 2000, 1, 1 );
  try
  {
    CDate e ( 2000, 32, 1 );
    assert ( "No exception thrown!" == nullptr );
  }
  catch ( ... )
  {
  }
  iss . clear ();
  iss . str ( "2000-12-33" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-01-01" );
  iss . clear ();
  iss . str ( "2000-11-31" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-01-01" );
  iss . clear ();
  iss . str ( "2000-02-29" );
  assert ( ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-02-29" );
  iss . clear ();
  iss . str ( "2001-02-29" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-02-29" );

  //-----------------------------------------------------------------------------
  // bonus test examples
  //-----------------------------------------------------------------------------
  CDate f ( 2000, 5, 12 );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  oss . str ("");
  oss << date_format ( "%Y/%m/%d" ) << f;
  assert ( oss . str () == "2000/05/12" );
  oss . str ("");
  oss << date_format ( "%d.%m.%Y" ) << f;
  assert ( oss . str () == "12.05.2000" );
  oss . str ("");
  oss << date_format ( "%m/%d/%Y" ) << f;
  assert ( oss . str () == "05/12/2000" );
  oss . str ("");
  oss << date_format ( "%Y%m%d" ) << f;
  assert ( oss . str () == "20000512" );
  oss . str ("");
  oss << date_format ( "hello kitty" ) << f;
  assert ( oss . str () == "hello kitty" );
  oss . str ("");
  oss << date_format ( "%d%d%d%d%d%d%m%m%m%Y%Y%Y%%%%%%%%%%" ) << f;
  assert ( oss . str () == "121212121212050505200020002000%%%%%" );
  oss . str ("");
  oss << date_format ( "%Y-%m-%d" ) << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-01-1" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-1-01" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-001-01" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-01-02" );
  assert ( ( iss >> date_format ( "%Y-%m-%d" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2001-01-02" );
  iss . clear ();
  iss . str ( "05.06.2003" );
  assert ( ( iss >> date_format ( "%d.%m.%Y" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2003-06-05" );
  iss . clear ();
  iss . str ( "07/08/2004" );
  assert ( ( iss >> date_format ( "%m/%d/%Y" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2004-07-08" );
  iss . clear ();
  iss . str ( "2002*03*04" );
  assert ( ( iss >> date_format ( "%Y*%m*%d" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2002-03-04" );
  iss . clear ();
  iss . str ( "C++09format10PA22006rulez" );
  assert ( ( iss >> date_format ( "C++%mformat%dPA2%Yrulez" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2006-09-10" );
  iss . clear ();
  iss . str ( "%12%13%2010%" );
  assert ( ( iss >> date_format ( "%%%m%%%d%%%Y%%" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2010-12-13" );

  CDate g ( 2000, 6, 8 );
  iss . clear ();
  iss . str ( "2001-11-33" );
  assert ( ! ( iss >> date_format ( "%Y-%m-%d" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "29.02.2003" );
  assert ( ! ( iss >> date_format ( "%d.%m.%Y" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "14/02/2004" );
  assert ( ! ( iss >> date_format ( "%m/%d/%Y" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "2002-03" );
  assert ( ! ( iss >> date_format ( "%Y-%m" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "hello kitty" );
  assert ( ! ( iss >> date_format ( "hello kitty" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "2005-07-12-07" );
  assert ( ! ( iss >> date_format ( "%Y-%m-%d-%m" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "20000101" );
  assert ( ( iss >> date_format ( "%Y%m%d" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-01-01" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
