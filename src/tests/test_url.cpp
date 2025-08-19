#include "../url.h"
#include <gtest/gtest.h>

TEST(URL, init) {
  url a("https://example.com/");
  url b("https://example.com");
  url c("http://twitch.tv/user");
  EXPECT_EQ(a.to_string(), "https://example.com/");
  EXPECT_EQ(b.to_string(), "https://example.com");
  EXPECT_EQ(c.to_string(), "http://twitch.tv/user");
}

TEST(URL, correct_components) {
  url a("http://hello.wa.bc/x/y/z");
  EXPECT_EQ(a.get_base_domain(), "hello.wa.bc");
  EXPECT_EQ(a.get_scheme(), "http");
  EXPECT_EQ(a.get_origin(), "http://hello.wa.bc");
  EXPECT_EQ(a.get_last_subpath(), "z");
}