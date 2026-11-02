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

bool report ( const string & fileName, ostream & out )
{
  ifstream in ( fileName );
  if ( ! in . is_open () )
    return false;

  struct TContact
  {
    string m_Name;
    string m_Surname;
    string m_Phone;
  };

  vector<TContact> contacts;
  string line;
  bool separatorFound = false;

  while ( getline ( in, line ) )
  {
    if ( line . empty () )
    {
      separatorFound = true;
      break;
    }

    istringstream iss ( line );
    TContact contact;
    string extra;

    if ( ! ( iss >> contact . m_Name >> contact . m_Surname >> contact . m_Phone ) )
      return false;
    if ( iss >> extra )
      return false;

    if ( contact . m_Phone . length () != 9 )
      return false;
    if ( contact . m_Phone[0] == '0' )
      return false;
    if ( ! all_of ( contact . m_Phone . begin (), contact . m_Phone . end (),
                     [] ( char c ) { return isdigit ( static_cast<unsigned char> ( c ) ); } ) )
      return false;

    contacts . push_back ( contact );
  }

  if ( ! separatorFound )
    return false;

  while ( getline ( in, line ) )
  {
    istringstream iss ( line );
    string query;
    string extra;
    int matches = 0;

    if ( ! ( iss >> query ) )
      return false;
    if ( iss >> extra )
      return false;

    for ( const auto & contact : contacts )
    {
      if ( contact . m_Name == query || contact . m_Surname == query )
      {
        out << contact . m_Name << ' '
            << contact . m_Surname << ' '
            << contact . m_Phone << '\n';
        ++ matches;
      }
    }
    out << "-> " << matches << '\n';
  }

  return true;
}

#ifndef __PROGTEST__
int main ()
{
  ostringstream oss;
  oss . str ( "" );
  assert ( report( "tests/test0_in.txt", oss ) == true );
  assert ( oss . str () ==
    "John Christescu 258452362\n"
    "John Harmson 861647702\n"
    "-> 2\n"
    "-> 0\n"
    "Josh Dakhov 264112084\n"
    "Dakhov Speechley 865216101\n"
    "-> 2\n"
    "John Harmson 861647702\n"
    "-> 1\n" );
  oss . str ( "" );
  assert ( report( "tests/test1_in.txt", oss ) == false );
  return 0;
}
#endif /* __PROGTEST__ */
