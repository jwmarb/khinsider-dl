#include "curl_handler.h"
#include "url.h"
#include <cstring>
#include <curl/curl.h>
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <gumbo.h>
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#define HREF_ATTR "href"
#define ID_ATTR "id"
#define H2_ALBUM_TITLE_OCCURRING 4

// https://downloads.khinsider.com/game-soundtracks/album/robotics-notes-original-soundtrack

std::string get_album_title(GumboNode *node) {
  std::stack<GumboNode *> s;
  s.push(node);
  u_int8_t h2_occurrences = 0;
  while (!s.empty()) {
    GumboNode *n = s.top();
    s.pop();

    if (n->type != GUMBO_NODE_ELEMENT) {
      continue;
    }

    GumboElement *el = &n->v.element;
    GumboVector *children = &el->children;

    if (n->v.element.tag == GUMBO_TAG_H2) {
      ++h2_occurrences;
      if (h2_occurrences == H2_ALBUM_TITLE_OCCURRING) {
        GumboNode *child = static_cast<GumboNode *>(children->data[0]);
        return child->v.text.text;
      }
    }

    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      s.push(child);
    }
  }

  return "";
}

GumboNode *get_songlist(GumboNode *node) {
  std::stack<GumboNode *> s;
  s.push(node);
  while (!s.empty()) {
    GumboNode *n = s.top();
    s.pop();
    if (n->type != GUMBO_NODE_ELEMENT) {
      continue;
    }

    if (n->v.element.tag == GUMBO_TAG_TABLE) {
      GumboAttribute *id_attr =
          gumbo_get_attribute(&n->v.element.attributes, ID_ATTR);
      if (id_attr) {
        // std::cout << "table id= " << id_attr->value << std::endl;
        return n;
      }
    }

    GumboElement *el = &n->v.element;
    GumboVector *children = &el->children;

    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      s.push(child);
    }
  }

  return nullptr;
}

void get_songlinks(GumboNode *songlist,
                   std::unordered_set<std::string> &songlinks) {

  std::stack<GumboNode *> s;
  s.push(songlist);

  while (!s.empty()) {
    GumboNode *n = s.top();
    s.pop();

    GumboElement *el = &n->v.element;
    GumboVector *children = &el->children;

    if (n->type != GUMBO_NODE_ELEMENT) {
      continue;
    }

    if (n->v.element.tag == GUMBO_TAG_A) {
      GumboAttribute *id_attr =
          gumbo_get_attribute(&n->v.element.attributes, HREF_ATTR);
      if (id_attr) {
        std::string value(id_attr->value);
        songlinks.insert(value);
      }
    }

    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      s.push(child);
    }
  }
}

std::unique_ptr<url> get_anchor_file_url(GumboNode *root,
                                         std::vector<std::string> &file_exts) {
  std::stack<GumboNode *> s;
  std::unordered_map<std::string, std::unique_ptr<url>> file_urls;
  s.push(root);

  while (!s.empty()) {
    GumboNode *n = s.top();
    s.pop();

    GumboElement *el = &n->v.element;
    GumboVector *children = &el->children;

    if (n->type != GUMBO_NODE_ELEMENT) {
      continue;
    }

    if (n->v.element.tag == GUMBO_TAG_A) {
      GumboAttribute *href_attr =
          gumbo_get_attribute(&n->v.element.attributes, HREF_ATTR);
      if (href_attr != nullptr) {
        size_t str_n = strlen(href_attr->value);
        for (std::string &file_ext : file_exts) {
          bool is_target_file_ext =
              strcmp(href_attr->value + str_n - file_ext.length(),
                     file_ext.c_str()) == 0;
          if (is_target_file_ext) {
            file_urls.emplace(file_ext,
                              std::make_unique<url>(url(href_attr->value)));
            break;
          }
        }
      }
    }

    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      s.push(child);
    }
  }
  for (std::string &file_ext : file_exts) {
    auto it = file_urls.find(file_ext);
    if (it != file_urls.end() && it->second) {
      return std::move(it->second);
    }
  }

  return nullptr;
}

