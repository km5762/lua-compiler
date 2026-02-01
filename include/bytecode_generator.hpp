#pragma once

#include "ast.hpp"
#include "error.hpp"
#include "instructions.hpp"
#include "value.hpp"

#include <memory_resource>

enum class BytecodeGeneratorErrorCode { UnresolvedSymbol };
class BytecodeGeneratorErrorCategory : public std::error_category {
  const char *name() const noexcept override { return "parser"; }

  std::string message(int ev) const override {
    switch (static_cast<BytecodeGeneratorErrorCode>(ev)) {
    case BytecodeGeneratorErrorCode::UnresolvedSymbol:
      return "unresolved symbol";
    default:
      return "unknown bytecode generator error";
    }
  }
};
inline const std::error_category &bytecodeGeneratorCategory() {
  static BytecodeGeneratorErrorCategory instance;
  return instance;
}
inline std::error_code make_error_code(BytecodeGeneratorErrorCode e) noexcept {
  return {static_cast<int>(e), bytecodeGeneratorCategory()};
}
namespace std {
template <>
struct is_error_code_enum<BytecodeGeneratorErrorCode> : true_type {};
} // namespace std

class BytecodeGenerator {
public:
  static Function generate(const ast::Node &node,
                           std::pmr::memory_resource &compilerAllocator,
                           std::pmr::memory_resource &runtimeAllocator);

private:
  struct Visitor {
    BytecodeGenerator &generator;
    Result<RegisterIndex> operator()(const std::monostate &node);
    Result<RegisterIndex> operator()(const ast::Block &node);
    Result<RegisterIndex> operator()(const ast::Chunk &node);
    Result<RegisterIndex> operator()(const ast::LocalDeclaration &node);
    Result<RegisterIndex> operator()(const ast::Function &node);
    Result<RegisterIndex> operator()(const ast::Assignment &node);
    Result<RegisterIndex> operator()(const ast::BinaryOperator &node);
    Result<RegisterIndex> operator()(const ast::UnaryOperator &node);
    Result<RegisterIndex> operator()(const ast::Return &node);
    Result<RegisterIndex> operator()(const ast::Break &node);
    Result<RegisterIndex> operator()(const ast::Number &node);
    Result<RegisterIndex> operator()(const ast::Boolean &node);
    Result<RegisterIndex> operator()(const ast::Name &node);
    Result<RegisterIndex> operator()(const ast::Subscript &node);
    Result<RegisterIndex> operator()(const ast::Access &node);
    Result<RegisterIndex> operator()(const ast::FunctionCall &node);
    Result<RegisterIndex> operator()(const ast::WhileLoop &node);
    Result<RegisterIndex> operator()(const ast::RepeatLoop &node);
    Result<RegisterIndex> operator()(const ast::Conditional &node);
    Result<RegisterIndex> operator()(const ast::NumericForLoop &node);
    Result<RegisterIndex> operator()(const ast::GenericForLoop &node);
    Result<RegisterIndex> operator()(const ast::String &node);
  };

  struct Symbol {
    RegisterIndex index{};
    bool upvalue{};
    Symbol *outer{};
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

    State(std::pmr::memory_resource &compilerAllocator,
          std::pmr::memory_resource &runtimeAllocator, State *outer = nullptr)
        : outer{outer}, function{std::pmr::vector<uint8_t>(&runtimeAllocator),
                                 std::pmr::vector<Value>(&runtimeAllocator)},
          instructionWriter(function.instructions),
          symbolTable{&compilerAllocator} {}

    template <typename... Args> RegisterIndex addConstant(Args &&...args) {
      function.constants.emplace_back(std::forward<Args>(args)...);
      return function.constants.size() - 1;
    }

    RegisterIndex allocateRegister() { return function.registerCount++; }
    RegisterIndex nextRegister() { return function.registerCount; }
    RegisterIndex lastRegister() { return function.registerCount - 1; }
  };
  std::reference_wrapper<std::pmr::memory_resource> m_compilerAllocator;
  std::reference_wrapper<std::pmr::memory_resource> m_runtimeAllocator;
  std::pmr::vector<State> m_states{};

  BytecodeGenerator(std::pmr::memory_resource &compilerAllocator,
                    std::pmr::memory_resource &runtimeAllocator)
      : m_compilerAllocator{compilerAllocator},
        m_runtimeAllocator{runtimeAllocator}, m_states(&compilerAllocator) {
    m_states.emplace_back(compilerAllocator, runtimeAllocator);
  }

  void defineNativeFunctions();

  State &state() { return m_states.back(); }
  State &global() { return m_states.front(); }

  std::optional<Symbol> resolve(std::string_view name);
};
