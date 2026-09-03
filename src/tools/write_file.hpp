#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace nlohmann::literals;
namespace fs = std::filesystem;

namespace tools::write_file {

const std::string name = "write_file";

const json schema = R"({
  "name": "write_file",
  "description": "Write content to a file or return an error in format { error: string }",
  "parameters": {
    "type": "object",
    "properties": {
      "file_path": {
        "type": "string",
        "description": "The path to the file to write."
      },
      "content": {
        "type": "string",
        "description": "The content to write to the file."
      },
      "overwrite": {
        "type": "boolean",
        "description": "Whether to overwrite the file if it already exists. False by default."
      }
    },
    "required": ["file_path", "content"]
  }
})"_json;

std::string function(const json &args) {
  std::string file_path = args.value("file_path", "");
  std::string content = args.value("content", "");
  bool overwrite = args.value("overwrite", false);
  if (fs::exists(file_path) && !overwrite)
    return json({ "error", "File already exists and overwrite is false" }).dump();
  try {
    std::ofstream file(file_path);
    if (!file.is_open())
      return json({ "error", "Failed to open file" }).dump();
    file << content;
    file.close();
    return json({ "result", "Content written to file" }).dump();
  } catch (const std::exception &e) {
    return json({ "error", "Error: " + std::string(e.what()) }).dump();
  }
}

} // namespace tools::write_file