std::string fetch_html(url &link) {
  CURL *curl = curl_easy_init();
  std::string link_str = link.to_string();
  if (curl != nullptr) {
    CURLcode res;
    html_writer writer;

    curl_easy_setopt(curl, CURLOPT_URL, link_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_handler);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK) {
      return writer.get_html();
    }

    throw std::runtime_error(std::string(curl_easy_strerror(res)));
  }

  throw std::runtime_error("Failed to initialize cURL");
}

void download_file(const std::string &file_name,
                   const std::unique_ptr<url> &dl) {
  CURL *curl = curl_easy_init();

  if (curl != nullptr) {
    std::string dl_link = dl->to_string();

    // Initially fetch the content headers to get the total download size
    curl_easy_setopt(curl, CURLOPT_URL, dl_link.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      curl_easy_cleanup(curl);
      throw std::runtime_error("Failed to fetch download headers for cURL");
    }

    curl_off_t content_length = -1;
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                            &content_length);

    if (res != CURLE_OK) {
      curl_easy_cleanup(curl);
      throw std::runtime_error(
          "Failed to get download size from cURL response");
    }

    file_writer writer(file_name, content_length);

    if (!writer.is_open()) {
      curl_easy_cleanup(curl);
      throw std::runtime_error("Failed to open file: " + file_name);
    }

    // change options for actual download
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_handler);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &writer);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_handler);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writer);

    res = curl_easy_perform(curl);

    writer.close();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      throw std::runtime_error("Failed to run cURL");
    }
  }
}

void scrape_song_dl(url &song_dl, std::string &directory,
                    std::vector<std::string> &file_exts) {
  CURL *curl = curl_easy_init();
  std::string html = fetch_html(song_dl);
  GumboOutput *output = gumbo_parse(html.c_str());
  std::unique_ptr<url> file_url = get_anchor_file_url(output->root, file_exts);
  std::string file_name = file_url->get_last_subpath();
  std::filesystem::path resolved_path =
      std::filesystem::path(directory) / url::decode_uri(file_name);
  download_file(resolved_path.string(), file_url);
}

int main(int argc, char *argv[]) {

  cxxopts::Options options("khinsider-dl",
                           "A tool for bulk downloading songs from khinsider");

  options.add_options()("input", "Download page for the album/soundtrack",
                        cxxopts::value<std::string>())(
      "d,directory", "Output directory",
      cxxopts::value<std::string>()->default_value(""))(
      "t,type",
      "Comma-separated list of types of file extensions to download with "
      "priority. Left-most item will be downloaded first, and if not found, "
      "the program will attempt to download the next file extension.",
      cxxopts::value<std::vector<std::string>>()->default_value("flac,mp3"))(
      "h,help", "Print usage");

  options.parse_positional({"input"});

  cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help") || !result.count("input")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  CURL *curl = curl_easy_init();
  url download_page(result["input"].as<std::string>());
  std::string html = fetch_html(download_page);
  std::unordered_set<std::string> songlinks;
  GumboOutput *output = gumbo_parse(html.c_str());
  GumboNode *songlist = get_songlist(output->root);
  get_songlinks(songlist, songlinks);
  url song_dl(download_page);
  std::cout << get_album_title(output->root) << std::endl;
  // make the directory if it doesnt exist
  std::string directory = result["directory"].as<std::string>();
  if (directory.length() == 0) {
    std::filesystem::path resolved_path =
        std::filesystem::current_path() / get_album_title(output->root);
    directory = resolved_path.string();
  }
  if (!std::filesystem::exists(directory)) {
    bool is_success = std::filesystem::create_directory(directory);
    if (!is_success) {
      throw std::runtime_error("Failed to create " + directory);
    }
  }
  // but if it exists, make sure it is a folder
  else if (!std::filesystem::is_directory(directory)) {
    throw std::runtime_error(directory + " must be a folder");
  }
  size_t i = 1, n = songlinks.size();
  std::vector<std::string> file_exts =
      result["type"].as<std::vector<std::string>>();

  std::cout << "Downloading " << download_page.to_string() << std::endl;

  for (std::unordered_set<std::string>::iterator el = songlinks.begin();
       el != songlinks.end(); ++el) {
    song_dl.set_path(*el);

    scrape_song_dl(song_dl, directory, file_exts);

    ++i;
  }

  std::cout << "All files downloaded to " << "\"" << directory << "\""
            << std::endl;
  return 0;
}