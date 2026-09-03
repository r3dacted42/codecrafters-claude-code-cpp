#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace nlohmann::literals;
namespace fs = std::filesystem;

namespace tools::run_bash_cmd {

const std::string name = "run_bash_cmd";

const json schema = R"({
  "name": "run_bash_cmd",
  "description": "Run a bash command or return an error in format { error: string }",
  "parameters": {
    "type": "object",
    "properties": {
      "command": {
        "type": "string",
        "description": "The bash command to run."
      }
    },
    "required": ["command"]
  }
})"_json;

std::string function(const json &args) {
  std::string command = args.value("command", "");
  int result = std::system(command.c_str());
  if (result != 0) {
    return json({ "error", "Failed to execute command" }).dump();
  }
  return json({ "result", "Command executed successfully" }).dump();
}

} // namespace tools::run_bash_cmd
