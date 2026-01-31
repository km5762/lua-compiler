#pragma once

#include "ast.hpp"
#include "instructions.hpp"
#include "value.hpp"

#include <memory_resource>

class BytecodeGenerator {
public:
  static Function generate(const ast::Node &node,
                           std::pmr::memory_resource &allocator);

private:
  struct Visitor {
    BytecodeGenerator &generator;
    RegisterIndex operator()(const std::monostate &node);
    RegisterIndex operator()(const ast::Block &node);
    RegisterIndex operator()(const ast::Chunk &node);
    RegisterIndex operator()(const ast::LocalDeclaration &node);
    RegisterIndex operator()(const ast::Function &node);
    RegisterIndex operator()(const ast::Assignment &node);
    RegisterIndex operator()(const ast::BinaryOperator &node);
    RegisterIndex operator()(const ast::UnaryOperator &node);
    RegisterIndex operator()(const ast::Return &node);
    RegisterIndex operator()(const ast::Break &node);
    RegisterIndex operator()(const ast::Number &node);
    RegisterIndex operator()(const ast::Boolean &node);
    RegisterIndex operator()(const ast::Name &node);
    RegisterIndex operator()(const ast::Subscript &node);
    RegisterIndex operator()(const ast::Access &node);
    RegisterIndex operator()(const ast::FunctionCall &node);
    RegisterIndex operator()(const ast::WhileLoop &node);
    RegisterIndex operator()(const ast::RepeatLoop &node);
    RegisterIndex operator()(const ast::Conditional &node);
    RegisterIndex operator()(const ast::NumericForLoop &node);
    RegisterIndex operator()(const ast::GenericForLoop &node);
    RegisterIndex operator()(const ast::String &node);
  };

  struct Symbol {
    RegisterIndex index{};
    bool upvalue{};
  };
  struct TransparentHash {
    using is_transparent = void;

    std::size_t operator()(std::string_view sv) const noexcept {
      return std::hash<std::string_view>{}(sv);
    }

    std::size_t operator()(const std::pmr::string &s) const noexcept {
      return std::hash<std::string_view>{}(s);
    }
  };
  struct TransparentEqual {
    using is_transparent = void;

    bool operator()(std::string_view a, std::string_view b) const noexcept {
      return a == b;
    }

    bool operator()(const std::pmr::string &a,
                    std::string_view b) const noexcept {
      return a == b;
    }

    bool operator()(std::string_view a,
                    const std::pmr::string &b) const noexcept {
      return a == b;
    }

    bool operator()(const std::pmr::string &a,
                    const std::pmr::string &b) const noexcept {
      return a == b;
    }
  };
  using SymbolTable =
      std::pmr::unordered_map<std::pmr::string, Symbol, TransparentHash,
                              TransparentEqual>;
  struct State {
    State *outer{};
    Function function{};
    InstructionWriter instructionWriter;
    SymbolTable symbolTable{};

    State(std::pmr::memory_resource &allocator, State *outer = nullptr)
        : m_allocator{&allocator}, outer{outer},
          function{std::pmr::vector<uint8_t>(&allocator),
                   std::pmr::vector<Value>(&allocator)},
          instructionWriter(function.instructions), symbolTable{&allocator} {}

    template <typename... Args> RegisterIndex addConstant(Args &&...args) {
      function.constants.emplace_back(std::forward<Args>(args)...);
      return function.constants.size() - 1;
    }

    RegisterIndex allocateRegister() { return function.registerCount++; }
    RegisterIndex nextRegister() { return function.registerCount; }
    RegisterIndex lastRegister() { return function.registerCount - 1; }

  private:
    std::pmr::memory_resource *m_allocator{};
  };
  std::reference_wrapper<std::pmr::memory_resource> m_allocator;
  std::pmr::vector<State> m_states{};

  BytecodeGenerator(std::pmr::memory_resource &allocator)
      : m_allocator{allocator}, m_states(&allocator) {
    m_states.emplace_back(allocator);
  }

  void defineNativeFunctions();

  State &state() { return m_states.back(); }
  State &global() { return m_states.front(); }

  std::optional<Symbol> resolve(std::string_view name);
};
