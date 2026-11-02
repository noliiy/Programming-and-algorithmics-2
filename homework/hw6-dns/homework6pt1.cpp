#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
#include <compare>
#include "ipaddress.h"
using namespace std::literals;
#endif /* __PROGTEST__ */

class CRecA
{
  public:
    // Base record class.
    // I keep common behavior here (name, virtual type/print/clone/compare),
    // so CZone can store one polymorphic container and not care about concrete type.
    CRecA ( const std::string & name, const CIPv4 & addr )
      : m_Name ( name ), m_IPv4 ( addr )
    {
    }
    virtual ~CRecA ( void ) = default;

    std::string name ( void ) const
    {
      return m_Name;
    }

    virtual std::string type ( void ) const
    {
      return "A";
    }

    friend std::ostream & operator << ( std::ostream & os, const CRecA & x )
    {
      // Printing is virtual through print(), so this friend works for all derived records.
      x . print ( os );
      return os;
    }

    virtual std::shared_ptr<CRecA> clone ( void ) const
    {
      // Needed for polymorphic copy when records are added to CZone.
      return std::make_shared<CRecA> ( *this );
    }

    virtual bool sameRecord ( const CRecA & other ) const
    {
      // Compare only if both objects are the same runtime record type.
      const CRecA * x = dynamic_cast<const CRecA *> ( &other );
      if ( ! x )
        return false;
      return m_Name == x -> m_Name && m_IPv4 == x -> m_IPv4;
    }

    virtual void print ( std::ostream & os ) const
    {
      os << m_Name << " A " << m_IPv4;
    }

  protected:
    std::string m_Name;
    CIPv4 m_IPv4;
};

class CRecAAAA
  : public CRecA
{
  public:
    // CRecA constructor still needs an IPv4 value, but AAAA does not use it,
    // so I pass a default CIPv4() and keep the real value in m_IPv6.
    CRecAAAA ( const std::string & name, const CIPv6 & addr )
      : CRecA ( name, CIPv4 () ), m_IPv6 ( addr )
    {
    }
    std::string type ( void ) const override
    {
      return "AAAA";
    }
    friend std::ostream & operator << ( std::ostream & os, const CRecAAAA & x )
    {
      x . print ( os );
      return os;
    }
    std::shared_ptr<CRecA> clone ( void ) const override
    {
      return std::make_shared<CRecAAAA> ( *this );
    }
    bool sameRecord ( const CRecA & other ) const override
    {
      const CRecAAAA * x = dynamic_cast<const CRecAAAA *> ( &other );
      if ( ! x )
        return false;
      return m_Name == x -> m_Name && m_IPv6 == x -> m_IPv6;
    }
    void print ( std::ostream & os ) const override
    {
      os << m_Name << " AAAA " << m_IPv6;
    }
  private:
    CIPv6 m_IPv6;
};

class CRecMX
  : public CRecA
{
  public:
    // Same idea as CRecAAAA: base gets default IPv4, MX stores its own fields.
    CRecMX ( const std::string & name, const std::string & server, int priority )
      : CRecA ( name, CIPv4 () ), m_Server ( server ), m_Priority ( priority )
    {
    }
    std::string type ( void ) const override
    {
      return "MX";
    }
    friend std::ostream & operator << ( std::ostream & os, const CRecMX & x )
    {
      x . print ( os );
      return os;
    }
    std::shared_ptr<CRecA> clone ( void ) const override
    {
      return std::make_shared<CRecMX> ( *this );
    }
    bool sameRecord ( const CRecA & other ) const override
    {
      const CRecMX * x = dynamic_cast<const CRecMX *> ( &other );
      if ( ! x )
        return false;
      return m_Name == x -> m_Name
             && m_Server == x -> m_Server
             && m_Priority == x -> m_Priority;
    }
    void print ( std::ostream & os ) const override
    {
      os << m_Name << " MX " << m_Priority << " " << m_Server;
    }
  private:
    std::string m_Server;
    int m_Priority;
};

