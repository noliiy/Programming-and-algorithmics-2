#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream> 
#include <iomanip> 
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <utility>
#endif /* __PROGTEST__ */

class CVATRegister
{
  public:
                  CVATRegister   ();
                  ~CVATRegister  ();
    bool          newCompany     ( const std::string    & name,
                                   const std::string    & addr,
                                   const std::string    & taxID );
    bool          cancelCompany  ( const std::string    & name,
                                   const std::string    & addr );
    bool          cancelCompany  ( const std::string    & taxID );
    bool          invoice        ( const std::string    & taxID,
                                   unsigned int           amount );
    bool          invoice        ( const std::string    & name,
                                   const std::string    & addr,
                                   unsigned int           amount );
    bool          auditCompany   ( const std::string    & name,
                                   const std::string    & addr,
                                   unsigned int         & sumIncome ) const;
    bool          auditCompany   ( const std::string    & taxID,
                                   unsigned int         & sumIncome ) const;
    bool          firstCompany   ( std::string          & name,
                                   std::string          & addr ) const;
    bool          nextCompany    ( std::string          & name,
                                   std::string          & addr ) const;
    unsigned int  medianInvoice  () const;
  private:
    struct Company {
      std::string name, addr, taxID;
      unsigned int sumIncome;
      Company() : sumIncome(0) {}
    };
    // sorted by tax id for binary search
    struct ByTaxIDEntry {
      std::string taxID;
      size_t idx;
    };
    struct ByNameAddrEntry {
      std::string nameKey, addrKey;
      size_t idx;
    };

    struct CmpTaxID {
      const CVATRegister* reg;
      CmpTaxID(const CVATRegister* r) : reg(r) {}
      bool operator()(const ByTaxIDEntry& e, const std::string& id) const {
        return reg->companies[e.idx].taxID < id;
      }
    };
    struct CmpNameAddr {
      bool operator()(const ByNameAddrEntry& e, const std::pair<std::string, std::string>& p) const {
        return e.nameKey < p.first || (e.nameKey == p.first && e.addrKey < p.second);
      }
    };
    // for min-heap (upper half)
    struct Greater {
      bool operator()(unsigned int a, unsigned int b) const { return a > b; }
    };

    std::vector<Company> companies;
    std::vector<ByTaxIDEntry> byTaxID;
    std::vector<ByNameAddrEntry> byNameAddr;
    // two heaps for O(log n) insert and O(1) median: lower = max-heap, upper = min-heap; median = upper.front()
    std::vector<unsigned int> medianLower;
    std::vector<unsigned int> medianUpper;

    void addToMedian(unsigned int amount);
    static std::string toLower(const std::string& s);
    size_t findIdxByTaxID(const std::string& taxID) const;
    size_t findIdxByNameAddr(const std::string& nk, const std::string& ak) const;
    bool cancelCompanyByIdx(size_t i);
};

// names and addresses compared case insensitive so i lowercase them
std::string CVATRegister::toLower(const std::string& s) {
  std::string result;
  for (std::string::size_type i = 0; i < s.size(); i++) {
    result += (char)std::tolower((unsigned char)s[i]);
  }
  return result;
}

size_t CVATRegister::findIdxByTaxID(const std::string& taxID) const {
  std::vector<ByTaxIDEntry>::const_iterator it = std::lower_bound(byTaxID.begin(), byTaxID.end(), taxID, CmpTaxID(this));
  if (it == byTaxID.end()) return companies.size();
  if (companies[it->idx].taxID != taxID) return companies.size();
  return it->idx;
}

size_t CVATRegister::findIdxByNameAddr(const std::string& nk, const std::string& ak) const {
  std::pair<std::string, std::string> key(nk, ak);
  std::vector<ByNameAddrEntry>::const_iterator it = std::lower_bound(byNameAddr.begin(), byNameAddr.end(), key, CmpNameAddr());
  if (it == byNameAddr.end()) return companies.size();
  if (it->nameKey != nk || it->addrKey != ak) return companies.size();
  return it->idx;
}

CVATRegister::CVATRegister() {
}

CVATRegister::~CVATRegister() {
}

void CVATRegister::addToMedian(unsigned int amount) {
  if (medianUpper.empty()) {
    medianUpper.push_back(amount);
    std::push_heap(medianUpper.begin(), medianUpper.end(), Greater());
    return;
  }
  if (amount >= medianUpper.front()) {
    medianUpper.push_back(amount);
    std::push_heap(medianUpper.begin(), medianUpper.end(), Greater());
  } else {
    medianLower.push_back(amount);
    std::push_heap(medianLower.begin(), medianLower.end());
  }
  while (medianUpper.size() > medianLower.size() + 1) {
    std::pop_heap(medianUpper.begin(), medianUpper.end(), Greater());
    unsigned int t = medianUpper.back();
    medianUpper.pop_back();
    medianLower.push_back(t);
    std::push_heap(medianLower.begin(), medianLower.end());
  }
  while (medianLower.size() > medianUpper.size()) {
    std::pop_heap(medianLower.begin(), medianLower.end());
    unsigned int t = medianLower.back();
    medianLower.pop_back();
    medianUpper.push_back(t);
    std::push_heap(medianUpper.begin(), medianUpper.end(), Greater());
  }
}

