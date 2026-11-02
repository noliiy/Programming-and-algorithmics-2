#ifndef __PROGTEST__
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <iostream>
#endif

class CFile
{
  public:
                   CFile                      ();
                   CFile                      ( const CFile     & other );
                   ~CFile                     ();
    CFile        & operator =                 ( const CFile     & other );

    bool           seek                       ( size_t           offset );
    size_t         read                       ( uint8_t           dst[],
                                                size_t            bytes );
    size_t         write                      ( const uint8_t     src[],
                                                size_t            bytes );
    void           truncate                   ();
    size_t         fileSize                   () const;
    void           addVersion                 ();
    bool           undoVersion                ();

  private:
    struct Blk
    {
      enum { CAP = 128 };
      uint8_t d[CAP];
      size_t  ref;
    };

    struct Version
    {
      Blk   ** blk;
      size_t   nb;
      size_t   len;
      size_t   pos;
    };

    Blk   ** m_blk;
    size_t   m_nb;
    size_t   m_len;
    size_t   m_pos;

    Version * m_vers;
    size_t    m_verN;
    size_t    m_verCap;

    static Blk * blkNew   ();
    static void  blkInc   ( Blk * b );
    static void  blkDec   ( Blk * b );
    static size_t blkCnt  ( size_t len );

    void releaseBlocks     ( Blk ** arr, size_t nb );
    void freeData          ();
    void freeVers          ();
    void growFile          ( size_t needLen );
    void ensureUniqueBlock ( size_t bi );
    void pushVer           ();
};

CFile::Blk * CFile::blkNew ()
{
  Blk * b = new Blk;
  memset ( b->d, 0, Blk::CAP );
  b->ref = 1;
  return b;
}

void CFile::blkInc ( Blk * b )
{
  if ( b )
    b->ref ++;
}

void CFile::blkDec ( Blk * b )
{
  if ( !b )
    return;
  if ( -- b->ref == 0 )
    delete b;
}

size_t CFile::blkCnt ( size_t len )
{
  if ( len == 0 )
    return 0;
  return ( len + Blk::CAP - 1 ) / Blk::CAP;
}

void CFile::releaseBlocks ( Blk ** arr, size_t nb )
{
  if ( !arr )
    return;
  for ( size_t i = 0; i < nb; i ++ )
    blkDec ( arr[i] );
  delete [] arr;
}

CFile::CFile ()
  : m_blk ( nullptr ), m_nb ( 0 ), m_len ( 0 ), m_pos ( 0 ),
    m_vers ( nullptr ), m_verN ( 0 ), m_verCap ( 0 )
{
}

void CFile::freeData ()
{
  releaseBlocks ( m_blk, m_nb );
  m_blk = nullptr;
  m_nb = 0;
  m_len = 0;
  m_pos = 0;
}

void CFile::freeVers ()
{
  for ( size_t j = 0; j < m_verN; j ++ )
    releaseBlocks ( m_vers[j].blk, m_vers[j].nb );
  delete [] m_vers;
  m_vers = nullptr;
  m_verN = 0;
  m_verCap = 0;
}

CFile::~CFile ()
{
  freeData ();
  freeVers ();
}

CFile::CFile ( const CFile & other )
  : m_blk ( nullptr ), m_nb ( 0 ), m_len ( 0 ), m_pos ( 0 ),
    m_vers ( nullptr ), m_verN ( 0 ), m_verCap ( 0 )
{
  m_len = other.m_len;
  m_pos = other.m_pos;
  m_nb = other.m_nb;
  if ( m_nb > 0 )
  {
    m_blk = new Blk * [m_nb];
    for ( size_t i = 0; i < m_nb; i ++ )
    {
      m_blk[i] = other.m_blk[i];
      blkInc ( m_blk[i] );
    }
  }

  m_verN = other.m_verN;
  m_verCap = m_verN;
  if ( m_verN > 0 )
  {
    m_vers = new Version[m_verN];
    for ( size_t j = 0; j < m_verN; j ++ )
    {
      m_vers[j].len = other.m_vers[j].len;
      m_vers[j].pos = other.m_vers[j].pos;
      m_vers[j].nb = other.m_vers[j].nb;
      if ( m_vers[j].nb > 0 )
      {
        m_vers[j].blk = new Blk * [m_vers[j].nb];
        for ( size_t i = 0; i < m_vers[j].nb; i ++ )
        {
          m_vers[j].blk[i] = other.m_vers[j].blk[i];
          blkInc ( m_vers[j].blk[i] );
        }
      }
      else
        m_vers[j].blk = nullptr;
    }
  }
}

