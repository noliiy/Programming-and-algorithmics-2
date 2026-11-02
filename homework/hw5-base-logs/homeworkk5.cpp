#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <deque>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <limits>
#include <stdint.h>
#endif /* __PROGTEST__ */


namespace
{
  const int INF_DIST = 1000000000;

  /* Standard leap year check from the assignment rules. */
  bool isLeap ( int y )
  {
    if ( y % 400 == 0 ) return true;
    if ( y % 100 == 0 ) return false;
    return y % 4 == 0;
  }

  /* Days in month helper, with February fix for leap years. */
  int dim ( int y, int m )
  {
    static const int d[13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
    if ( m < 1 || m > 12 ) return 0;
    if ( m == 2 && isLeap ( y ) ) return 29;
    return d[m];
  }

  /* Throws runtime_error if date/time is outside valid Gregorian range. */
  void checkDate ( int y, int m, int d, int h, int mi )
  {
    if ( y < 1 ) throw std::runtime_error ( "bad date" );
    if ( m < 1 || m > 12 ) throw std::runtime_error ( "bad date" );
    if ( h < 0 || h > 23 || mi < 0 || mi > 59 ) throw std::runtime_error ( "bad time" );
    int days = dim ( y, m );
    if ( d < 1 || d > days ) throw std::runtime_error ( "bad day" );
  }

  int toMins ( int y, int m, int d, int h, int mi )
  {
    checkDate ( y, m, d, h, mi );
    int days = 0;
    if ( y >= 1970 )
    {
      for ( int yy = 1970; yy < y; yy++ )
        days += isLeap ( yy ) ? 366 : 365;
    }
    else
    {
      for ( int yy = y; yy < 1970; yy++ )
        days -= isLeap ( yy ) ? 366 : 365;
    }
    for ( int mm = 1; mm < m; mm++ )
      days += dim ( y, mm );
    days += ( d - 1 );
    return days * 24 * 60 + h * 60 + mi;
  }

  uint16_t read16LE ( const unsigned char * p )
  {
    return static_cast<uint16_t> ( p[0] | ( p[1] << 8 ) );
  }

  uint16_t read16BE ( const unsigned char * p )
  {
    return static_cast<uint16_t> ( ( p[0] << 8 ) | p[1] );
  }

  uint32_t read32LE ( const unsigned char * p )
  {
    return static_cast<uint32_t> ( p[0] | ( p[1] << 8u ) | ( p[2] << 16u ) | ( p[3] << 24u ) );
  }

  uint32_t read32BE ( const unsigned char * p )
  {
    return static_cast<uint32_t> ( ( static_cast<uint32_t> ( p[0] ) << 24u ) |
                                   ( static_cast<uint32_t> ( p[1] ) << 16u ) |
                                   ( static_cast<uint32_t> ( p[2] ) << 8u ) |
                                   static_cast<uint32_t> ( p[3] ) );
  }

  void decodeDT ( uint32_t x, int & y, int & m, int & d, int & h, int & mi )
  {
    mi = static_cast<int> ( x & 0x3Fu );
    h  = static_cast<int> ( ( x >> 6 ) & 0x1Fu );
    d  = static_cast<int> ( ( x >> 11 ) & 0x1Fu );
    m  = static_cast<int> ( ( x >> 16 ) & 0xFu );
    y  = static_cast<int> ( ( x >> 20 ) & 0xFFFu );
    checkDate ( y, m, d, h, mi );
  }

  /* Small trim helper for header and text line parsing. */
  void trim ( std::string & s )
  {
    while ( !s.empty () && std::isspace ( static_cast<unsigned char> ( s.front () ) ) ) s.erase ( s.begin () );
    while ( !s.empty () && std::isspace ( static_cast<unsigned char> ( s.back () ) ) ) s.pop_back ();
  }

  bool startsWith ( const std::vector<unsigned char> & buf, size_t pos, const char * txt )
  {
    for ( size_t i = 0; txt[i]; i++ )
    {
      if ( pos + i >= buf.size () ) return false;
      if ( buf[pos + i] != static_cast<unsigned char> ( txt[i] ) ) return false;
    }
    return true;
  }

  std::string readLineBuf ( const std::vector<unsigned char> & buf, size_t & pos )
  {
    std::string out;
    while ( pos < buf.size () && buf[pos] != '\n' )
      out.push_back ( static_cast<char> ( buf[pos++] ) );
    if ( pos < buf.size () && buf[pos] == '\n' ) pos++;
    return out;
  }

  void parseTxtEntry ( const std::string & line, int & t, std::string & name )
  {
    int y, m, d, h, mi;
    int n = 0;
    if ( std::sscanf ( line.c_str (), "%d-%d-%d %d:%d%n", &y, &m, &d, &h, &mi, &n ) != 5 )
      throw std::runtime_error ( "bad text entry" );
    while ( n < static_cast<int> ( line.size () ) && std::isspace ( static_cast<unsigned char> ( line[n] ) ) ) n++;
    name = line.substr ( static_cast<size_t> ( n ) );
    trim ( name );
    if ( name.empty () ) throw std::runtime_error ( "name empty" );
    t = toMins ( y, m, d, h, mi );
  }

  struct Event
  {
    int t;
    std::string zone;
    std::string name;

    Event () : t ( 0 ) {}
    Event ( int tt, const std::string & zz, const std::string & nn ) : t ( tt ), zone ( zz ), name ( nn ) {}
  };

  struct Visit
  {
    std::string name;
    std::string inZ;
    std::string outZ;
    int tin;
    int tout;
    bool open;

    Visit () : tin ( 0 ), tout ( 0 ), open ( false ) {}
    Visit ( const std::string & n, const std::string & iz, const std::string & oz, int ti, int to, bool o )
      : name ( n ), inZ ( iz ), outZ ( oz ), tin ( ti ), tout ( to ), open ( o ) {}
  };

  struct BaseEdge
  {
    std::string a;
    std::string b;
    int w;
  };
}

class CAuditFilter
{
  public:
    explicit CAuditFilter ( const std::string & zoneName )
      : m_zone ( zoneName ), m_hasFrom ( false ), m_hasTo ( false ), m_from ( 0 ), m_to ( 0 )
    {
    }

