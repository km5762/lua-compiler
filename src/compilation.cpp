#include "compilation.hpp"
#include "ast.hpp"
#include "bytecode_generator.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "virtual_machine.hpp"
#include <memory_resource>

namespace compilation {
std::optional<Error> run(std::string_view program) {
  std::pmr::monotonic_buffer_resource compilerAllocator{};
  Scanner scanner{program, compilerAllocator};
  Result<ast::Node *> result{Parser::parse(scanner, compilerAllocator)};
  if (!result) {
    return result.error();
  }
  std::pmr::monotonic_buffer_resource runtimeAllocator{};
  Function function{BytecodeGenerator::generate(**result, compilerAllocator,
                                                runtimeAllocator)};
  VirtualMachine::run(function);

  return {};
}
} // namespace compilation
