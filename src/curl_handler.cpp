#include "curl_handler.h"
#include <barkeep/barkeep.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

#define KB 1024.0
#define MB (KB * 1024.0)

html_writer::html_writer() : html() {};

const std::string &html_writer::get_html() const { return this->html; };
size_t html_writer::write_cb(void *contents, size_t size, size_t nmemb) {
  size_t actual_size = size * nmemb;
  this->html.append(static_cast<char *>(contents), actual_size);
  return actual_size;
}

file_writer::file_writer(const std::string &file_path,
                         const curl_off_t total_content_length)
    : file(file_path, std::ios::binary) {
  this->total_size = 0;
  std::filesystem::path path(file_path);
  this->file_name = path.filename().string();
  this->bk = barkeep::ProgressBar(
      &this->total_size,
      {.total = total_content_length / MB,
       .format = this->file_name +
                 "\n\t{percent:.2f}% "
                 "{bar} {value:.2f}/{total:.2f}MB ({speed:.1f} MB/s) ",
       .speed = 1.0});
}

int html_writer::progress_cb(curl_off_t dl_total, curl_off_t dl_now,
                             curl_off_t ul_total, curl_off_t ul_now) {
  return 0;
}

int file_writer::progress_cb(curl_off_t dl_total, curl_off_t dl_now,
                             curl_off_t ul_total, curl_off_t ul_now) {
  this->total_size = static_cast<double>(dl_now) / MB;
  return 0;
}

size_t file_writer::write_cb(void *contents, size_t size, size_t nmemb) {
  size_t actual_size = size * nmemb;
  this->file.write(static_cast<char *>(contents), actual_size);
  return actual_size;
}

void file_writer::close() {
  this->file.close();
  this->bk->done();
}

bool file_writer::is_open() { return this->file.is_open(); }

size_t write_handler(void *contents, size_t size, size_t nmemb, void *userp) {
  base_writer *writer = static_cast<base_writer *>(userp);
  return writer->write_cb(contents, size, nmemb);
}

int progress_handler(void *clientp, curl_off_t dl_total, curl_off_t dl_now,
                     curl_off_t ul_total, curl_off_t ul_now) {
  base_writer *writer = static_cast<base_writer *>(clientp);
  return writer->progress_cb(dl_total, dl_now, ul_total, ul_now);
}