bool CVATRegister::newCompany(const std::string& name, const std::string& addr, const std::string& taxID) {
  std::string nk = toLower(name);
  std::string ak = toLower(addr);

  if (findIdxByTaxID(taxID) != companies.size())
    return false;
  if (findIdxByNameAddr(nk, ak) != companies.size())
    return false;

  size_t pos = companies.size();
  Company c;
  c.name = name;
  c.addr = addr;
  c.taxID = taxID;
  c.sumIncome = 0;
  companies.push_back(c);

  ByTaxIDEntry e1;
  e1.taxID = taxID;
  e1.idx = pos;
  std::vector<ByTaxIDEntry>::iterator itTax = std::lower_bound(byTaxID.begin(), byTaxID.end(), taxID, CmpTaxID(this));
  byTaxID.insert(itTax, e1);

  ByNameAddrEntry e2;
  e2.nameKey = nk;
  e2.addrKey = ak;
  e2.idx = pos;
  std::pair<std::string, std::string> key(nk, ak);
  std::vector<ByNameAddrEntry>::iterator itName = std::lower_bound(byNameAddr.begin(), byNameAddr.end(), key, CmpNameAddr());
  byNameAddr.insert(itName, e2);
  return true;
}

bool CVATRegister::cancelCompany(const std::string& name, const std::string& addr) {
  std::string nk = toLower(name);
  std::string ak = toLower(addr);
  size_t idx = findIdxByNameAddr(nk, ak);
  if (idx == companies.size()) return false;
  return cancelCompanyByIdx(idx);
}

bool CVATRegister::cancelCompany(const std::string& taxID) {
  size_t idx = findIdxByTaxID(taxID);
  if (idx == companies.size()) return false;
  return cancelCompanyByIdx(idx);
}

// erase from both index vectors and from companies then fix idx in the rest
bool CVATRegister::cancelCompanyByIdx(size_t i) {
  std::string taxID = companies[i].taxID;
  std::string nk = toLower(companies[i].name);
  std::string ak = toLower(companies[i].addr);
  std::pair<std::string, std::string> key(nk, ak);

  std::vector<ByTaxIDEntry>::iterator itTax = std::lower_bound(byTaxID.begin(), byTaxID.end(), taxID, CmpTaxID(this));
  byTaxID.erase(itTax);
  std::vector<ByNameAddrEntry>::iterator itName = std::lower_bound(byNameAddr.begin(), byNameAddr.end(), key, CmpNameAddr());
  byNameAddr.erase(itName);

  companies.erase(companies.begin() + i);

  for (size_t j = 0; j < byTaxID.size(); j++) {
    if (byTaxID[j].idx > i) byTaxID[j].idx--;
  }
  for (size_t j = 0; j < byNameAddr.size(); j++) {
    if (byNameAddr[j].idx > i) byNameAddr[j].idx--;
  }
  return true;
}

bool CVATRegister::invoice(const std::string& taxID, unsigned int amount) {
  size_t idx = findIdxByTaxID(taxID);
  if (idx == companies.size()) return false;
  companies[idx].sumIncome += amount;
  addToMedian(amount);
  return true;
}

bool CVATRegister::invoice(const std::string& name, const std::string& addr, unsigned int amount) {
  std::string nk = toLower(name);
  std::string ak = toLower(addr);
  size_t idx = findIdxByNameAddr(nk, ak);
  if (idx == companies.size()) return false;
  companies[idx].sumIncome += amount;
  addToMedian(amount);
  return true;
}

bool CVATRegister::auditCompany(const std::string& name, const std::string& addr, unsigned int& sumIncome) const {
  std::string nk = toLower(name);
  std::string ak = toLower(addr);
  size_t idx = findIdxByNameAddr(nk, ak);
  if (idx == companies.size()) return false;
  sumIncome = companies[idx].sumIncome;
  return true;
}

bool CVATRegister::auditCompany(const std::string& taxID, unsigned int& sumIncome) const {
  size_t idx = findIdxByTaxID(taxID);
  if (idx == companies.size()) return false;
  sumIncome = companies[idx].sumIncome;
  return true;
}

bool CVATRegister::firstCompany(std::string& name, std::string& addr) const {
  if (byNameAddr.empty()) return false;
  size_t firstIdx = byNameAddr[0].idx;
  name = companies[firstIdx].name;
  addr = companies[firstIdx].addr;
  return true;
}

