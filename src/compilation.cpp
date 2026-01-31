#include "compilation.hpp"
#include "ast.hpp"
#include "bytecode_generator.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "virtual_machine.hpp"

namespace compilation {
std::optional<Error> run(std::string_view program) {
  std::pmr::monotonic_buffer_resource allocator{};
  Scanner scanner{program, allocator};
  Result<ast::Node *> result{Parser::parse(scanner, allocator)};
  if (!result) {
    return result.error();
  }
  Function function{BytecodeGenerator::generate(**result, allocator)};
  VirtualMachine::run(function);

  return {};
}
} // namespace compilation
