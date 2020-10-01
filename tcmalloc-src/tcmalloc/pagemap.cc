// Copyright 2019 The TCMalloc Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tcmalloc/pagemap.h"

#include <sys/mman.h>

#include "tcmalloc/common.h"
#include "tcmalloc/span.h"
#include "tcmalloc/static_vars.h"

namespace tcmalloc {

void PageMap::RegisterSizeClass(Span* span, size_t sc) {
  ASSERT(span->location() == Span::IN_USE);
  const PageID first = span->first_page();
  const PageID last = span->last_page();
  ASSERT(GetDescriptor(first) == span);
  for (PageID p = first; p <= last; ++p) {
    map_.set_with_sizeclass(p, span, sc);
  }
}

void PageMap::UnregisterSizeClass(Span* span) {
  ASSERT(span->location() == Span::IN_USE);
  const PageID first = span->first_page();
  const PageID last = span->last_page();
  ASSERT(GetDescriptor(first) == span);
  for (PageID p = first; p <= last; ++p) {
    map_.clear_sizeclass(p);
  }
}

void PageMap::MapRootWithSmallPages() {
  constexpr size_t kHugePageMask = ~(kHugePageSize - 1);
  uintptr_t begin = reinterpret_cast<uintptr_t>(map_.RootAddress());
  // Round begin up to the nearest hugepage, this avoids causing memory before
  // the start of the pagemap to become mapped onto small pages.
  uintptr_t rbegin = (begin + kHugePageSize) & kHugePageMask;
  size_t length = map_.RootSize();
  // Round end down to the nearest hugepage, this avoids causing memory after
  // the end of the pagemap becoming mapped onto small pages.
  size_t rend = (begin + length) & kHugePageMask;
  // Since we have rounded the start up, and the end down, we also want to
  // confirm that there is something left between them for us to modify.
  // For small but slow, the root pagemap is less than a hugepage in size,
  // so we will not end up forcing it to be small pages.
  if (rend > rbegin) {
    size_t rlength = rend - rbegin;
    madvise(reinterpret_cast<void*>(rbegin), rlength, MADV_NOHUGEPAGE);
  }
}

void* MetaDataAlloc(size_t bytes) ABSL_EXCLUSIVE_LOCKS_REQUIRED(pageheap_lock) {
  return Static::arena()->Alloc(bytes);
}

}  // namespace tcmalloc