    CAuditFilter & notBefore ( int y, int m, int d, int h, int mi )
    {
      m_from = toMins ( y, m, d, h, mi );
      m_hasFrom = true;
      return *this;
    }

    CAuditFilter & notAfter ( int y, int m, int d, int h, int mi )
    {
      m_to = toMins ( y, m, d, h, mi );
      m_hasTo = true;
      return *this;
    }

    const std::string & zone () const { return m_zone; }
    bool hasFrom () const { return m_hasFrom; }
    bool hasTo () const { return m_hasTo; }
    int from () const { return m_from; }
    int to () const { return m_to; }

  private:
    std::string m_zone;
    bool m_hasFrom;
    bool m_hasTo;
    int m_from;
    int m_to;
};

class CVisitorLog
{
  public:
    std::set<std::string> search ( const CAuditFilter & filter ) const
    {
      int wantFrom = filter.hasFrom () ? filter.from () : std::numeric_limits<int>::min () / 4;
      int wantTo   = filter.hasTo () ? filter.to () : std::numeric_limits<int>::max () / 4;
      std::string target = filter.zone ();
      std::set<std::string> ans;

      bool targetInBase = m_zoneId.count ( target ) > 0;

      for ( size_t i = 0; i < m_visits.size (); i++ )
      {
        const Visit & v = m_visits[i];
        int earliest = 0;
        int latest = 0;

        if ( !targetInBase )
        {
          if ( v.open )
          {
            if ( v.inZ != target ) continue;
            earliest = v.tin;
            latest = INF_DIST;
          }
          else
          {
            bool atIn = ( v.inZ == target );
            bool atOut = ( v.outZ == target );
            if ( !atIn && !atOut ) continue;
            if ( atIn && atOut ) { earliest = v.tin; latest = v.tout; }
            else if ( atIn ) { earliest = v.tin; latest = v.tin; }
            else { earliest = v.tout; latest = v.tout; }
          }
        }
        else
        {
          if ( v.open )
          {
            int d1 = distAt ( v.inZ, target );
            if ( d1 >= INF_DIST / 2 ) continue;
            earliest = v.tin + d1;
            latest = INF_DIST;
            for ( size_t j = 0; j < m_events.size (); j++ )
            {
              const Event & e = m_events[j];
              if ( e.name != v.name || e.t <= v.tin ) continue;
              int dd = distAt ( target, e.zone );
              if ( dd >= INF_DIST / 2 ) continue;
              latest = std::min ( latest, e.t - dd );
            }
          }
          else
          {
            int d1 = distAt ( v.inZ, target );
            int d2 = distAt ( target, v.outZ );
            if ( d1 >= INF_DIST / 2 || d2 >= INF_DIST / 2 ) continue;
            earliest = v.tin + d1;
            latest = v.tout - d2;
            for ( size_t j = 0; j < m_events.size (); j++ )
            {
              const Event & e = m_events[j];
              if ( e.name != v.name ) continue;
              if ( e.t <= v.tin || e.t >= v.tout ) continue;
              int dd = distAt ( target, e.zone );
              if ( dd >= INF_DIST / 2 ) continue;
              latest = std::min ( latest, e.t - dd );
            }
          }
        }

        if ( earliest > latest ) continue;
        int A = std::max ( earliest, wantFrom );
        int B = std::min ( latest, wantTo );
        if ( A <= B ) ans.insert ( v.name );
      }
      return ans;
    }

