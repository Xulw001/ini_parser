/**
 * @file ini.h
 * @brief Implementation of the ini parser.
 *
 * This file contains a simple ini parser implemented using C++11
 *
 * @author nevermore.xulw@hotmail.com
 * @date 2025-08-12
 * @version 1.0
 * @copyright Copyright (c) 2025
 */
#ifndef _INI_HPP_
#define _INI_HPP_

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace ini {
namespace container {
template <typename T>
struct element {
    T value_;
    bool update_ = false;

    T value() const { return value_; }
    bool update() const { return update_; }
};

template <typename T>
class collections : public element<std::unordered_map<std::string, T>> {
   public:
    void setter(const std::string& key, const T& value) {
        auto it = this->value_.find(key);
        if (it == this->value_.end()) {
            this->value_.emplace(key, value);
        } else {
            it->second = value;
        }
    }

    T& getter(const std::string& key) {
        auto it = this->value_.find(key);
        if (it == this->value_.end()) {
            it = this->value_.emplace(key, T()).first;
        }
        return it->second;
    }

    T getter(const std::string& key) const {
        auto it = this->value_.find(key);
        if (it == this->value_.end()) {
            return T();
        }
        return it->second;
    }
};

using attribute = container::element<std::string>;
using section = container::collections<attribute>;
using config = container::collections<section>;
}  // namespace container

namespace parser {
class config : container::config {
    friend void read_ini(const std::string& path, config& conf);
    friend void write_ini(const std::string& path, config& conf);

   public:
    template <typename T>
    void set(const std::string& section, const std::string& key, const T& value) {
        std::stringstream ss;
        ss << value;
        set(section, key, ss.str());
    }

    template <>
    void set<std::string>(const std::string& section, const std::string& key, const std::string& value) {
        auto& it = getter(section);
        it.setter(key, {value, true});
        it.update_ = true;
    }

    void set(const std::string& section, const std::string& key, const char* value) {
        set(section, key, std::string(value));
    }

    template <typename T>
    T get(const std::string& section, const std::string& key, const T& value = T()) {
        std::stringstream ss(get(section, key));
        T val = value;
        ss >> val;
        return val;
    }

    std::string get(const std::string& section, const std::string& key) {
        auto it = getter(section);
        auto attr = it.getter(key);
        return attr.value();
    }
};

void read_file(const std::string& path, std::string& data) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return;

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    data.resize(size);
    file.read(data.data(), size);
    file.close();
}

// fetch line from data[npos] until find '\n'
std::size_t fetch_line(const std::string& data, std::size_t npos, std::string& front, std::string& middle, std::string& back) {
    auto begin = npos;
    // trim front character before real data
    while (npos < data.size() && std::isspace(data[npos])) ++npos;
    front.assign(&data[begin], &data[npos]);
    if (npos >= data.size()) return npos;

    bool comment = false;
    int end = -1;
    begin = npos;
    // fetch real data until find '\n'
    while (npos < data.size() && data[npos] != '\n') {
        if (std::isspace(data[npos])) {  // find consecutive blank character in remaining data
            if (end == -1) end = (int)npos;
        } else if (data[npos] == ';' || data[npos] == '#') {  // find comment
            if (end == -1) end = (int)npos;
            comment = true;
        } else if (!comment) {
            end = -1;
        }
        ++npos;
    }

    middle.assign(&data[begin], &data[end == -1 ? npos : end]);
    back.assign(&data[end == -1 ? npos : end], &data[npos + 1]);
    return npos;
}

void read_ini(const std::string& path, config& conf) {
    std::string data;
    read_file(path, data);

    std::string front, middle, back, section_name;
    for (std::size_t npos = 0; npos < data.size(); npos++) {
        npos = fetch_line(data, npos, front, middle, back);
        if (middle.empty()) {
            ;
        } else if (middle.front() == '[' && middle.back() == ']') {
            section_name = middle.substr(1, middle.size() - 2);
        } else {
            // heandle <K,V>
            size_t equal_pos = middle.find('=');
            if (equal_pos != std::string::npos) {
                std::string key = middle.substr(0, equal_pos);
                std::string value = middle.substr(equal_pos + 1);

                // trim blank character
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));

                auto& section = conf.getter(section_name);
                auto& attr = section.getter(key);
                if (!attr.update()) attr.value_ = value;
            }
        }
    }
}

#ifdef _FAST_OVERWRITE
void write_ini(const std::string& path, config& conf) {
    std::string data, endl("\n");
    data.reserve(1024);

    for (auto section : conf.value()) {
        data.append("[").append(section.first).append("]").append(endl);
        for (auto attr : section.second.value()) {
            data.append(attr.first).append(" = ").append(attr.second.value()).append(endl);
        }
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out.is_open()) {
        out.write(data.c_str(), data.size());
        out.close();
    }
}
#else
void output_section(std::string& data, container::section& section, std::string& endl, const std::string& name = "") {
    if (endl.empty()) {
        if (data[data.size() - 1] == '\r')
            endl = "\r";
        else if (data[data.size() - 1] == '\n')
            if (data[data.size() - 2] == '\r')
                endl = "\r\n";
            else
                endl = "\n";
    }

    if (section.update()) {
        if (name.size()) {
            data.append("[").append(name).append("]").append(endl);
        }

        for (auto& attr : section.value_) {
            if (attr.second.update()) {
                data.append(attr.first).append(" = ").append(attr.second.value()).append(endl);
                attr.second.update_ = false;
            }
        }
        section.update_ = false;
    }
}

void write_ini(const std::string& path, config& conf) {
    std::string data;
    read_file(path, data);

    std::string output;
    output.reserve(data.size() * 2);

    std::string front, middle, back, section_name, endl;
    for (std::size_t npos = 0; npos < data.size(); npos++) {
        npos = fetch_line(data, npos, front, middle, back);
        if (middle.empty()) {
            // do nothing
        }
        // handle section header
        else if (middle.front() == '[' && middle.back() == ']') {
            if (section_name.size()) {
                // process the remaining attributes in the previous section
                auto& section = conf.getter(section_name);
                output_section(output, section, endl);
            }
            section_name = middle.substr(1, middle.size() - 2);
            auto section = conf.getter(section_name);
            // section not update, ignore
            if (!section.update()) section_name = "";
        }
        // section not update, ignore
        else if (section_name.size()) {
            // heandle <K,V>
            size_t equal_pos = middle.find('=');
            if (equal_pos != std::string::npos) {
                std::string key = middle.substr(0, equal_pos);
                // trim blank character
                key.erase(key.find_last_not_of(" \t") + 1);

                auto& section = conf.getter(section_name);
                auto& attr = section.getter(key);
                if (attr.update()) {
                    middle = middle.substr(0, equal_pos + 1);
                    middle.append(" ").append(attr.value());
                    attr.update_ = false;
                }
            }
        }
        output.append(front);
        output.append(middle);
        output.append(back);
    }

    // process the remaining attributes in the last section
    if (section_name.size()) {
        auto& section = conf.getter(section_name);
        output_section(output, section, endl);
    }

    for (auto& it : conf.value_) {
        output_section(output, it.second, endl, it.first);
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out.is_open()) {
        out.write(output.c_str(), output.size());
        out.close();
    }
}
#endif
}  // namespace parser
}  // namespace ini

#endif