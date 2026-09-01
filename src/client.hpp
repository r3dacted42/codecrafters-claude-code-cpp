#pragma once

#include <cpr/cpr.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "api.hpp"
#include "tools.hpp"

class Client : private tools::Registry {
  std::string api_key_;
  std::string base_url_;
  tools::Registry tools_registry_;
  std::vector<api::Message> messages_;

public:
  Client(const std::string &api_key, const std::string &base_url)
      : api_key_(api_key), base_url_(base_url) {
  }

  void set_system_prompt(const std::string &system_prompt) {
    if (messages_.empty()) {
      messages_.push_back({ "system", system_prompt });
    } else if (messages_[0].role != "system") {
      messages_.insert(messages_.begin(), { "system", system_prompt });
    } else {
      messages_[0].content = system_prompt;
    }
  }

  using tools::Registry::register_tool;
  using tools::Registry::unregister_tool;

  std::string chat(const std::string model, const api::Message &message) {
    messages_.push_back(message);
    bool looping = true;
    while (looping) {
      looping = false;

      json payload = {
        { "model", model },
        { "messages", messages_ },
      };
      if (!tools_registry_.get_tools_schema().empty()) {
        payload["tools"] = tools_registry_.get_tools_schema();
      }

      cpr::Response response = cpr::Post(cpr::Url{ base_url_ + "/chat/completions" },
                                         cpr::Header{ { "Authorization", "Bearer " + api_key_ },
                                                      { "Content-Type", "application/json" } },
                                         cpr::Body{ payload.dump() });
      if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << std::endl;
        break;
      }
      json result = json::parse(response.text);
      if (!result.contains("choices") || result["choices"].empty()) {
        std::cerr << "No choices in response" << std::endl;
        break;
      }
      auto &choice = result["choices"][0]["message"];

      api::Message assistant_msg;
      assistant_msg.role = choice.value("role", "");
      assistant_msg.content = choice.value("content", "");

      if (choice.contains("tool_calls")) {
        assistant_msg.tool_calls = choice["tool_calls"].get<std::vector<api::ToolCall>>();
        messages_.push_back(assistant_msg);

        for (const auto &tc : *assistant_msg.tool_calls) {
          std::string tool_name = tc.function.name;
          std::string tool_args = tc.function.arguments;
          std::string tool_response = tools_registry_.execute(tool_name, tool_args);
          messages_.push_back({ "tool", tool_response, tc.id, tool_name });
        }
        looping = true;
      } else {
        messages_.push_back(assistant_msg);
      }
    }
    return messages_.back().content;
  }
};