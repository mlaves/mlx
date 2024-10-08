// Copyright © 2023-2024 Apple Inc.

#include <numeric>

#include "mlx/allocator.h"
#include "mlx/backend/common/copy.h"
#include "mlx/backend/common/utils.h"

namespace mlx::core {

namespace {

template <typename SrcT, typename DstT>
void copy_single(const array& src, array& dst) {
  auto val = static_cast<DstT>(src.data<SrcT>()[0]);
  auto dst_ptr = dst.data<DstT>();
  for (int i = 0; i < dst.size(); ++i) {
    dst_ptr[i] = val;
  }
}

template <typename SrcT, typename DstT>
void copy_vector(const array& src, array& dst) {
  auto src_ptr = src.data<SrcT>();
  auto dst_ptr = dst.data<DstT>();
  std::copy(src_ptr, src_ptr + src.data_size(), dst_ptr);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim1(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>();
  DstT* dst_ptr = dst.data<DstT>();
  stride_t src_idx = i_offset;
  stride_t dst_idx = 0;
  for (int i = 0; i < data_shape[0]; ++i) {
    dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
    src_idx += i_strides[0];
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim1(const array& src, array& dst) {
  return copy_general_dim1<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim2(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>();
  DstT* dst_ptr = dst.data<DstT>();
  stride_t src_idx = i_offset;
  stride_t dst_idx = 0;
  for (int i = 0; i < data_shape[0]; ++i) {
    for (int j = 0; j < data_shape[1]; ++j) {
      dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
      src_idx += i_strides[1];
    }
    src_idx += i_strides[0] - i_strides[1] * data_shape[1];
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim2(const array& src, array& dst) {
  return copy_general_dim2<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim3(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>();
  DstT* dst_ptr = dst.data<DstT>();
  stride_t src_idx = i_offset;
  stride_t dst_idx = 0;
  for (int i = 0; i < data_shape[0]; ++i) {
    for (int j = 0; j < data_shape[1]; ++j) {
      for (int k = 0; k < data_shape[2]; ++k) {
        dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
        src_idx += i_strides[2];
      }
      src_idx += i_strides[1] - i_strides[2] * data_shape[2];
    }
    src_idx += i_strides[0] - i_strides[1] * data_shape[1];
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim3(const array& src, array& dst) {
  return copy_general_dim3<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim4(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>();
  DstT* dst_ptr = dst.data<DstT>();
  stride_t src_idx = i_offset;
  stride_t dst_idx = 0;
  for (int i = 0; i < data_shape[0]; ++i) {
    for (int j = 0; j < data_shape[1]; ++j) {
      for (int k = 0; k < data_shape[2]; ++k) {
        for (int ii = 0; ii < data_shape[3]; ++ii) {
          dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
          src_idx += i_strides[3];
        }
        src_idx += i_strides[2] - i_strides[3] * data_shape[3];
      }
      src_idx += i_strides[1] - i_strides[2] * data_shape[2];
    }
    src_idx += i_strides[0] - i_strides[1] * data_shape[1];
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim4(const array& src, array& dst) {
  return copy_general_dim4<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim5(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>() + i_offset;
  DstT* dst_ptr = dst.data<DstT>();

  // Pre-compute loop bounds and strides
  const int d0 = data_shape[0], d1 = data_shape[1], d2 = data_shape[2],
            d3 = data_shape[3], d4 = data_shape[4];
  const stride_t s0 = i_strides[0], s1 = i_strides[1], s2 = i_strides[2],
                 s3 = i_strides[3], s4 = i_strides[4];

  // Pre-compute stride adjustments
  const stride_t s3_adj = s3 - s4 * d4;
  const stride_t s2_adj = s2 - s3 * d3;
  const stride_t s1_adj = s1 - s2 * d2;
  const stride_t s0_adj = s0 - s1 * d1;

  stride_t src_idx = 0;
  stride_t dst_idx = 0;

  for (int i = 0; i < d0; ++i) {
    for (int j = 0; j < d1; ++j) {
      for (int k = 0; k < d2; ++k) {
        for (int l = 0; l < d3; ++l) {
          for (int m = 0; m < d4; ++m) {
            dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
            src_idx += s4;
          }
          src_idx += s3_adj;
        }
        src_idx += s2_adj;
      }
      src_idx += s1_adj;
    }
    src_idx += s0_adj;
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim5(const array& src, array& dst) {
  return copy_general_dim5<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim6(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>() + i_offset;
  DstT* dst_ptr = dst.data<DstT>();

  // Pre-compute loop bounds and strides
  const int d0 = data_shape[0], d1 = data_shape[1], d2 = data_shape[2],
            d3 = data_shape[3], d4 = data_shape[4], d5 = data_shape[5];
  const stride_t s0 = i_strides[0], s1 = i_strides[1], s2 = i_strides[2],
                 s3 = i_strides[3], s4 = i_strides[4], s5 = i_strides[5];

  // Pre-compute stride adjustments
  const stride_t s4_adj = s4 - s5 * d5;
  const stride_t s3_adj = s3 - s4 * d4;
  const stride_t s2_adj = s2 - s3 * d3;
  const stride_t s1_adj = s1 - s2 * d2;
  const stride_t s0_adj = s0 - s1 * d1;

  stride_t src_idx = 0;
  stride_t dst_idx = 0;

  for (int i = 0; i < d0; ++i) {
    for (int j = 0; j < d1; ++j) {
      for (int k = 0; k < d2; ++k) {
        for (int l = 0; l < d3; ++l) {
          for (int m = 0; m < d4; ++m) {
            for (int n = 0; n < d5; ++n) {
              dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
              src_idx += s5;
            }
            src_idx += s4_adj;
          }
          src_idx += s3_adj;
        }
        src_idx += s2_adj;
      }
      src_idx += s1_adj;
    }
    src_idx += s0_adj;
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim6(const array& src, array& dst) {
  return copy_general_dim6<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_dim7(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  const SrcT* src_ptr = src.data<SrcT>() + i_offset;
  DstT* dst_ptr = dst.data<DstT>();

  // Pre-compute loop bounds and strides
  const int d0 = data_shape[0], d1 = data_shape[1], d2 = data_shape[2],
            d3 = data_shape[3], d4 = data_shape[4], d5 = data_shape[5],
            d6 = data_shape[6];
  const stride_t s0 = i_strides[0], s1 = i_strides[1], s2 = i_strides[2],
                 s3 = i_strides[3], s4 = i_strides[4], s5 = i_strides[5],
                 s6 = i_strides[6];

  // Pre-compute stride adjustments
  const stride_t s5_adj = s5 - s6 * d6;
  const stride_t s4_adj = s4 - s5 * d5;
  const stride_t s3_adj = s3 - s4 * d4;
  const stride_t s2_adj = s2 - s3 * d3;
  const stride_t s1_adj = s1 - s2 * d2;
  const stride_t s0_adj = s0 - s1 * d1;

  stride_t src_idx = 0;
  stride_t dst_idx = 0;

  for (int i = 0; i < d0; ++i) {
    for (int j = 0; j < d1; ++j) {
      for (int k = 0; k < d2; ++k) {
        for (int l = 0; l < d3; ++l) {
          for (int m = 0; m < d4; ++m) {
            for (int n = 0; n < d5; ++n) {
              for (int p = 0; p < d6; ++p) {
                dst_ptr[dst_idx++] = static_cast<DstT>(src_ptr[src_idx]);
                src_idx += s6;
              }
              src_idx += s5_adj;
            }
            src_idx += s4_adj;
          }
          src_idx += s3_adj;
        }
        src_idx += s2_adj;
      }
      src_idx += s1_adj;
    }
    src_idx += s0_adj;
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_dim7(const array& src, array& dst) {
  return copy_general_dim7<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    int64_t i_offset) {
  auto [new_shape, new_strides] = collapse_contiguous_dims(
      data_shape, std::vector<std::vector<stride_t>>{i_strides});
  switch (new_shape.size()) {
    case 1:
      copy_general_dim1<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 2:
      copy_general_dim2<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 3:
      copy_general_dim3<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 4:
      copy_general_dim4<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 5:
      copy_general_dim5<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 6:
      copy_general_dim6<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
    case 7:
      copy_general_dim7<SrcT, DstT, stride_t>(
          src, dst, new_shape, new_strides[0], i_offset);
      return;
  }

  auto src_ptr = src.data<SrcT>() + i_offset;
  auto dst_ptr = dst.data<DstT>();
  for (size_t i = 0; i < dst.size(); ++i) {
    stride_t src_elem = elem_to_loc(i, new_shape, new_strides[0]);
    dst_ptr[i] = static_cast<DstT>(src_ptr[src_elem]);
  }
}

template <typename SrcT, typename DstT>
inline void copy_general(const array& src, array& dst) {
  return copy_general<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), 0);
}

template <typename SrcT, typename DstT, typename stride_t>
inline void copy_general(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    const std::vector<stride_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset) {
  return copy_general<SrcT, DstT, stride_t>(
      src, dst, data_shape, i_strides, i_offset);
}

template <typename SrcT, typename DstT, typename stride_t, int D>
inline void copy_general_general_dims(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    const std::vector<stride_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset) {
  if constexpr (D > 1) {
    int axis = data_shape.size() - D;
    auto stride_src = i_strides[axis];
    auto stride_dst = o_strides[axis];
    auto N = data_shape[axis];
    for (int i = 0; i < N; i++) {
      copy_general_general_dims<SrcT, DstT, stride_t, D - 1>(
          src, dst, data_shape, i_strides, o_strides, i_offset, o_offset);
      i_offset += stride_src;
      o_offset += stride_dst;
    }
  } else {
    int axis = data_shape.size() - 1;
    auto stride_src = i_strides[axis];
    auto stride_dst = o_strides[axis];
    auto N = data_shape[axis];
    const SrcT* src_ptr = src.data<SrcT>() + i_offset;
    DstT* dst_ptr = dst.data<DstT>() + o_offset;
    for (int i = 0; i < N; i++) {
      *dst_ptr = static_cast<DstT>(*src_ptr);
      src_ptr += stride_src;
      dst_ptr += stride_dst;
    }
  }
}

template <typename SrcT, typename DstT, typename stride_t>
void copy_general_general(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    const std::vector<stride_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset) {
  auto [new_shape, new_strides] = collapse_contiguous_dims(
      data_shape, std::vector<std::vector<stride_t>>{i_strides, o_strides});
  switch (new_shape.size()) {
    case 1:
      copy_general_general_dims<SrcT, DstT, stride_t, 1>(
          src,
          dst,
          new_shape,
          new_strides[0],
          new_strides[1],
          i_offset,
          o_offset);
      return;
    case 2:
      copy_general_general_dims<SrcT, DstT, stride_t, 2>(
          src,
          dst,
          new_shape,
          new_strides[0],
          new_strides[1],
          i_offset,
          o_offset);
      return;
    case 3:
      copy_general_general_dims<SrcT, DstT, stride_t, 3>(
          src,
          dst,
          new_shape,
          new_strides[0],
          new_strides[1],
          i_offset,
          o_offset);
      return;
    case 4:
      copy_general_general_dims<SrcT, DstT, stride_t, 4>(
          src,
          dst,
          new_shape,
          new_strides[0],
          new_strides[1],
          i_offset,
          o_offset);
      return;
    case 5:
      copy_general_general_dims<SrcT, DstT, stride_t, 5>(
          src,
          dst,
          new_shape,
          new_strides[0],
          new_strides[1],
          i_offset,
          o_offset);
      return;
  }

  int size = std::accumulate(
      new_shape.end() - 5, new_shape.end(), 1, std::multiplies<int>());
  for (int i = 0; i < src.size(); i += size) {
    stride_t src_offset = i_offset + elem_to_loc(i, new_shape, new_strides[0]);
    stride_t dst_offset = o_offset + elem_to_loc(i, new_shape, new_strides[1]);
    copy_general_general_dims<SrcT, DstT, stride_t, 5>(
        src,
        dst,
        new_shape,
        new_strides[0],
        new_strides[1],
        src_offset,
        dst_offset);
  }
}

template <typename SrcT, typename DstT>
inline void copy_general_general(const array& src, array& dst) {
  return copy_general_general<SrcT, DstT, size_t>(
      src, dst, src.shape(), src.strides(), dst.strides(), 0, 0);
}

template <typename SrcT, typename DstT, typename... Args>
void copy(const array& src, array& dst, CopyType ctype, Args&&... args) {
  switch (ctype) {
    case CopyType::Scalar:
      copy_single<SrcT, DstT>(src, dst);
      return;
    case CopyType::Vector:
      copy_vector<SrcT, DstT>(src, dst);
      return;
    case CopyType::General:
      copy_general<SrcT, DstT>(src, dst, std::forward<Args>(args)...);
      return;
    case CopyType::GeneralGeneral:
      copy_general_general<SrcT, DstT>(src, dst, std::forward<Args>(args)...);
  }
}

template <typename SrcT, typename... Args>
void copy(const array& src, array& dst, CopyType ctype, Args&&... args) {
  switch (dst.dtype()) {
    case bool_:
      copy<SrcT, bool>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint8:
      copy<SrcT, uint8_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint16:
      copy<SrcT, uint16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint32:
      copy<SrcT, uint32_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint64:
      copy<SrcT, uint64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int8:
      copy<SrcT, int8_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int16:
      copy<SrcT, int16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int32:
      copy<SrcT, int32_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int64:
      copy<SrcT, int64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case float16:
      copy<SrcT, float16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case float32:
      copy<SrcT, float>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case bfloat16:
      copy<SrcT, bfloat16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case complex64:
      copy<SrcT, complex64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
  }
}

template <typename... Args>
inline void copy_inplace_dispatch(
    const array& src,
    array& dst,
    CopyType ctype,
    Args&&... args) {
  switch (src.dtype()) {
    case bool_:
      copy<bool>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint8:
      copy<uint8_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint16:
      copy<uint16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint32:
      copy<uint32_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case uint64:
      copy<uint64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int8:
      copy<int8_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int16:
      copy<int16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int32:
      copy<int32_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case int64:
      copy<int64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case float16:
      copy<float16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case float32:
      copy<float>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case bfloat16:
      copy<bfloat16_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
    case complex64:
      copy<complex64_t>(src, dst, ctype, std::forward<Args>(args)...);
      break;
  }
}

} // namespace

void copy_inplace(const array& src, array& dst, CopyType ctype) {
  return copy_inplace_dispatch(src, dst, ctype);
}

void copy(const array& src, array& dst, CopyType ctype) {
  // Allocate the output
  switch (ctype) {
    case CopyType::Vector:
      if (src.is_donatable() && src.itemsize() == dst.itemsize()) {
        dst.copy_shared_buffer(src);
      } else {
        auto size = src.data_size();
        dst.set_data(
            allocator::malloc_or_wait(size * dst.itemsize()),
            size,
            src.strides(),
            src.flags());
      }
      break;
    case CopyType::Scalar:
    case CopyType::General:
    case CopyType::GeneralGeneral:
      dst.set_data(allocator::malloc_or_wait(dst.nbytes()));
      break;
  }
  if (ctype == CopyType::GeneralGeneral) {
    ctype = CopyType::General;
  }
  copy_inplace(src, dst, ctype);
}

template <typename stride_t>
void copy_inplace(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<stride_t>& i_strides,
    const std::vector<stride_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset,
    CopyType ctype) {
  switch (ctype) {
    case CopyType::General:
    case CopyType::GeneralGeneral:
      return copy_inplace_dispatch(
          src,
          dst,
          ctype,
          data_shape,
          i_strides,
          o_strides,
          i_offset,
          o_offset);

    case CopyType::Scalar:
    case CopyType::Vector:
      return copy_inplace_dispatch(src, dst, ctype);
  }
}

template void copy_inplace<size_t>(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<size_t>& i_strides,
    const std::vector<size_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset,
    CopyType ctype);

template void copy_inplace<int64_t>(
    const array& src,
    array& dst,
    const std::vector<int>& data_shape,
    const std::vector<int64_t>& i_strides,
    const std::vector<int64_t>& o_strides,
    int64_t i_offset,
    int64_t o_offset,
    CopyType ctype);

} // namespace mlx::core
