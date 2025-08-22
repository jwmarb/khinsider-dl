#include "curl_handler.h"

html_writer::html_writer() : html() {};

const std::string &html_writer::get_html() const { return this->html; };
size_t html_writer::write_cb(void *contents, size_t size, size_t nmemb) {
  size_t actual_size = size * nmemb;
  this->html.append(static_cast<char *>(contents), actual_size);
  return actual_size;
}

file_writer::file_writer(const std::string &file_name)
    : file(file_name, std::ios::binary) {}

size_t file_writer::write_cb(void *contents, size_t size, size_t nmemb) {
  size_t actual_size = size * nmemb;
  this->file.write(static_cast<char *>(contents), actual_size);
  this->total_size += actual_size;
  return actual_size;
}

void file_writer::close() { this->file.close(); }

bool file_writer::is_open() { return this->file.is_open(); }

size_t write_handler(void *contents, size_t size, size_t nmemb, void *userp) {
  base_writer *writer = static_cast<base_writer *>(userp);
  return writer->write_cb(contents, size, nmemb);
}

size_t base_writer::write_cb(void *contents, size_t size, size_t nmemb) {
  return 0;
}