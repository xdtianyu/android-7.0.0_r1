// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/url_utils.h>

#include <algorithm>

namespace {
// Given a URL string, determine where the query string starts and ends.
// URLs have schema, domain and path (along with possible user name, password
// and port number which are of no interest for us here) which could optionally
// have a query string that is separated from the path by '?'. Finally, the URL
// could also have a '#'-separated URL fragment which is usually used by the
// browser as a bookmark element. So, for example:
//    http://server.com/path/to/object?k=v&foo=bar#fragment
// Here:
//    http://server.com/path/to/object - is the URL of the object,
//    ?k=v&foo=bar                     - URL query string
//    #fragment                        - URL fragment string
// If |exclude_fragment| is true, the function returns the start character and
// the length of the query string alone. If it is false, the query string length
// will include both the query string and the fragment.
bool GetQueryStringPos(const std::string& url,
                       bool exclude_fragment,
                       size_t* query_pos,
                       size_t* query_len) {
  size_t query_start = url.find_first_of("?#");
  if (query_start == std::string::npos) {
    *query_pos = url.size();
    if (query_len)
      *query_len = 0;
    return false;
  }

  *query_pos = query_start;
  if (query_len) {
    size_t query_end = url.size();

    if (exclude_fragment) {
      if (url[query_start] == '?') {
        size_t pos_fragment = url.find('#', query_start);
        if (pos_fragment != std::string::npos)
          query_end = pos_fragment;
      } else {
        query_end = query_start;
      }
    }
    *query_len = query_end - query_start;
  }
  return true;
}
}  // anonymous namespace

namespace brillo {

std::string url::TrimOffQueryString(std::string* url) {
  size_t query_pos;
  if (!GetQueryStringPos(*url, false, &query_pos, nullptr))
    return std::string();
  std::string query_string = url->substr(query_pos);
  url->resize(query_pos);
  return query_string;
}

std::string url::Combine(const std::string& url, const std::string& subpath) {
  return CombineMultiple(url, {subpath});
}

std::string url::CombineMultiple(const std::string& url,
                                 const std::vector<std::string>& parts) {
  std::string result = url;
  if (!parts.empty()) {
    std::string query_string = TrimOffQueryString(&result);
    for (const auto& part : parts) {
      if (!part.empty()) {
        if (!result.empty() && result.back() != '/')
          result += '/';
        size_t non_slash_pos = part.find_first_not_of('/');
        if (non_slash_pos != std::string::npos)
          result += part.substr(non_slash_pos);
      }
    }
    result += query_string;
  }
  return result;
}

std::string url::GetQueryString(const std::string& url, bool remove_fragment) {
  std::string query_string;
  size_t query_pos, query_len;
  if (GetQueryStringPos(url, remove_fragment, &query_pos, &query_len)) {
    query_string = url.substr(query_pos, query_len);
  }
  return query_string;
}

data_encoding::WebParamList url::GetQueryStringParameters(
    const std::string& url) {
  // Extract the query string and remove the leading '?'.
  std::string query_string = GetQueryString(url, true);
  if (!query_string.empty() && query_string.front() == '?')
    query_string.erase(query_string.begin());
  return data_encoding::WebParamsDecode(query_string);
}

std::string url::GetQueryStringValue(const std::string& url,
                                     const std::string& name) {
  return GetQueryStringValue(GetQueryStringParameters(url), name);
}

std::string url::GetQueryStringValue(const data_encoding::WebParamList& params,
                                     const std::string& name) {
  for (const auto& pair : params) {
    if (name.compare(pair.first) == 0)
      return pair.second;
  }
  return std::string();
}

std::string url::RemoveQueryString(const std::string& url,
                                   bool remove_fragment_too) {
  size_t query_pos, query_len;
  if (!GetQueryStringPos(url, !remove_fragment_too, &query_pos, &query_len))
    return url;
  std::string result = url.substr(0, query_pos);
  size_t fragment_pos = query_pos + query_len;
  if (fragment_pos < url.size()) {
    result += url.substr(fragment_pos);
  }
  return result;
}

std::string url::AppendQueryParam(const std::string& url,
                                  const std::string& name,
                                  const std::string& value) {
  return AppendQueryParams(url, {{name, value}});
}

std::string url::AppendQueryParams(const std::string& url,
                                   const data_encoding::WebParamList& params) {
  if (params.empty())
    return url;
  size_t query_pos, query_len;
  GetQueryStringPos(url, true, &query_pos, &query_len);
  size_t fragment_pos = query_pos + query_len;
  std::string result = url.substr(0, fragment_pos);
  if (query_len == 0) {
    result += '?';
  } else if (query_len > 1) {
    result += '&';
  }
  result += data_encoding::WebParamsEncode(params);
  if (fragment_pos < url.size()) {
    result += url.substr(fragment_pos);
  }
  return result;
}

bool url::HasQueryString(const std::string& url) {
  size_t query_pos, query_len;
  GetQueryStringPos(url, true, &query_pos, &query_len);
  return (query_len > 0);
}

}  // namespace brillo
