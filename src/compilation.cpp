#include "compilation.hpp"
#include "ast.hpp"
#include "bytecode_generator.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "virtual_machine.hpp"

namespace compilation {
void run(std::string_view program) {
  Scanner scanner{program};
  std::pmr::monotonic_buffer_resource allocator{};
  ast::Node *node{Parser::parse(scanner, allocator)};
  Function function{BytecodeGenerator::generate(*node, allocator)};
  VirtualMachine::run(function);
}
} // namespace compilation
