#pragma once

#include "value.hpp"

#include <optional>
#include <string_view>
#include <unordered_map>

struct Symbol {
  RegisterIndex index{};
  bool upvalue{};
};

class SymbolTable {
public:
  SymbolTable *outerTable{};
  SymbolTable *globalTable{};

  SymbolTable() = default;
  SymbolTable(std::unordered_map<std::string_view, Symbol> &&table)
      : m_table{std::move(table)} {};
  SymbolTable(SymbolTable &symbolTable) { symbolTable.outerTable = this; }

  void define(std::string_view name, Symbol value) { m_table[name] = value; }

  std::optional<Symbol> resolve(std::string_view name) {
    Symbol symbol{};
    SymbolTable *current{this};

    // found in local scope
    auto it{m_table.find(name)};
    if (it != m_table.end()) {
      return it->second;
    }

    // traversing up scopes
    while (true) {
      auto it{m_table.find(name)};
      if (it != m_table.end()) {
        return it->second;
      }
      if (!outerTable) {
        break;
      }
      current = current->outerTable;
    }

    return symbol;
  }

private:
  std::unordered_map<std::string_view, Symbol> m_table{};
};
