#include <cstdlib>
#include <iostream>
#include <string>

#include "client.hpp"
#include "read_file.hpp"
#include "run_bash_cmd.hpp"
#include "skills.hpp"
#include "write_file.hpp"

int main(int argc, char *argv[]) {
  if (argc < 3 || std::string(argv[1]) != "-p") {
    std::cerr << "Expected first argument to be '-p'" << std::endl;
    return 1;
  }

  std::string prompt = argv[2];

  if (prompt.empty()) {
    std::cerr << "Prompt must not be empty" << std::endl;
    return 1;
  }

  const char *api_key_env = std::getenv("OPENROUTER_API_KEY");
  const char *base_url_env = std::getenv("OPENROUTER_BASE_URL");
  const char *model_name_env = std::getenv("MODEL_NAME");

  std::string api_key = api_key_env ? api_key_env : "";
  std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";
  std::string model_name = model_name_env ? model_name_env : "anthropic/claude-haiku-4.5";

  if (api_key.empty()) {
    std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
    return 1;
  }

  Client client(api_key, base_url);
  REGISTER_TOOL(client, read_file)
  REGISTER_TOOL(client, write_file)
  REGISTER_TOOL(client, run_bash_cmd)

  if (Skills::load_skills(fs::path(".claude/skills")))
    client.set_system_prompt(Skills::get_metadata_str());

  if (prompt.front() == '/') {
    try {
      prompt = Skills::get_skill_prompt(prompt);
    } catch (const std::exception &e) {
      std::cerr << "Error processing skill invocation: " << e.what() << std::endl;
      return 1;
    }
  }
  api::Message user_message{ "user", prompt };

  std::string response;
  try {
    response = client.chat(model_name, user_message);
  } catch (const std::exception &e) {
    std::cerr << "Error during chat: " << e.what() << std::endl;
    return 1;
  }

  std::cout << response << std::endl;

  return 0;
}
