// Copyright (c) 2013, Cloudera, inc.
#ifndef KUDU_CLIENT_PARTIAL_ROW_H
#define KUDU_CLIENT_PARTIAL_ROW_H

#include <gtest/gtest.h>
#include <string>

#include "common/types.h"
#include "gutil/macros.h"
#include "util/slice.h"

namespace kudu {

class Schema;
class Arena;

class PartialRowsPB;

namespace client {

// A row which may only contain values for a subset of the columns.
// This type contains a normal contiguous row, plus a bitfield indicating
// which columns have been set. Additionally, this type may optionally own
// copies of indirect data (eg STRING values).
class PartialRow {
 public:
  // The given Schema object must remain valid for the lifetime of this
  // row.
  explicit PartialRow(const Schema* schema);
  virtual ~PartialRow();

  Status SetInt8(const Slice& col_name, int8_t val);
  Status SetInt16(const Slice& col_name, int16_t val);
  Status SetInt32(const Slice& col_name, int32_t val);
  Status SetInt64(const Slice& col_name, int64_t val);

  Status SetUInt8(const Slice& col_name, uint8_t val);
  Status SetUInt16(const Slice& col_name, uint16_t val);
  Status SetUInt32(const Slice& col_name, uint32_t val);
  Status SetUInt64(const Slice& col_name, uint64_t val);

  // Copies 'val' immediately.
  Status SetStringCopy(const Slice& col_name, const Slice& val);

  // Set the given column to NULL. This will only succeed on nullable
  // columns. Use Unset(...) to restore a column to its default.
  Status SetNull(const Slice& col_name);

  // Unsets the given column. Note that this is different from setting
  // it to NULL.
  Status Unset(const Slice& col_name);

  // Return true if all of the key columns have been specified
  // for this mutation.
  bool IsKeySet() const;

  // Return true if the given column has been specified.
  bool IsColumnSet(int col_idx) const;

  std::string ToString() const;

  // Append this partial row to the given protobuf.
  void AppendToPB(PartialRowsPB* pb) const;

  // Parse this partial row out of the given protobuf.
  // 'offset' is the offset within the 'rows' field at which
  // to begin parsing.
  //
  // NOTE: any string fields in this PartialRow will continue to reference
  // the protobuf data, so the protobuf must remain valid.
  Status CopyFromPB(const PartialRowsPB& pb, int offset);

 private:
  template<DataType TYPE>
  Status Set(const Slice& col_name,
             const typename DataTypeTraits<TYPE>::cpp_type& val,
             bool owned = false);

  // If the given column is a string whose memory is owned by this instance,
  // deallocates the value.
  // NOTE: Does not mutate the isset bitmap.
  // REQUIRES: col_idx must be a string column.
  void DeallocateStringIfSet(int col_idx);

  // Deallocate any strings whose memory is managed by this object.
  void DeallocateOwnedStrings();

  const Schema* schema_;

  // 1-bit set for any field which has been explicitly set. This is distinct
  // from NULL -- an "unset" field will take the server-side default on insert,
  // whereas a field explicitly set to NULL will override the default.
  uint8_t* isset_bitmap_;

  // 1-bit set for any strings whose memory is managed by this instance.
  // These strings need to be deallocated whenever the value is reset,
  // or when the instance is destructed.
  uint8_t* owned_strings_bitmap_;

  uint8_t* row_data_;

  DISALLOW_COPY_AND_ASSIGN(PartialRow);
};

} // namespace client
} // namespace kudu
#endif /* KUDU_CLIENT_PARTIAL_ROW_H */
