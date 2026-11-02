#ifndef __PROGTEST__
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <array>
#include <cassert>
using namespace std;
#endif /* __PROGTEST__ */

class CTimeStamp
{
  public:
    CTimeStamp ( int year, int month, int day, int hour, int min, int sec )
      : m_Year(year), m_Month(month), m_Day(day), m_Hour(hour), m_Min(min), m_Sec(sec) {}

    bool operator <= ( const CTimeStamp & other ) const
    {
      if ( m_Year != other.m_Year )   return m_Year < other.m_Year;
      if ( m_Month != other.m_Month ) return m_Month < other.m_Month;
      if ( m_Day != other.m_Day )     return m_Day < other.m_Day;
      if ( m_Hour != other.m_Hour )   return m_Hour < other.m_Hour;
      if ( m_Min != other.m_Min )     return m_Min < other.m_Min;
      return m_Sec <= other.m_Sec;
    }

  private:
    int m_Year, m_Month, m_Day, m_Hour, m_Min, m_Sec;
};

class CContact
{
  public:
    CContact ( CTimeStamp ts, int phone1, int phone2 )
      : m_Time(ts), m_Phone1(phone1), m_Phone2(phone2) {}

    CTimeStamp getTime   ( void ) const { return m_Time; }
    int        getPhone1 ( void ) const { return m_Phone1; }
    int        getPhone2 ( void ) const { return m_Phone2; }

  private:
    CTimeStamp m_Time;
    int m_Phone1;
    int m_Phone2;
};

class CEFaceMask
{
  public:
    CEFaceMask & addContact ( const CContact & contact )
    {
      m_Contacts.push_back(contact);
      return *this;
    }

    vector<int> listContacts ( int phone ) const
    {
      vector<int> result;
      for ( size_t i = 0; i < m_Contacts.size(); i++ )
      {
        int other = -1;
        if ( m_Contacts[i].getPhone1() == phone )
          other = m_Contacts[i].getPhone2();
        else if ( m_Contacts[i].getPhone2() == phone )
          other = m_Contacts[i].getPhone1();

        if ( other == -1 || other == phone )
          continue;

        bool dup = false;
        for ( size_t j = 0; j < result.size(); j++ )
          if ( result[j] == other )
          { dup = true; break; }

        if ( !dup )
          result.push_back(other);
      }
      return result;
    }

    vector<int> listContacts ( int phone, CTimeStamp from, CTimeStamp to ) const
    {
      vector<int> result;
      for ( size_t i = 0; i < m_Contacts.size(); i++ )
      {
        if ( !(from <= m_Contacts[i].getTime()) || !(m_Contacts[i].getTime() <= to) )
          continue;

        int other = -1;
        if ( m_Contacts[i].getPhone1() == phone )
          other = m_Contacts[i].getPhone2();
        else if ( m_Contacts[i].getPhone2() == phone )
          other = m_Contacts[i].getPhone1();

        if ( other == -1 || other == phone )
          continue;

        bool dup = false;
        for ( size_t j = 0; j < result.size(); j++ )
          if ( result[j] == other )
          { dup = true; break; }

        if ( !dup )
          result.push_back(other);
      }
      return result;
    }

  private:
    vector<CContact> m_Contacts;
};

#ifndef __PROGTEST__
int main ()
{
  CEFaceMask test;

  test . addContact ( CContact ( CTimeStamp ( 2021, 1, 10, 12, 40, 10 ), 123456789, 999888777 ) );
  test . addContact ( CContact ( CTimeStamp ( 2021, 1, 12, 12, 40, 10 ), 123456789, 111222333 ) )
       . addContact ( CContact ( CTimeStamp ( 2021, 2, 5, 15, 30, 28 ), 999888777, 555000222 ) );
  test . addContact ( CContact ( CTimeStamp ( 2021, 2, 21, 18, 0, 0 ), 123456789, 999888777 ) );
  test . addContact ( CContact ( CTimeStamp ( 2021, 1, 5, 18, 0, 0 ), 123456789, 456456456 ) );
  test . addContact ( CContact ( CTimeStamp ( 2021, 2, 1, 0, 0, 0 ), 123456789, 123456789 ) );
  assert ( test . listContacts ( 123456789 ) == (vector<int> {999888777, 111222333, 456456456}) );
  assert ( test . listContacts ( 999888777 ) == (vector<int> {123456789, 555000222}) );
  assert ( test . listContacts ( 191919191 ) == (vector<int> {}) );
  assert ( test . listContacts ( 123456789, CTimeStamp ( 2021, 1, 5, 18, 0, 0 ), CTimeStamp ( 2021, 2, 21, 18, 0, 0 ) ) == (vector<int> {999888777, 111222333, 456456456}) );
  assert ( test . listContacts ( 123456789, CTimeStamp ( 2021, 1, 5, 18, 0, 1 ), CTimeStamp ( 2021, 2, 21, 17, 59, 59 ) ) == (vector<int> {999888777, 111222333}) );
  assert ( test . listContacts ( 123456789, CTimeStamp ( 2021, 1, 10, 12, 41, 9 ), CTimeStamp ( 2021, 2, 21, 17, 59, 59 ) ) == (vector<int> {111222333}) );
  return 0;
}
#endif /* __PROGTEST__ */