  private:
    int distAt ( const std::string & a, const std::string & b ) const
    {
      std::map<std::string,int>::const_iterator ia = m_zoneId.find ( a );
      std::map<std::string,int>::const_iterator ib = m_zoneId.find ( b );
      if ( ia == m_zoneId.end () || ib == m_zoneId.end () ) return INF_DIST;
      return m_dist[ia->second][ib->second];
    }

    friend class CMilBase;
    std::vector<Visit> m_visits;
    std::vector<Event> m_events;
    std::map<std::string,int> m_zoneId;
    std::vector<std::vector<int> > m_dist;
};

class CMilBase
{
  public:
    CMilBase         ()
    {
    }
    void readBase    ( const std::string & baseFilename )
    {
      std::ifstream in ( baseFilename );
      if ( !in ) throw std::runtime_error ( "cannot read base" );

      std::vector<BaseEdge> edges;
      std::set<std::string> zones;

      std::string line;
      while ( std::getline ( in, line ) )
      {
        trim ( line );
        if ( line.empty () ) continue;
        std::istringstream is ( line );
        std::string a, b;
        int w;
        if ( !( is >> a >> b >> w ) ) throw std::runtime_error ( "bad base line" );
        if ( w < 1 || w > 9 ) throw std::runtime_error ( "bad weight" );
        std::string extra;
        if ( is >> extra ) throw std::runtime_error ( "bad base line" );
        BaseEdge e;
        e.a = a;
        e.b = b;
        e.w = w;
        edges.push_back ( e );
        zones.insert ( a );
        zones.insert ( b );
      }

      m_zoneList.assign ( zones.begin (), zones.end () );
      std::sort ( m_zoneList.begin (), m_zoneList.end () );
      m_zoneId.clear ();
      for ( int i = 0; i < static_cast<int> ( m_zoneList.size () ); i++ )
        m_zoneId[m_zoneList[i]] = i;

      int n = static_cast<int> ( m_zoneList.size () );
      std::vector<std::vector<std::pair<int,int> > > g ( n );
      for ( size_t i = 0; i < edges.size (); i++ )
      {
        const BaseEdge & e = edges[i];
        int u = m_zoneId[e.a];
        int v = m_zoneId[e.b];
        g[u].push_back ( std::make_pair ( v, e.w ) );
        g[v].push_back ( std::make_pair ( u, e.w ) );
      }

      m_dist.assign ( n, std::vector<int> ( n, INF_DIST ) );
      for ( int src = 0; src < n; src++ )
      {
        std::vector<int> & d = m_dist[src];
        std::vector<bool> inQ ( n, false );
        std::queue<int> q;
        d[src] = 0;
        q.push ( src );
        inQ[src] = true;

        while ( !q.empty () )
        {
          int u = q.front ();
          q.pop ();
          inQ[u] = false;

          for ( size_t i = 0; i < g[u].size (); i++ )
          {
            int v = g[u][i].first;
            int w = g[u][i].second;
            int cand = d[u] + w;
            if ( cand >= d[v] ) continue;
            d[v] = cand;
            if ( !inQ[v] )
            {
              q.push ( v );
              inQ[v] = true;
            }
          }
        }
      }
    }

