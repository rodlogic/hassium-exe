// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/nw/src/net/app_protocol_handler.h"

#include "base/logging.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_dir_job.h"
#include "net/url_request/url_request_file_job.h"
#include "net/url_request/url_request_http_job.h"

namespace net {

AppProtocolHandler::AppProtocolHandler(const base::FilePath& root, const GURL& base_url)
  :root_path_(root), base_url_(base_url)
{
}

URLRequestJob* AppProtocolHandler::MaybeCreateJob(
    URLRequest* request, NetworkDelegate* network_delegate) const {
  GURL url(request->url());

  if( !base_url_.is_empty() ){
	  url_canon::Replacements<char> repl;
	  repl.SetScheme(base_url_.scheme().data(), url_parse::Component(0, base_url_.scheme().length()));
	  repl.SetHost(base_url_.host().data(), url_parse::Component(0, base_url_.host().length()));
	  repl.SetPort(base_url_.port().data(), url_parse::Component(0, base_url_.port().length()));
	  url = url.ReplaceComponents(repl);

	  return URLRequestHttpJob::Factory(request, network_delegate, base_url_.scheme());
  }

  base::FilePath file_path;
  url_canon::Replacements<char> replacements;
  replacements.SetScheme("file", url_parse::Component(0, 4));
  replacements.ClearHost();
  url = url.ReplaceComponents(replacements);
  
  const bool is_file = FileURLToFilePath(url, &file_path);

  file_path = root_path_.Append(file_path);
  // Check file access permissions.
  if (!network_delegate ||
      !network_delegate->CanAccessFile(*request, file_path)) {
    return new URLRequestErrorJob(request, network_delegate, ERR_ACCESS_DENIED);
  }

  // We need to decide whether to create URLRequestFileJob for file access or
  // URLRequestFileDirJob for directory access. To avoid accessing the
  // filesystem, we only look at the path string here.
  // The code in the URLRequestFileJob::Start() method discovers that a path,
  // which doesn't end with a slash, should really be treated as a directory,
  // and it then redirects to the URLRequestFileDirJob.
  if (is_file &&
      file_path.EndsWithSeparator() &&
      file_path.IsAbsolute()) {
    return new URLRequestFileDirJob(request, network_delegate, file_path);
  }

  // Use a regular file request job for all non-directories (including invalid
  // file names).
  return new URLRequestFileJob(request, network_delegate, file_path);
}

bool AppProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace net
