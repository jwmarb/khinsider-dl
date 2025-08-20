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

  url b("https://test.com");
  EXPECT_EQ(b.get_last_subpath(), "");
  EXPECT_EQ(b.get_base_domain(), "test.com");
  EXPECT_EQ(b.get_scheme(), "https");
  EXPECT_EQ(b.get_origin(), "https://test.com");
}

TEST(URL, mutate_components) {
  url a("https://abc.xyz");
  a.set_path("/helloworld");

  EXPECT_EQ(a.get_last_subpath(), "helloworld");
  EXPECT_EQ(a.to_string(), "https://abc.xyz/helloworld");

  a = url("https://abc.xyz/existing/path");
  a.set_path("/something/else");

  EXPECT_EQ(a.get_last_subpath(), "else");
  EXPECT_EQ(a.to_string(), "https://abc.xyz/something/else");
}