bool CVATRegister::nextCompany(std::string& name, std::string& addr) const {
  std::string nk = toLower(name);
  std::string ak = toLower(addr);
  std::pair<std::string, std::string> key(nk, ak);
  std::vector<ByNameAddrEntry>::const_iterator it = std::lower_bound(byNameAddr.begin(), byNameAddr.end(), key, CmpNameAddr());
  if (it == byNameAddr.end()) return false;
  if (it->nameKey != nk || it->addrKey != ak) return false;
  it++;
  if (it == byNameAddr.end()) return false;
  name = companies[it->idx].name;
  addr = companies[it->idx].addr;
  return true;
}

// median from heaps: upper holds the upper half, its min is the "greater of two middles" when even
unsigned int CVATRegister::medianInvoice() const {
  if (medianUpper.empty()) return 0;
  return medianUpper.front();
}

#ifndef __PROGTEST__
int               main           ()
{
  std::string name, addr;
  unsigned int sumIncome;

  CVATRegister b1;
  assert ( b1 . newCompany ( "ACME", "Thakurova", "666/666" ) );
  assert ( b1 . newCompany ( "ACME", "Kolejni", "666/666/666" ) );
  assert ( b1 . newCompany ( "Dummy", "Thakurova", "123456" ) );
  assert ( b1 . invoice ( "666/666", 2000 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "666/666/666", 3000 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 4000 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "aCmE", "Kolejni", 5000 ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . auditCompany ( "ACME", "Kolejni", sumIncome ) && sumIncome == 8000 );
  assert ( b1 . auditCompany ( "123456", sumIncome ) && sumIncome == 4000 );
  assert ( b1 . firstCompany ( name, addr ) && name == "ACME" && addr == "Kolejni" );
  assert ( b1 . nextCompany ( name, addr ) && name == "ACME" && addr == "Thakurova" );
  assert ( b1 . nextCompany ( name, addr ) && name == "Dummy" && addr == "Thakurova" );
  assert ( ! b1 . nextCompany ( name, addr ) );
  assert ( b1 . cancelCompany ( "ACME", "KoLeJnI" ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . cancelCompany ( "666/666" ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . invoice ( "123456", 100 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 300 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 200 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 230 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 830 ) );
  assert ( b1 . medianInvoice () == 830 );
  assert ( b1 . invoice ( "123456", 1830 ) );
  assert ( b1 . medianInvoice () == 1830 );
  assert ( b1 . invoice ( "123456", 2830 ) );
  assert ( b1 . medianInvoice () == 1830 );
  assert ( b1 . invoice ( "123456", 2830 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 3200 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . firstCompany ( name, addr ) && name == "Dummy" && addr == "Thakurova" );
  assert ( ! b1 . nextCompany ( name, addr ) );
  assert ( b1 . cancelCompany ( "123456" ) );
  assert ( ! b1 . firstCompany ( name, addr ) );

  CVATRegister b2;
  assert ( b2 . newCompany ( "ACME", "Kolejni", "abcdef" ) );
  assert ( b2 . newCompany ( "Dummy", "Kolejni", "123456" ) );
  assert ( ! b2 . newCompany ( "AcMe", "kOlEjNi", "1234" ) );
  assert ( b2 . newCompany ( "Dummy", "Thakurova", "ABCDEF" ) );
  assert ( b2 . medianInvoice () == 0 );
  assert ( b2 . invoice ( "ABCDEF", 1000 ) );
  assert ( b2 . medianInvoice () == 1000 );
  assert ( b2 . invoice ( "abcdef", 2000 ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( b2 . invoice ( "aCMe", "kOlEjNi", 3000 ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( ! b2 . invoice ( "1234567", 100 ) );
  assert ( ! b2 . invoice ( "ACE", "Kolejni", 100 ) );
  assert ( ! b2 . invoice ( "ACME", "Thakurova", 100 ) );
  assert ( ! b2 . auditCompany ( "1234567", sumIncome ) );
  assert ( ! b2 . auditCompany ( "ACE", "Kolejni", sumIncome ) );
  assert ( ! b2 . auditCompany ( "ACME", "Thakurova", sumIncome ) );
  assert ( ! b2 . cancelCompany ( "1234567" ) );
  assert ( ! b2 . cancelCompany ( "ACE", "Kolejni" ) );
  assert ( ! b2 . cancelCompany ( "ACME", "Thakurova" ) );
  assert ( b2 . cancelCompany ( "abcdef" ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( ! b2 . cancelCompany ( "abcdef" ) );
  assert ( b2 . newCompany ( "ACME", "Kolejni", "abcdef" ) );
  assert ( b2 . cancelCompany ( "ACME", "Kolejni" ) );
  assert ( ! b2 . cancelCompany ( "ACME", "Kolejni" ) );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