class CZone
{
  public:
    class CSearchResult
    {
      public:
        CSearchResult ( void ) = default;
        explicit CSearchResult ( const std::vector<std::shared_ptr<CRecA>> & data )
          : m_Data ( data )
        {
        }
        size_t size ( void ) const
        {
          return m_Data . size ();
        }
        const CRecA & operator [] ( size_t idx ) const
        {
          // Assignment explicitly requires std::out_of_range for bad index.
          if ( idx >= m_Data . size () )
            throw std::out_of_range ( "search result index out of range" );
          return *m_Data[idx];
        }
        friend std::ostream & operator << ( std::ostream & os, const CSearchResult & x )
        {
          // Search output is one record per line in the same order as records were added.
          for ( size_t i = 0; i < x . m_Data . size (); i ++ )
            os << *x . m_Data[i] << "\n";
          return os;
        }
      private:
        std::vector<std::shared_ptr<CRecA>> m_Data;
    };

    explicit CZone ( const std::string & zoneName )
      : m_ZoneName ( zoneName )
    {
    }

    bool add ( const CRecA & rec )
    {
      // 100% identical records are not allowed in one zone.
      for ( const auto & r : m_Records )
        if ( r -> sameRecord ( rec ) )
          return false;

      // We clone so zone owns its own copy and is independent from caller objects.
      std::shared_ptr<CRecA> recCopy = rec . clone ();
      m_Records . push_back ( recCopy );
      // Separate index for faster search by hostname.
      m_ByName[recCopy -> name ()] . push_back ( recCopy );
      return true;
    }

    bool del ( const CRecA & rec )
    {
      // First remove from main ordered list (used for zone printing).
      for ( auto it = m_Records . begin (); it != m_Records . end (); ++ it )
      {
        if ( ( *it ) -> sameRecord ( rec ) )
        {
          std::string key = ( *it ) -> name ();
          std::shared_ptr<CRecA> toDelete = *it;
          m_Records . erase ( it );

          // Then remove from the name index to keep both structures consistent.
          auto mapIt = m_ByName . find ( key );
          if ( mapIt != m_ByName . end () )
          {
            auto & vec = mapIt -> second;
            // Remove the exact shared object from the name index as well.
            for ( auto bit = vec . begin (); bit != vec . end (); ++ bit )
            {
              if ( *bit == toDelete )
              {
                vec . erase ( bit );
                break;
              }
            }
            if ( vec . empty () )
              m_ByName . erase ( mapIt );
          }
          return true;
        }
      }
      return false;
    }
    CSearchResult search ( const std::string & hostName ) const
    {
      // O(1) expected lookup by name thanks to unordered_map index.
      auto it = m_ByName . find ( hostName );
      if ( it == m_ByName . end () )
        return CSearchResult ();
      // Keep insertion order inside each hostname bucket.
      return CSearchResult ( it -> second );
    }
    friend std::ostream & operator << ( std::ostream & os, const CZone & x )
    {
      os << x . m_ZoneName << "\n";
      // Exact tree-like formatting required by tests: "+-" for middle, "\-" for last.
      for ( size_t i = 0; i < x . m_Records . size (); i ++ )
      {
        if ( i + 1 == x . m_Records . size () )
          os << " \\- ";
        else
          os << " +- ";
        os << *x . m_Records[i] << "\n";
      }
      return os;
    }
  private:
    std::string m_ZoneName;
    std::vector<std::shared_ptr<CRecA>> m_Records;
    std::unordered_map<std::string, std::vector<std::shared_ptr<CRecA>>> m_ByName;
};

