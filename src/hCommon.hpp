#ifndef __HCOMMON_HPP__
#define __HCOMMON_HPP__

#include <string>

#ifndef PQVERSION
#define PQVERSION = "UNKNOWN"
#endif

#ifndef PQREVISION
#define PQREVISION "UNKNOWN"
#endif

#ifndef PQRELDATE
#define PQRELDATE "UNKNOWN"
#endif

#ifndef MYSQL_FORK
#define MYSQL_FORK "UNKNOWN"
#endif

#if defined(WIN32) || defined(_WIN32)
const std::string FSSEP = "\\";
#else
const std::string FSSEP = "/";
#endif
#endif
