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
    instance.skills_.clear();

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
        body = buf.str().substr(buf.tellg());
        ryml::Tree yaml_tree = ryml::parse_in_arena(ryml::to_csubstr(frontmatter));
        SkillMetadata metadata{
          .name = skill_name,
          .description = yaml_tree["description"].valid() ? yaml_tree["description"].val().str
                                                          : "No description provided",
        };
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

  static std::optional<std::string> get_skill_body(const std::string &skill_name) {
    Skills &instance = Skills::instance();
    auto it = instance.skills_.find(skill_name);
    if (it != instance.skills_.end()) {
      return it->second.body;
    }
    return std::nullopt;
  }

  static std::string get_skill_prompt(const std::string &skill_invocation) {
    Skills &instance = Skills::instance();
    std::stringstream ss(skill_invocation);
    ss.ignore(); // skip the leading '/'
    std::string skill_name;
    ss >> skill_name >> std::ws; // read skill name and skip whitespace
    auto skill_body_opt = Skills::get_skill_body(skill_name);
    if (!skill_body_opt.has_value()) {
      throw std::runtime_error("Skill not found: " + skill_name);
    }
    std::string skill_body = *skill_body_opt, args_str = "";
    std::getline(ss, args_str);
    if (args_str.empty()) {
      return skill_body;
    }
    ss.clear();
    ss.str(args_str);
    std::vector<std::string> args;
    while (ss) {
      std::string arg;
      ss >> arg;
      if (!arg.empty()) {
        args.push_back(arg);
      }
    }
    // replace all $ARGUMENTS
    while (skill_body.find("$ARGUMENTS") != std::string::npos && !args_str.empty())
      skill_body.replace(skill_body.find("$ARGUMENTS"), 10, args_str);

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
};
