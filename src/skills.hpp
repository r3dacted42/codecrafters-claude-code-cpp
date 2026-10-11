#pragma once

#include <c4/std/string.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ryml/ryml.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct SkillMetadata {
  std::string name;
  std::string description;
};

struct SkillContent {
  SkillMetadata metadata;
  std::string body;
};

class Skills {
  // name -> content of md file
  std::unordered_map<std::string, SkillContent> skills_;

  static Skills &instance() {
    static Skills instance;
    return instance;
  }

  Skills() = default;
  ~Skills() = default;

public:
  Skills(const Skills &) = delete;
  Skills &operator=(const Skills &) = delete;

  static bool load_skills(const fs::path &skills_dir) {
    Skills &instance = Skills::instance();
    if (!fs::exists(skills_dir) || !fs::is_directory(skills_dir)) {
      return false;
    }

    for (const auto &entry : fs::directory_iterator(skills_dir)) {
      if (entry.is_directory() && fs::exists(entry.path() / "SKILL.md")) {
        std::string skill_name = entry.path().stem().string();
        std::ifstream file(entry.path() / "SKILL.md");
        if (!file) {
          std::cerr << "Failed to open skill file: " << entry.path() / "SKILL.md" << std::endl;
          continue;
        }
        std::stringstream buf;
        buf << file.rdbuf();
        std::string line, frontmatter = "", body = "";
        if (std::getline(buf, line) && line == "---") {
          while (std::getline(buf, line) && line != "---")
            frontmatter += line + "\n";
        } else {
          std::cerr << "Failed to find frontmatter in skill file: " << entry.path() / "SKILL.md"
                    << std::endl;
          continue;
        }
        buf >> std::ws; // skip whitespace
        if (!buf || buf.tellg() == std::streampos(-1)) {
          std::cerr << "Failed to read skill body in skill file: " << entry.path() / "SKILL.md"
                    << std::endl;
          continue;
        }
        body = buf.str().substr(buf.tellg());
        ryml::Tree yaml_tree = ryml::parse_in_arena(ryml::to_csubstr(frontmatter));
        SkillMetadata metadata{ .name = skill_name };
        if (yaml_tree["description"].valid()) {
          metadata.description = yaml_tree["description"].val().str;
        } else {
          std::cerr << "Failed to find description in frontmatter of skill file: "
                    << entry.path() / "SKILL.md" << std::endl;
          continue;
        }
        instance.skills_[skill_name] = { metadata, body };
      }
    }
    return true;
  }

  static std::string get_metadata_str() {
    Skills &instance = Skills::instance();
    std::stringstream ss;
    ss << "You have access to the following skills:\n\n";
    for (const auto &pair : instance.skills_) {
      const auto &skill_metadata = pair.second.metadata;
      ss << "- " << skill_metadata.name << ": " << skill_metadata.description << "\n";
    }
    return ss.str();
  }

  static std::string get_skill_prompt(const std::string &skill_name,
                                      const std::vector<std::string> &args) {
    Skills &instance = Skills::instance();
    if (!instance.skills_.contains(skill_name)) {
      throw std::runtime_error("Skill not found: " + skill_name);
    }
    std::string skill_body = instance.skills_.at(skill_name).body;
    if (args.empty()) {
      return skill_body;
    }
    std::string args_str = "";
    for (const auto &arg : args) {
      args_str += arg + " ";
    }
    args_str.pop_back(); // remove trailing space
    // replace all $ARGUMENTS
    while (skill_body.find("$ARGUMENTS") != std::string::npos && !args_str.empty()) {
      skill_body.replace(skill_body.find("$ARGUMENTS"), 10, args_str);
    }
    for (size_t i = 0; i < args.size(); ++i) {
      std::vector<std::string> placeholders = {
        "$ARGUMENTS[" + std::to_string(i) + "]", // $ARGUMENTS[0]
        "$" + std::to_string(i),                 // $0
      };
      for (const auto &placeholder : placeholders) {
        size_t pos = skill_body.find(placeholder);
        while (pos != std::string::npos) {
          skill_body.replace(pos, placeholder.length(), args[i]);
          pos = skill_body.find(placeholder, pos + args[i].length());
        }
      }
    }
    return skill_body;
  }

  static std::vector<std::string> get_prompts(const std::string &skill_invocation) {
    Skills &instance = Skills::instance();
    std::stringstream ss(skill_invocation);
    std::vector<std::string> skills, args;
    bool args_start = false;
    while (ss) {
      std::string part;
      ss >> part;
      if (part.empty())
        continue;
      if (!args_start && part.front() == '/') {
        if (!instance.skills_.contains(part.substr(1))) {
          args_start = true;
          args.push_back(part);
          continue;
        }
        skills.push_back(part.substr(1));
        continue;
      }
      args.push_back(part);
    }
    std::vector<std::string> prompts;
    for (const auto &skill : skills) {
      prompts.push_back(get_skill_prompt(skill, args));
    }
    return prompts;
  }
};
