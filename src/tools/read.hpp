#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace nlohmann::literals;
namespace fs = std::filesystem;

namespace tools::read {

const std::string name = "read";

const json schema = R"({
  "name": "read",
  "description": "Read the contents of a file.",
  "parameters": {
    "type": "object",
    "properties": {
      "file_path": {
        "type": "string",
        "description": "The path to the file to read."
      },
      "max_lines": {
        "type": "integer",
        "description": "The maximum number of lines to read from the file. If not specified, read the entire file."
      }
    },
    "required": ["file_path"]
  }
})"_json;

std::string function(const json &args) {
  std::string file_path = args.value("file_path", "");
  int max_lines = args.value("max_lines", -1);
  if (!fs::exists(file_path))
    return json({ "error", "File does not exist" }).dump();
  try {
    std::ifstream file(file_path);
    if (!file.is_open())
      return json({ "error", "Failed to open file" }).dump();
    std::string content;
    if (max_lines < 0) {
      content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    } else {
      std::string line;
      int count = 0;
      while (std::getline(file, line) && count < max_lines) {
        content += line + "\n";
        count++;
      }
    }
    return json({ "content", content }).dump();
  } catch (const std::ifstream::failure &e) {
    return json({ "error", "Exception reading file: " + std::string(e.what()) }).dump();
  } catch (const std::exception &e) {
    return json({ "error", "Error: " + std::string(e.what()) }).dump();
  }
}

} // namespace tools::read
