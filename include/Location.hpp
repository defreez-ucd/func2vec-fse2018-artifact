// The pair of file / line number is the best unique identifier for an
// instruction that we have that remains constant between llvm instantiations.

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "llvm/Support/raw_ostream.h"
#include <string>

class Location {
public:
  std::string file;
  unsigned line = 0;

  Location() {}
  Location(std::string file, unsigned line) : file(file), line(line) {}
  Location(std::pair<std::string, unsigned> loc) : file(loc.first), line(loc.second) {}

  bool empty() const {
    return line == 0;
  }

  bool operator==(const Location& right) const {
    return file == right.file && line == right.line;
  }

  bool operator!=(const Location& right) const {
    return file != right.file || line != right.line;
  }

  // So Locations can be inserted into sets
  bool operator<(const Location& right) const {
    if (file == right.file) {
      return line < right.line;
    } else {
      return file < right.file;
    }
  }

  std::string str() const {
    return file + ":" + std::to_string(line);
  }


  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Location &L) {
    return OS << L.str();
  }

  friend std::ostream &operator<<(std::ostream &OS, const Location &L) {
    return OS << L.str();
  }
};


#endif
