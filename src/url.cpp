#include "url.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::string url::decode_uri(std::string &uri) {
  std::string decoded;
  decoded.reserve(uri.length());

  for (size_t i = 0; i < uri.length(); ++i) {
    if (uri[i] == '%' && i + 2 < uri.length()) {
      if (std::isxdigit(uri[i + 1]) && std::isxdigit(uri[i + 2])) {
        u_int16_t hex_value;
        std::stringstream hex_stream;
        hex_stream << std::hex << uri.substr(i + 1, 2);
        hex_stream >> hex_value;

        decoded += static_cast<char>(hex_value);
        i += 2;
        continue;
      }
    }

    decoded += (uri[i] == '+' ? ' ' : uri[i]);
  }

  return decoded;
}

url::url(const url &other) {
  this->base = other.get_base_domain();
  this->scheme = other.get_scheme();
  for (std::string path : other.paths) {
    this->paths.push_back(path);
  }
}

url::url(const std::string &url_string) {
  size_t url_len = url_string.length();
  size_t scheme_end_idx = url_string.find_first_of(':');
  size_t base_domain_end_idx =
      url_string.substr(scheme_end_idx + 3).find_first_of('/');
  if (base_domain_end_idx == std::string::npos) {
    base_domain_end_idx = url_len;
  }
  this->scheme = url_string.substr(0, scheme_end_idx);
  this->base = url_string.substr(scheme_end_idx + 3, base_domain_end_idx);
  if (base_domain_end_idx != url_len) {
    size_t start, end = url_len,
                  scheme_slash_idx = this->get_scheme().length() + 2;
    std::string path_str = url_string;
    while ((start = path_str.find_last_of('/')) > scheme_slash_idx) {
      std::string sub_path = path_str.substr(start + 1, end);
      this->paths.push_back(sub_path);
      end = start;
      path_str = path_str.substr(0, end);
    }
  }
}

void url::set_path(std::string path) {
  if (path.front() != '/') {
    throw std::invalid_argument("`path` must begin with a leading slash");
  }
  this->paths.clear();

  size_t start, end = path.length() - 1;
  while (end > 0) {
    start = path.find_last_of('/');
    this->paths.push_back(path.substr(start + 1, end));
    end = start;
    path = path.substr(0, end);
  }
}

std::string url::get_last_subpath() { return std::string(this->paths.front()); }

std::string url::get_base_domain() const { return this->base; }

std::string url::get_scheme() const { return this->scheme; }

std::string url::get_origin() const {
  return this->scheme + "://" + this->get_base_domain();
}

std::string url::to_string() const {
  std::string combined_path;
  for (size_t i = this->paths.size(); i > 0; --i) {
    combined_path += '/' + this->paths[i - 1];
  }

  return this->get_origin() + combined_path;
}