    CVisitorLog processLog  ( const std::string & logFilename )
    {
      std::ifstream in ( logFilename, std::ios::binary );
      if ( !in ) throw std::runtime_error ( "cannot read log" );

      std::vector<unsigned char> buf ( ( std::istreambuf_iterator<char> ( in ) ), std::istreambuf_iterator<char> () );
      size_t pos = 0;
      std::vector<Event> events;

      while ( pos < buf.size () )
      {
        if ( startsWith ( buf, pos, "TEXT" ) )
        {
          std::string head = readLineBuf ( buf, pos );
          std::istringstream hs ( head );
          std::string tag, zone;
          unsigned N = 0;
          if ( !( hs >> tag >> zone >> N ) || tag != "TEXT" ) throw std::runtime_error ( "bad text head" );
          std::string extra;
          if ( hs >> extra ) throw std::runtime_error ( "bad text head" );
          for ( unsigned i = 0; i < N; i++ )
          {
            if ( pos >= buf.size () ) throw std::runtime_error ( "missing text item" );
            std::string ln = readLineBuf ( buf, pos );
            trim ( ln );
            if ( ln.empty () ) throw std::runtime_error ( "bad text item" );
            int t;
            std::string name;
            parseTxtEntry ( ln, t, name );
            events.push_back ( Event ( t, zone, name ) );
          }
        }
        else if ( pos + 4 <= buf.size () &&
                  ( ( buf[pos]=='I' && buf[pos+1]=='I' && buf[pos+2]=='I' && buf[pos+3]=='I' ) ||
                    ( buf[pos]=='M' && buf[pos+1]=='M' && buf[pos+2]=='M' && buf[pos+3]=='M' ) ) )
        {
          bool little = ( buf[pos] == 'I' );
          pos += 4;
          if ( pos + 2 > buf.size () ) throw std::runtime_error ( "bad bin header" );
          uint16_t zlen = little ? read16LE ( &buf[pos] ) : read16BE ( &buf[pos] );
          pos += 2;
          if ( pos + zlen > buf.size () ) throw std::runtime_error ( "bad bin zone" );
          std::string zone ( reinterpret_cast<const char*> ( &buf[pos] ), zlen );
          pos += zlen;
          if ( pos + 4 > buf.size () ) throw std::runtime_error ( "bad bin count" );
          uint32_t N = little ? read32LE ( &buf[pos] ) : read32BE ( &buf[pos] );
          pos += 4;

          for ( uint32_t i = 0; i < N; i++ )
          {
            if ( pos + 6 > buf.size () ) throw std::runtime_error ( "bad bin item" );
            uint32_t dt = little ? read32LE ( &buf[pos] ) : read32BE ( &buf[pos] );
            pos += 4;
            uint16_t len = little ? read16LE ( &buf[pos] ) : read16BE ( &buf[pos] );
            pos += 2;
            if ( pos + len > buf.size () ) throw std::runtime_error ( "bad bin name" );
            std::string name ( reinterpret_cast<const char*> ( &buf[pos] ), len );
            pos += len;
            int y, m, d, h, mi;
            decodeDT ( dt, y, m, d, h, mi );
            int t = toMins ( y, m, d, h, mi );
            events.push_back ( Event ( t, zone, name ) );
          }
        }
        else
          throw std::runtime_error ( "unknown format" );
      }

      std::map<std::string,int> cnt;
      std::map<std::string,Visit> openMap;
      std::vector<Visit> visits;

      for ( size_t i = 0; i < events.size (); i++ )
      {
        const Event & e = events[i];
        int & x = cnt[e.name];
        x++;
        if ( x % 2 == 1 )
        {
          openMap[e.name] = Visit ( e.name, e.zone, "", e.t, 0, true );
        }
        else
        {
          std::map<std::string,Visit>::iterator it = openMap.find ( e.name );
          if ( it == openMap.end () ) throw std::runtime_error ( "leave without enter" );
          Visit v = it->second;
          v.outZ = e.zone;
          v.tout = e.t;
          v.open = false;
          visits.push_back ( v );
          openMap.erase ( it );
        }
      }

      CVisitorLog res;
      res.m_zoneId = m_zoneId;
      res.m_dist = m_dist;
      res.m_events = events;
      res.m_visits = visits;
      for ( std::map<std::string,Visit>::iterator it = openMap.begin (); it != openMap.end (); ++it )
        res.m_visits.push_back ( it->second );
      return res;
    }
  private:
    std::vector<std::string> m_zoneList;
    std::map<std::string,int> m_zoneId;
    std::vector<std::vector<int> > m_dist;
};

#ifndef __PROGTEST__
static std::set<std::string> mkSet ( const char * const arr[], int n )
{
  std::set<std::string> x;
  for ( int i = 0; i < n; i++ ) x.insert ( arr[i] );
  return x;
}

void basicTests ( const CVisitorLog & log )
{
  {
    const char * const a[] = { "Alice Cooper", "George Peterson", "Henry Montgomery", "Jane Bush", "John Smith", "Tim Cook", "Robert Smith" };
    assert ( log . search ( CAuditFilter ( "headquarters" ) ) == mkSet ( a, 7 ) );
  }
  {
    const char * const a[] = { "Alice Cooper", "Henry Montgomery", "Jane Bush", "John Smith", "Robert Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) ) == mkSet ( a, 5 ) );
  }
  {
    const char * const a[] = { "Henry Montgomery", "Robert Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notAfter ( 2026, 3, 10, 8, 0 ) ) == mkSet ( a, 2 ) );
  }
  {
    const char * const a[] = { "Henry Montgomery", "Jane Bush", "John Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 11, 12, 0 ) ) == mkSet ( a, 3 ) );
  }
  {
    const char * const a[] = { "Alice Cooper", "Henry Montgomery", "Jane Bush", "John Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 0 ) . notAfter ( 2026, 3, 10, 13, 0 ) ) == mkSet ( a, 4 ) );
  }
  {
    const char * const a[] = { "Henry Montgomery" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 5 ) . notAfter ( 2026, 3, 10, 9, 5 ) ) == mkSet ( a, 1 ) );
  }
  {
    const char * const a[] = { "Henry Montgomery", "John Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 6 ) . notAfter ( 2026, 3, 10, 9, 6 ) ) == mkSet ( a, 2 ) );
  }
  {
    const char * const a[] = { "Alice Cooper", "Henry Montgomery", "John Smith" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 24 ) . notAfter ( 2026, 3, 10, 9, 24 ) ) == mkSet ( a, 3 ) );
  }
  {
    const char * const a[] = { "Alice Cooper", "Henry Montgomery" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 25 ) . notAfter ( 2026, 3, 10, 9, 25 ) ) == mkSet ( a, 2 ) );
  }
  {
    const char * const a[] = { "Robert Smith", "Henry Montgomery" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2024, 2, 1, 0, 0 ) . notAfter ( 2024, 3, 31, 0, 0 ) ) == mkSet ( a, 2 ) );
  }
  {
    const char * const a[] = { "Henry Montgomery" };
    assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2025, 2, 1, 0, 0 ) . notAfter ( 2025, 3, 31, 0, 0 ) ) == mkSet ( a, 1 ) );
  }
  {
    const char * const a[] = { "<classified>" };
    assert ( log . search ( CAuditFilter ( "privateParking" ) ) == mkSet ( a, 1 ) );
  }
}

int main ()
{
  class CMilBase b;
  b . readBase ( "base.txt" );

  const char * fns[] = { "in1.log", "in2.log", "in3.log" };
  for ( int i = 0; i < 3; i++ )
    basicTests ( b . processLog ( fns[i] ) );
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */