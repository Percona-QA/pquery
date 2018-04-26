// Read an INI file into easy-to-access name/value pairs.

#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <ini.h>
#include <INIReader.hpp>

using std::string;

INIReader::INIReader(string filename) {
  _error = ini_parse(filename.c_str(), ValueHandler, this);
  }


int
INIReader::ParseError() {
  return _error;
  }


string
INIReader::Get(string section, string name, string default_value) {
  string key = MakeKey(section, name);
  return _values.count(key) ? _values[key] : default_value;
  }


eDBTYPE
INIReader::getDbType(string section, string name, eDBTYPE default_value) {
  string valstr = Get(section, name, "");
  std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
  if(valstr.empty()) { return default_value; }
  if(valstr == "mysql") { return eMYSQL; }
  if((valstr == "pgsql") || (valstr == "postgres") || (valstr == "postgresql"))  { return ePGSQL; }
  if((valstr == "mongo") || (valstr == "mongodb")) { return eMONGO; }
//throw std::logic_error("Invalid value for DB TYPE: " + valstr);
  return eNONE;
  }


long
INIReader::GetInteger(string section, string name, long default_value) {
  string valstr = Get(section, name, "");
  if(valstr.empty()){ return default_value; }

  std::istringstream vss;
  vss.str(valstr);
  long ipart = 0;
  char cpart = 0;                                 //can be K/M/G

  vss >> ipart;
  if(vss.fail()) {
    throw std::logic_error("Invalid value for " + name + ": " + valstr);
    }

  vss >> cpart;

  if (cpart == 0) {
    return ipart;
    }

  switch (cpart) {
    case 'k':
    case 'K': return (ipart * 1024);
    break;
    case 'm':
    case 'M': return (ipart * 1024 * 1024);
    break;
    case 'g':
    case 'G': return (ipart * 1024 * 1024 * 1024);
    break;
    default:
      throw std::logic_error("Invalid value for " + name + ": " + valstr);
      break;
    }
  return default_value;
  }


double INIReader::GetReal(string section, string name, double default_value) {
  string valstr = Get(section, name, "");
  const char* value = valstr.c_str();
  char* end;
  double n = strtod(value, &end);
  return end > value ? n : default_value;
  }


bool INIReader::GetBoolean(string section, string name, bool default_value) {
  string valstr = Get(section, name, "");
// Convert to lower case to make string comparisons case-insensitive
  std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
  if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1") {
    return true;
    }
  if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0") {
    return false;
    }
  return default_value;
  }


std::vector<std::string>
INIReader::GetSections() const
  {
  return _sections;
  }


string
INIReader::MakeKey(string section, string name) {
  string key = section + "." + name;
// Convert to lower case to make section/name lookups case-insensitive
  std::transform(key.begin(), key.end(), key.begin(), ::tolower);
  return key;
  }


int
INIReader::ValueHandler(void* user, const char* section, const char* name,
const char* value) {
  INIReader* reader = (INIReader*)user;
  string key = MakeKey(section, name);
  if (reader->_values[key].size() > 0)
    reader->_values[key] += "\n";
  reader->_values[MakeKey(section, name)] = value;

  std::vector<std::string>::iterator sec_beg = reader->_sections.begin();
  std::vector<std::string>::iterator sec_end = reader->_sections.end();
  if (std::find(sec_beg, sec_end, section) == reader->_sections.end())
    reader->_sections.push_back(section);
  return 1;
  }
