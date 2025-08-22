#ifndef CURL_HANDLER_H
#define CURL_HANDLER_H
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

class base_writer {
public:
  virtual ~base_writer() = default;
  virtual size_t write_cb(void *contents, size_t size, size_t nmemb) = 0;
};

class html_writer : public base_writer {
private:
  std::string html;

public:
  html_writer();
  const std::string &get_html() const;
  size_t write_cb(void *contents, size_t size, size_t nmemb);
};

class file_writer : public base_writer {
private:
  std::ofstream file;
  size_t total_size;

public:
  file_writer(const std::string &file_name);
  size_t write_cb(void *contents, size_t size, size_t nmemb);
  bool is_open();
  void close();
};

size_t write_handler(void *contents, size_t size, size_t nmemb, void *userp);

#endif