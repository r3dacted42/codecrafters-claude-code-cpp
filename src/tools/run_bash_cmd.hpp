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
  "description": "Run a bash command and return output or return an error in format { error: string }",
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
  command += " 2>&1 >" + std::filesystem::temp_directory_path().string() + "/bash_output.txt";
  int result = std::system(command.c_str());
  if (result != 0)
    return json({ "error", "Failed to execute command" }).dump();
  std::ifstream output_file(std::filesystem::temp_directory_path() / "bash_output.txt");
  if (!output_file.is_open())
    return json({ "error", "Failed to open output file" }).dump();
  std::string output((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
  output_file.close();
  return json({ "result", output }).dump();
}

} // namespace tools::run_bash_cmd
