#pragma once

#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

namespace std {

template <typename T> void from_json(const json &j, std::optional<T> &opt) {
  if (j.is_null()) {
    opt = std::nullopt;
  } else {
    opt = j.get<T>();
  }
}

} // namespace std

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
  std::optional<std::string> content;
  std::optional<std::string> tool_call_id;
  std::optional<std::string> name;
  std::optional<std::vector<ToolCall>> tool_calls;
};

void to_json(json &j, const Message &m) {
  j = json{ { "role", m.role } };
  if (m.content)
    j["content"] = *m.content;
  if (m.tool_call_id)
    j["tool_call_id"] = *m.tool_call_id;
  if (m.name)
    j["name"] = *m.name;
  if (m.tool_calls)
    j["tool_calls"] = *m.tool_calls;
}

} // namespace api
