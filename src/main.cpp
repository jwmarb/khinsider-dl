#include "url.h"
#include <cstring>
#include <curl/curl.h>
#include <fstream>
#include <gumbo.h>
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#define FLAC ".flac"
#define HREF_ATTR "href"
#define ID_ATTR "id"

size_t data_stream_recv(void *contents, size_t size, size_t nmemb,
                        void *userp) {
  size_t realsize = size * nmemb;
  std::string *str = static_cast<std::string *>(userp);
  str->append(static_cast<char *>(contents), realsize);
  return realsize;
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

std::unique_ptr<url> get_anchor_flac_url(GumboNode *root) {
  std::stack<GumboNode *> s;
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
        bool is_flac =
            strcmp(href_attr->value + str_n - strlen(FLAC), FLAC) == 0;
        if (is_flac) {
          return std::make_unique<url>(url(href_attr->value));
        }
      }
    }

    for (unsigned int i = 0; i < children->length; ++i) {
      GumboNode *child = static_cast<GumboNode *>(children->data[i]);
      s.push(child);
    }
  }
  return nullptr;
}

std::string fetch_html(url &link) {
  CURL *curl = curl_easy_init();
  std::string link_str = link.to_string();
  if (curl != nullptr) {
    CURLcode res;
    std::string html;
    curl_easy_setopt(curl, CURLOPT_URL, link_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_stream_recv);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK) {
      return html;
    }

    throw std::runtime_error(std::string(curl_easy_strerror(res)));
  }

  throw std::runtime_error("Failed to initialize cURL");
}

size_t file_callback(void *contents, size_t size, size_t nmemb,
                     std::ofstream *file) {
  size_t total_size = size * nmemb;
  file->write(static_cast<char *>(contents), total_size);
  return total_size;
}

void download_file(const std::string &file_name,
                   const std::unique_ptr<url> &dl) {
  CURL *curl = curl_easy_init();

  if (curl != nullptr) {
    std::ofstream file(file_name, std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + file_name);
    }

    std::string dl_link = dl->to_string();
    curl_easy_setopt(curl, CURLOPT_URL, dl_link.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                     1); // follow redirects, just in case

    CURLcode res = curl_easy_perform(curl);

    file.close();
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      throw std::runtime_error("Failed to run cURL");
    }
  }
}

void scrape_song_dl(url &song_dl) {
  CURL *curl = curl_easy_init();
  std::string html = fetch_html(song_dl);
  GumboOutput *output = gumbo_parse(html.c_str());
  std::unique_ptr<url> flac_url = get_anchor_flac_url(output->root);
  std::string file_name = flac_url->get_last_subpath();
  download_file(url::decode_uri(file_name), flac_url);
}

int main(int argc, char *argv[]) {
  CURL *curl = curl_easy_init();
  url download_page("https://downloads.khinsider.com/game-soundtracks/album/"
                    "robotics-notes-original-soundtrack");
  std::string html = fetch_html(download_page);
  std::unordered_set<std::string> songlinks;
  GumboOutput *output = gumbo_parse(html.c_str());
  GumboNode *songlist = get_songlist(output->root);
  get_songlinks(songlist, songlinks);
  url song_dl(download_page);
  size_t i = 1, n = songlinks.size();
  for (std::unordered_set<std::string>::iterator el = songlinks.begin();
       el != songlinks.end(); ++el) {
    song_dl.set_path(*el);

    std::cout << "Downloading " << i << " of " << n << std::endl;
    scrape_song_dl(song_dl);

    ++i;
  }
  return 0;
}