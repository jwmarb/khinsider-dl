#ifndef CURL_HANDLER_H
#define CURL_HANDLER_H
#include <barkeep/barkeep.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

class base_writer {
public:
  virtual ~base_writer() = default;
  virtual size_t write_cb(void *contents, size_t size, size_t nmemb) = 0;
  virtual int progress_cb(curl_off_t dl_total, curl_off_t dl_now,
                          curl_off_t ul_total, curl_off_t ul_now) = 0;
};

class html_writer : public base_writer {
private:
  std::string html;

public:
  html_writer();
  const std::string &get_html() const;
  size_t write_cb(void *contents, size_t size, size_t nmemb);
  int progress_cb(curl_off_t dl_total, curl_off_t dl_now, curl_off_t ul_total,
                  curl_off_t ul_now);
};

class file_writer : public base_writer {
private:
  std::ofstream file;
  std::string file_name;
  std::shared_ptr<barkeep::ProgressBarDisplay<double>> bk;
  double total_size;

public:
  file_writer(const std::string &file_name,
              const curl_off_t total_content_length);
  size_t write_cb(void *contents, size_t size, size_t nmemb);
  int progress_cb(curl_off_t dl_total, curl_off_t dl_now, curl_off_t ul_total,
                  curl_off_t ul_now);
  bool is_open();
  void close();
};

size_t write_handler(void *contents, size_t size, size_t nmemb, void *userp);

int progress_handler(void *clientp, curl_off_t dl_total, curl_off_t dl_now,
                     curl_off_t ul_total, curl_off_t ul_now);

#endif