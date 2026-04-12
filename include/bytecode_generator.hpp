#pragma once

#include "ast.hpp"
#include "error.hpp"
#include "instructions.hpp"
#include "value.hpp"

#include <memory_resource>

enum class BytecodeGeneratorErrorCode { UnresolvedSymbol };
class BytecodeGeneratorErrorCategory : public std::error_category {
  const char *name() const noexcept override { return "bytecode generator"; }

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
  static Result<Function> generate(const ast::Node &node,
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
    Result<RegisterIndex> operator()(const ast::Nil &node);
    Result<RegisterIndex> operator()(const ast::TableConstructor &node);
    Result<RegisterIndex> operator()(const ast::FunctionDeclaration &node);
  };

  struct Symbol {
    RegisterIndex index{};

    uint8_t flags{};
    static constexpr uint8_t upvalue{1};
    static constexpr uint8_t immutable{1 << 1};

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

  struct Resolver {
    BytecodeGenerator &generator;
    std::optional<Symbol> operator()(const ast::Name &node);
    std::optional<Symbol> operator()(const ast::Access &node);
    std::optional<Symbol> operator()(const ast::Subscript &node);
    template <typename T> std::optional<Symbol> operator()(const T &) {
      assert(false && "Resolver called on non-lvalue");
      std::unreachable();
    }
  };

  struct Scope {
    std::pmr::vector<std::pmr::string> variables{};
    Scope *outer;
  };

  struct State {
    State *outer{};
    Function function;
    InstructionWriter instructionWriter;
    SymbolTable symbolTable{};
    std::pmr::vector<RegisterIndex> breakJumps{};
    Scope *scope{};

    State(std::pmr::memory_resource &compilerAllocator,
          std::pmr::memory_resource &runtimeAllocator, State *outer = nullptr)
        : outer{outer}, function{runtimeAllocator},
          instructionWriter(function.instructions),
          symbolTable{&compilerAllocator}, breakJumps{&compilerAllocator},
          m_compilerAllocator{compilerAllocator},
          m_runtimeAllocator{runtimeAllocator} {
      void *storage{compilerAllocator.allocate(sizeof(Scope), alignof(Scope))};
      scope = new (storage)
          Scope{std::pmr::vector<std::pmr::string>{&compilerAllocator}};
    }

    template <typename... Args> RegisterIndex addConstant(Args &&...args) {
      function.constants.emplace_back(std::forward<Args>(args)...);
      return function.constants.size() - 1;
    }
    RegisterIndex allocateString(std::string_view stringView);
    RegisterIndex addJump() {
      function.jumps.push_back(function.instructions.size());
      return function.jumps.size() - 1;
    }
    void setJump(RegisterIndex index) {
      function.jumps[index] = function.instructions.size();
    }

    RegisterIndex allocateRegister() { return function.registerCount++; }
    RegisterIndex nextRegister() { return function.registerCount; }
    RegisterIndex lastRegister() { return function.registerCount - 1; }
    void copy(RegisterIndex to, RegisterIndex from) {
      if (from != to) {
        allocateRegister();
        instructionWriter.write(Operation::Copy, to, from);
      }
    }

    void define(std::string_view name, Symbol symbol);
    void undefine(std::string_view name);

  private:
    std::reference_wrapper<std::pmr::memory_resource> m_compilerAllocator;
    std::reference_wrapper<std::pmr::memory_resource> m_runtimeAllocator;
  };
  std::reference_wrapper<std::pmr::memory_resource> m_compilerAllocator;
  std::reference_wrapper<std::pmr::memory_resource> m_runtimeAllocator;
  State *m_state{};
  State *m_global{};

  BytecodeGenerator(std::pmr::memory_resource &compilerAllocator,
                    std::pmr::memory_resource &runtimeAllocator)
      : m_compilerAllocator{compilerAllocator},
        m_runtimeAllocator{runtimeAllocator} {
    pushState();
    m_global = m_state;
  }

  void defineNativeFunctions();

  void pushState() {
    void *storage{
        m_compilerAllocator.get().allocate(sizeof(State), alignof(State))};
    auto *state{new (storage) State{m_compilerAllocator, m_runtimeAllocator}};

    state->outer = m_state;
    m_state = state;
  }
  void popState() {
    assert(m_state);
    m_state = m_state->outer;
  }
  State &state() { return *m_state; }
  State &global() { return *m_global; }

  std::optional<Symbol> resolve(std::string_view name);
  std::optional<Error> assign(const ast::Node &node, RegisterIndex index);
  std::optional<Error> generateFunctionAt(const ast::Function &node,
                                          RegisterIndex index);
};
