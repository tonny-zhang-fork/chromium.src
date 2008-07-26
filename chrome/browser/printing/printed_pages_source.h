// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef CHROME_BROWSER_PRINTING_PRINTED_PAGES_SOURCE_H__
#define CHROME_BROWSER_PRINTING_PRINTED_PAGES_SOURCE_H__

#include <string>

class GURL;
class MessageLoop;

namespace printing {

class PrintedDocument;

// Source of printed pages.
class PrintedPagesSource {
 public:
  // Renders a printed page. It is not necessary to be synchronous. It must call
  // document->SetPage() once the source is done rendering the requested page.
  virtual void RenderOnePrintedPage(PrintedDocument* document,
                                    int page_number) = 0;

  // Returns the document title.
  virtual std::wstring RenderSourceName() = 0;

  // Returns the URL's source of the document if applicable.
  virtual GURL RenderSourceUrl() = 0;
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINTED_PAGES_SOURCE_H__
