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

#ifdef MAXPACKET
#ifndef MAX_PACKET_DEFAULT
#define MAX_PACKET_DEFAULT 4194304
#endif
#endif

const uint16_t MAX_CON_FAILURES = 250;

#if defined(WIN32) || defined(_WIN32)
const std::string FSSEP = "\\";
#else
const std::string FSSEP = "/";
#endif
#endif
