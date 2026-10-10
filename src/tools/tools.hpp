#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tools {

using ToolFunction = std::function<std::string(const json &args)>;

struct Tool {
  json schema;
  ToolFunction function;
};

class Registry {
private:
  std::unordered_map<std::string, Tool> tools_;
  json tools_array_ = json::array();

public:
  void register_tool(const std::string &name, const json &schema, ToolFunction function) {
    tools_[name] = { schema, function };
    tools_array_.push_back({
      { "type", "function" },
      { "function", schema },
    });
  }

  void unregister_tool(const std::string &name) {
    tools_.erase(name);
    tools_array_.erase(
      std::remove_if(tools_array_.begin(), tools_array_.end(),
                     [&name](const json &tool) { return tool["function"]["name"] == name; }),
      tools_array_.end());
  }

  const json &get_tools_schema() const {
    return tools_array_;
  }

  std::string execute(const std::string &name, const std::string &args_str) const {
    if (tools_.find(name) != tools_.end()) {
      try {
        json args = json::parse(args_str);
        return tools_.at(name).function(args);
      } catch (const std::exception &e) {
        return json{ { "error", e.what() } }.dump();
      }
    }
    return json{ { "error", "Tool not found" } }.dump();
  }
};

} // namespace tools
