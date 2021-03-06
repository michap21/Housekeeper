// Compared with std::stringstream, there are primary purpose of
// string::Printf:
//
// 1. Type-safe printing, with why and how explained in
//    http://www.drdobbs.com/stringprintf-a-typesafe-printf-family-fo/184401999.
//    Implementation includes
//
//    https://github.com/c42f/tinyformat
//    boost::format
//    std::stringstream
//
//    std::stringstream is not convenient enough in many cases.  For example:
//
//      std::cout << std::setprecision(2) << std::fixed << 1.23456 << "\n";
//
//    boost::format is the most convenient one.  We can have
//
//      std::cout << format("%2% %1%") % 36 % 77;
//
//    or
//
//      format fmter("%2% %1%");
//      fmter % 36; fmter % 77;
//      std::cout << fmter.c_str();
//
//    But the overloading of % might be overkilling and it would be
//    more efficient if it can write to std::cout directly.
//
//    tinyformat has an interface compatible with the C-printf style,
//    and it can writes to a stream or returns a std::string:
//
//      std::cout << tfm::printf(
//                  "%s, %s %d, %.2d:%.2d\n",
//                  weekday, month, day, hour, min);
//
//    or
//
//      tfm::format(std::cout,
//                  "%s, %s %d, %.2d:%.2d\n",
//                  weekday, month, day, hour, min);
//
// 2. High-performance -- most printed strings are not too long and
//    doens't need dynamic memory allocation.  Many StringPrintf
//    implementations doesn't enforce type-safe, but are
//    high-performance, including
//
//    https://developers.google.com/optimization/reference/base/stringprintf/
//    https://github.com/adobe/chromium/blob/master/base/stringprintf.h
//    https://github.com/google/protobuf/blob/master/src/google/protobuf/stubs/stringprintf.h
//
// According to
// https://github.com/c42f/tinyformat#compile-time-and-code-bloat,
// boost::format runs too slow and results in large executable binary
// files.  So here we port tinyformat.

#pragma once

#include "tinyformat.h" // https://github.com/c42f/tinyformat
#include <iostream>
#include <sstream>

namespace cloud {
namespace string {

template <typename... Args>
void Fprintf(std::ostream &out, const char *fmt, const Args &... args) {
  tinyformat::vformat(out, fmt, tinyformat::makeFormatList(args...));
}

template <typename... Args>
std::string Sprintf(const char *fmt, const Args &... args) {
  std::ostringstream oss;
  Fprintf(oss, fmt, args...);
  return oss.str();
}

template <typename... Args> void Printf(const char *fmt, const Args &... args) {
  Fprintf(std::cout, fmt, args...);
}

} // namespace string
} // namespace cloud
