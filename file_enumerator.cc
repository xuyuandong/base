#include "base/file_enumerator.h"

#include <fnmatch.h>
#include "base/logging.h"
#include "base/string_util.h"
#include "base/file.h"

using namespace std;
using namespace base;

namespace {

bool IsDirectory(const FTSENT* file) {
  switch (file->fts_info) {
    case FTS_D:
    case FTS_DC:
    case FTS_DNR:
    case FTS_DOT:
    case FTS_DP:
      return true;
    default:
      return false;
  }
}

int CompareFiles(const FTSENT** a, const FTSENT** b) {
  // Order lexicographically with directories before other files.
  const bool a_is_dir = IsDirectory(*a);
  const bool b_is_dir = IsDirectory(*b);
  if (a_is_dir != b_is_dir)
    return a_is_dir ? -1 : 1;

  string name_a = (*a)->fts_name;
  string name_b = (*b)->fts_name;
  return name_a > name_b;
}

}  // namespace

namespace file {

FileEnumerator::FileEnumerator(const string &root_path,
                               bool recursive,
                               FileEnumerator::FILE_TYPE file_type)
    : recursive_(recursive),
      file_type_(file_type),
      is_in_find_op_(false),
      fts_(NULL) {
  pending_paths_.push(root_path);
}

FileEnumerator::FileEnumerator(const string &root_path,
                               bool recursive,
                               FileEnumerator::FILE_TYPE file_type,
                               const string &pattern)
    : recursive_(recursive),
      file_type_(file_type),
      is_in_find_op_(false),
      fts_(NULL) {
  pattern_ = File::JoinPath(root_path, pattern);
  pending_paths_.push(root_path);
}

FileEnumerator::~FileEnumerator() {
  if (fts_)
    fts_close(fts_);
}

void FileEnumerator::GetFindInfo(FindInfo* info) {
  DCHECK(info);

  if (!is_in_find_op_)
    return;

  memcpy(&(info->stat), fts_ent_->fts_statp, sizeof(info->stat));
  info->filename.assign(fts_ent_->fts_name);
}

// As it stands, this method calls itself recursively when the next item of
// the fts enumeration doesn't match (type, pattern, etc.).  In the case of
// large directories with many files this can be quite deep.
// TODO(erikkay) - get rid of this recursive pattern
const string FileEnumerator::Next() {
  if (!is_in_find_op_) {
    if (pending_paths_.empty())
      return "";

    // The last find FindFirstFile operation is done, prepare a new one.
    root_path_ = pending_paths_.top();
    if (*root_path_.rbegin() == '/')
      root_path_.erase(root_path_.end() - 1);
    pending_paths_.pop();

    // Start a new find operation.
    int ftsflags = FTS_LOGICAL | FTS_SEEDOT;
    char top_dir[PATH_MAX];
    strncpy(top_dir, root_path_.c_str(), arraysize(top_dir));
    char* dir_list[2] = { top_dir, NULL };
    fts_ = fts_open(dir_list, ftsflags, CompareFiles);
    if (!fts_)
      return Next();
    is_in_find_op_ = true;
  }

  fts_ent_ = fts_read(fts_);
  if (fts_ent_ == NULL) {
    fts_close(fts_);
    fts_ = NULL;
    is_in_find_op_ = false;
    return Next();
  }

  // Level 0 is the top, which is always skipped.
  if (fts_ent_->fts_level == 0)
    return Next();

  // Patterns are only matched on the items in the top-most directory.
  // (see Windows implementation)
  if (fts_ent_->fts_level == 1 && pattern_.size() > 0) {
    if (fnmatch(pattern_.c_str(), fts_ent_->fts_path, 0) != 0) {
      if (fts_ent_->fts_info == FTS_D)
        fts_set(fts_, fts_ent_, FTS_SKIP);
      return Next();
    }
  }

  string cur_file(fts_ent_->fts_path);
  if (ShouldSkip(cur_file))
    return Next();

  if (fts_ent_->fts_info == FTS_D) {
    // If not recursive, then prune children.
    if (!recursive_)
      fts_set(fts_, fts_ent_, FTS_SKIP);
    return (file_type_ & FileEnumerator::DIRECTORIES) ? cur_file : Next();
  } else if (fts_ent_->fts_info == FTS_F) {
    return (file_type_ & FileEnumerator::FILES) ? cur_file : Next();
  } else if (fts_ent_->fts_info == FTS_DOT) {
    if ((file_type_ & FileEnumerator::DIRECTORIES) && cur_file == "..") {
      return cur_file;
    }
    return Next();
  }
  // TODO(erikkay) - verify that the other fts_info types aren't interesting
  return Next();
}

bool FileEnumerator::ShouldSkip(const string &path) {
  return path == "." || path == "..";
}

void MatchFile(const string &patterns, vector<string> *files) {
  vector<string> pattern_vec;
  SplitString(patterns, ',', &pattern_vec);

  files->clear();
  for (int i = 0; i < pattern_vec.size(); ++i) {
    string path;
    TrimWhitespaceASCII(pattern_vec[i], TRIM_ALL, &path);
    if (path.empty()) continue;
    FileEnumerator enumerator(File::DirName(path),
                              false,
                              FileEnumerator::FILES,
                              File::BaseName(path));
    string file = enumerator.Next();
    while (!file.empty()) {
      files->push_back(file);
      file = enumerator.Next();
    }
  }
}

}  // namespace file
