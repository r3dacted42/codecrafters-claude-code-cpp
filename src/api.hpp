#pragma once

#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

namespace api {

struct FunctionCall {
  std::string name;
  std::string arguments;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FunctionCall, name, arguments)

struct ToolCall {
  std::string id;
  std::string type = "function";
  FunctionCall function;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ToolCall, id, type, function)

struct Message {
  std::string role;
  std::string content;

  std::optional<std::string> tool_call_id;
  std::optional<std::string> name;
  std::optional<std::vector<ToolCall>> tool_calls;
};

void to_json(json &j, const Message &m) {
  j = json{ { "role", m.role } };
  if (m.content.empty() && m.tool_calls.has_value())
    j["content"] = nullptr;
  else
    j["content"] = m.content;
  if (m.tool_call_id)
    j["tool_call_id"] = *m.tool_call_id;
  if (m.name)
    j["name"] = *m.name;
  if (m.tool_calls)
    j["tool_calls"] = *m.tool_calls;
}

} // namespace api
