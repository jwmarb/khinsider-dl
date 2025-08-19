#ifndef URL_H
#define URL_H

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class url {
private:
  std::string base;
  std::string scheme;
  std::vector<std::string> paths;

public:
  static std::string decode_uri(std::string &uri);
  std::string get_last_subpath();
  url(const std::string &url_string);
  url(const url &other);
  void set_path(std::string path);
  std::string get_base_domain() const;
  std::string get_scheme() const;
  std::string get_origin() const;
  std::string to_string() const;
};

#endif