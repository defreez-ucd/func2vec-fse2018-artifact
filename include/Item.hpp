#ifndef ITEM_HPP
#define ITEM_HPP

#include "Location.hpp"
#include <iostream>

struct Item {
  enum class Type { CALL, STORE, LOAD, EC };

  Item(Item::Type type, Location loc, std::string name) : type(type), loc(loc), name(name) {}

  Item::Type type;
  Location loc;
  std::string name;  
  std::string tactic;

  std::string getType() const {
    switch (type) {
    case Type::CALL:
      return "CALL";
    case Type::STORE:
      return "STORE";
    case Type::LOAD:
      return "LOAD";
    case Type::EC:
      return "EC";
    }
    abort();
  }

  std::ostream& raw(std::ostream &OS) const {
    return OS << tactic << "|" << getType() << "|" << name;
  } 

  std::ostream& format(std::ostream &OS) const {
    return OS << loc << ":\"" << name << "\"";
  }

  bool operator<(const Item& right) const {
    return name < right.name;
  }

  bool operator==(const Item& right) const {
    return name == right.name && type == right.type && loc == right.loc;
  }
};

#endif