#ifndef __PROGTEST__
int main ()
{
  std::ostringstream oss;

  CZone z0 ( "fit.cvut.cz" );
  assert ( z0 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:1:2:3:4" ) ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay.fit.cvut.cz.", 0 ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay2.fit.cvut.cz.", 10 ) ) == true );
  oss . str ( "" );
  oss << z0;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " \\- courses MX 10 relay2.fit.cvut.cz.\n" );
  assert ( z0 . search ( "progtest" ) . size () == 3 );
  oss . str ( "" );
  oss << z0 . search ( "progtest" );
  assert ( oss . str () == 
    "progtest A 147.32.232.142\n"
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "progtest AAAA 2001:718:2:2902:1:2:3:4\n" );
  assert ( z0 . del ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.122" ) ) ) == true );
  oss . str ( "" );
  oss << z0;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " +- courses MX 10 relay2.fit.cvut.cz.\n"
    " \\- courses A 147.32.232.122\n" );
  assert ( z0 . search ( "courses" ) . size () == 5 );
  oss . str ( "" );
  oss << z0 . search ( "courses" );
  assert ( oss . str () == 
    "courses A 147.32.232.158\n"
    "courses A 147.32.232.159\n"
    "courses MX 0 relay.fit.cvut.cz.\n"
    "courses MX 10 relay2.fit.cvut.cz.\n"
    "courses A 147.32.232.122\n" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 0 ];
  assert ( oss . str () == "courses A 147.32.232.158" );
  assert ( z0 . search ( "courses" ) [ 0 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 0 ] . type () == "A" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 1 ];
  assert ( oss . str () == "courses A 147.32.232.159" );
  assert ( z0 . search ( "courses" ) [ 1 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 1 ] . type () == "A" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 2 ];
  assert ( oss . str () == "courses MX 0 relay.fit.cvut.cz." );
  assert ( z0 . search ( "courses" ) [ 2 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 2 ] . type () == "MX" );
  try
  {
    oss . str ( "" );
    oss << z0 . search ( "courses" ) [ 10 ];
    assert ( "No exception thrown!" == nullptr );
  }
  catch ( const std::out_of_range & e )
  {
  }
  catch ( ... )
  {
    assert ( "Invalid exception thrown!" == nullptr );
  }
  dynamic_cast<const CRecAAAA &> ( z0 . search ( "progtest" ) [ 1 ] );
  CZone z1 ( "fit2.cvut.cz" );
  z1 . add ( z0 . search ( "progtest" ) [ 2 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 0 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 1 ] );
  z1 . add ( z0 . search ( "courses" ) [ 2 ] );
  oss . str ( "" );
  oss << z1;
  assert ( oss . str () == 
    "fit2.cvut.cz\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- courses MX 0 relay.fit.cvut.cz.\n" );
  dynamic_cast<const CRecA &> ( z1 . search ( "progtest" ) [ 1 ] );

  CZone z2 ( "fit.cvut.cz" );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z2 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.144" ) ) ) == true );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == true );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.140" ) ) ) == false );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 20 ) ) == true );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == false );
  oss . str ( "" );
  oss << z2;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- progtest A 147.32.232.144\n"
    " +- progtest MX 10 relay.fit.cvut.cz.\n"
    " \\- progtest MX 20 relay.fit.cvut.cz.\n" );
  assert ( z2 . search ( "progtest" ) . size () == 4 );
  oss . str ( "" );
  oss << z2 . search ( "progtest" );
  assert ( oss . str () == 
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "progtest A 147.32.232.144\n"
    "progtest MX 10 relay.fit.cvut.cz.\n"
    "progtest MX 20 relay.fit.cvut.cz.\n" );
  assert ( z2 . search ( "courses" ) . size () == 0 );
  oss . str ( "" );
  oss << z2 . search ( "courses" );
  assert ( oss . str () == "" );
  try
  {
    dynamic_cast<const CRecMX &> ( z2 . search ( "progtest" ) [ 0 ] );
    assert ( "Invalid type" == nullptr );
  }
  catch ( const std::bad_cast & e )
  {
  }

  CZone z4 ( "fit.cvut.cz" );
  assert ( z4 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  CZone z5 ( z4 );
  assert ( z4 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z4 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:1:2:3:4" ) ) ) == true );
  assert ( z5 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  oss . str ( "" );
  oss << z4;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- progtest AAAA 2001:718:2:2902:1:2:3:4\n" );
  oss . str ( "" );
  oss << z5;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " \\- courses A 147.32.232.159\n" );
  z5 = z4;
  assert ( z4 . add ( CRecMX ( "courses", "relay.fit.cvut.cz.", 0 ) ) == true );
  assert ( z4 . add ( CRecMX ( "courses", "relay2.fit.cvut.cz.", 10 ) ) == true );
  oss . str ( "" );
  oss << z4;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " \\- courses MX 10 relay2.fit.cvut.cz.\n" );
  oss . str ( "" );
  oss << z5;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- progtest AAAA 2001:718:2:2902:1:2:3:4\n" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