CFile & CFile::operator = ( const CFile & other )
{
  if ( this == &other )
    return *this;

  freeData ();
  freeVers ();

  m_len = other.m_len;
  m_pos = other.m_pos;
  m_nb = other.m_nb;
  if ( m_nb > 0 )
  {
    m_blk = new Blk * [m_nb];
    for ( size_t i = 0; i < m_nb; i ++ )
    {
      m_blk[i] = other.m_blk[i];
      blkInc ( m_blk[i] );
    }
  }

  m_verN = other.m_verN;
  m_verCap = m_verN;
  if ( m_verN > 0 )
  {
    m_vers = new Version[m_verN];
    for ( size_t j = 0; j < m_verN; j ++ )
    {
      m_vers[j].len = other.m_vers[j].len;
      m_vers[j].pos = other.m_vers[j].pos;
      m_vers[j].nb = other.m_vers[j].nb;
      if ( m_vers[j].nb > 0 )
      {
        m_vers[j].blk = new Blk * [m_vers[j].nb];
        for ( size_t i = 0; i < m_vers[j].nb; i ++ )
        {
          m_vers[j].blk[i] = other.m_vers[j].blk[i];
          blkInc ( m_vers[j].blk[i] );
        }
      }
      else
        m_vers[j].blk = nullptr;
    }
  }
  return *this;
}

void CFile::growFile ( size_t needLen )
{
  size_t nb = blkCnt ( needLen );
  if ( nb <= m_nb )
    return;

  Blk ** nw = new Blk * [nb];
  for ( size_t i = 0; i < m_nb; i ++ )
    nw[i] = m_blk[i];
  for ( size_t i = m_nb; i < nb; i ++ )
    nw[i] = blkNew ();
  delete [] m_blk;
  m_blk = nw;
  m_nb = nb;
}

void CFile::ensureUniqueBlock ( size_t bi )
{
  Blk * b = m_blk[bi];
  if ( !b || b->ref <= 1 )
    return;
  Blk * nw = blkNew ();
  memcpy ( nw->d, b->d, Blk::CAP );
  m_blk[bi] = nw;
  blkDec ( b );
}

bool CFile::seek ( size_t offset )
{
  if ( offset > m_len )
    return false;
  m_pos = offset;
  return true;
}

size_t CFile::read ( uint8_t dst[], size_t bytes )
{
  if ( m_pos >= m_len || bytes == 0 )
    return 0;
  size_t todo = bytes < ( m_len - m_pos ) ? bytes : ( m_len - m_pos );
  size_t got = 0;
  while ( got < todo )
  {
    size_t off = m_pos + got;
    size_t bi = off / Blk::CAP;
    size_t io = off % Blk::CAP;
    size_t chunk = Blk::CAP - io;
    if ( chunk > todo - got )
      chunk = todo - got;
    memcpy ( dst + got, m_blk[bi]->d + io, chunk );
    got += chunk;
  }
  m_pos += todo;
  return todo;
}

size_t CFile::write ( const uint8_t src[], size_t bytes )
{
  if ( bytes == 0 )
    return 0;
  size_t need = m_pos + bytes;
  growFile ( need );
  size_t w = 0;
  while ( w < bytes )
  {
    size_t off = m_pos + w;
    size_t bi = off / Blk::CAP;
    ensureUniqueBlock ( bi );
    size_t io = off % Blk::CAP;
    size_t chunk = Blk::CAP - io;
    if ( chunk > bytes - w )
      chunk = bytes - w;
    memcpy ( m_blk[bi]->d + io, src + w, chunk );
    w += chunk;
  }
  m_pos += bytes;
  if ( need > m_len )
    m_len = need;
  return bytes;
}

void CFile::truncate ()
{
  m_len = m_pos;
  size_t nb = blkCnt ( m_len );
  for ( size_t i = nb; i < m_nb; i ++ )
    blkDec ( m_blk[i] );
  if ( nb != m_nb )
  {
    Blk ** nw = nullptr;
    if ( nb > 0 )
    {
      nw = new Blk * [nb];
      for ( size_t i = 0; i < nb; i ++ )
        nw[i] = m_blk[i];
    }
    delete [] m_blk;
    m_blk = nw;
    m_nb = nb;
  }
}

size_t CFile::fileSize () const
{
  return m_len;
}

void CFile::pushVer ()
{
  if ( m_verN >= m_verCap )
  {
    size_t nc = m_verCap ? m_verCap * 2 : 4;
    Version * nv = new Version[nc];
    for ( size_t i = 0; i < m_verN; i ++ )
      nv[i] = m_vers[i];
    delete [] m_vers;
    m_vers = nv;
    m_verCap = nc;
  }
  Version & v = m_vers[m_verN];
  v.len = m_len;
  v.pos = m_pos;
  v.nb = m_nb;
  if ( m_nb > 0 )
  {
    v.blk = new Blk * [m_nb];
    for ( size_t i = 0; i < m_nb; i ++ )
    {
      v.blk[i] = m_blk[i];
      blkInc ( m_blk[i] );
    }
  }
  else
    v.blk = nullptr;
  m_verN ++;
}

void CFile::addVersion ()
{
  pushVer ();
}

bool CFile::undoVersion ()
{
  if ( m_verN == 0 )
    return false;
  m_verN --;
  Version & v = m_vers[m_verN];

  releaseBlocks ( m_blk, m_nb );
  m_blk = v.blk;
  m_nb = v.nb;
  m_len = v.len;
  m_pos = v.pos;

  v.blk = nullptr;
  v.nb = 0;
  v.len = 0;
  v.pos = 0;

  return true;
}

#ifndef __PROGTEST__
bool               writeTest                  ( CFile           & x,
                                                const std::initializer_list<uint8_t> & data,
                                                size_t            wrLen )
{
  return x . write ( data . begin (), data . size () ) == wrLen;
}

bool               readTest                   ( CFile           & x,
                                                const std::initializer_list<uint8_t> & data,
                                                size_t            rdLen )
{
  uint8_t  tmp[100];
  uint32_t idx = 0;

  if ( x . read ( tmp, rdLen ) != data . size ())
    return false;
  for ( auto v : data )
    if ( tmp[idx++] != v )
      return false;
  return true;
}

int main ()
{
  CFile f0;

  assert ( writeTest ( f0, {10, 20, 30}, 3 ) );
  assert ( f0 . fileSize () == 3 );
  assert ( writeTest ( f0, {60, 70, 80}, 3 ) );
  assert ( f0 . fileSize () == 6 );
  assert ( f0 . seek ( 2 ));
  assert ( writeTest ( f0, {5, 4}, 2 ) );
  assert ( f0 . fileSize () == 6 );
  assert ( f0 . seek ( 1 ));
  assert ( readTest ( f0, {20, 5, 4, 70, 80}, 7 ));
  assert ( f0 . seek ( 3 ));
  f0 . addVersion();
  assert ( f0 . seek ( 6 ));
  assert ( writeTest ( f0, {100, 101, 102, 103}, 4 ) );
  f0 . addVersion();
  assert ( f0 . seek ( 5 ));
  CFile f1 ( f0 );
  f0 . truncate ();
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70}, 20 ));
  assert ( f0 . undoVersion () );
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70, 80, 100, 101, 102, 103}, 20 ));
  assert ( f0 . undoVersion () );
  assert ( f0 . seek ( 0 ));
  assert ( readTest ( f0, {10, 20, 5, 4, 70, 80}, 20 ));
  assert ( !f0 . seek ( 100 ));
  assert ( writeTest ( f1, {200, 210, 220}, 3 ) );
  assert ( f1 . seek ( 0 ));
  assert ( readTest ( f1, {10, 20, 5, 4, 70, 200, 210, 220, 102, 103}, 20 ));
  assert ( f1 . undoVersion () );
  assert ( f1 . undoVersion () );
  assert ( readTest ( f1, {4, 70, 80}, 20 ));
  assert ( !f1 . undoVersion () );
  return EXIT_SUCCESS;
}
#endif
