//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "LinearAllocator.h"

#include <algorithm>
#include <iostream>

using namespace et_runtime;
using namespace et_runtime::device::memory_management;

LinearAllocator::LinearAllocator(TensorOffsetTy base, TensorSizeTy size)
    : free_list_(), allocated_list_() {
  free_list_.push_back(TensorInfo<FreeRegion>(base, size));
}

LinearAllocator::allocated_tensor_info::iterator
LinearAllocator::findAllocatedTensor(TensorID tid) {
  return find_if(
      allocated_list_.begin(), allocated_list_.end(),
      [tid](const decltype(allocated_list_)::value_type &elem) -> bool {
        return tid == elem->id();
      });
}

// std::tuple<bool, const std::shared_ptr<AbstractTensorInfo>>>
// LinearAllocator::find_allocated_tensor(TensorID tid) const
// {

// }

ErrorOr<TensorID> LinearAllocator::malloc(TensorType type, TensorSizeTy size) {
  // Compute the tensor type's metadata header size
  auto md_size = mdSize(type);
  auto total_size = md_size + size;
  // find the first free buffer where we can allocate a tensor
  auto res =
      std::find_if(free_list_.begin(), free_list_.end(),
                   [=](const decltype(free_list_)::value_type &elem) -> bool {
                     return elem.size() >= total_size;
                   });

  if (res == free_list_.end()) {
    return etrtErrorMemoryAllocation;
  }
  auto base = res->base();
  // we ran out of space in this buffer
  if (res->size() == total_size) {
    // Remove free bucket from the list
    free_list_.erase(res);
  } else {
    // Resize the free buffer
    res->tensor_info().hdr.base += total_size;
    res->tensor_info().hdr.size -= total_size;
  }

  auto tensor = createTensorInfo(type, base + md_size, size);

  // Insert the tensor in the allocated list
  auto alloc_pos = std::find_if(
      allocated_list_.begin(), allocated_list_.end(),
      [&tensor](const decltype(allocated_list_)::value_type &elem) -> bool {
        return *tensor.get() < *elem.get();
      });
  auto insert_pos = allocated_list_.insert(alloc_pos, tensor);

  // Update the prev/next pointers of the adjacent tensors
  // and maintain the double linked list in the tensor metadata
  if (insert_pos != allocated_list_.begin()) {
    auto prev_pos = std::prev(insert_pos);
    (*prev_pos)->next(tensor->base());
    tensor->prev((*prev_pos)->base());
  }
  auto next_pos = std::next(insert_pos);
  if (next_pos != allocated_list_.end()) {
    (*next_pos)->prev(tensor->base());
    tensor->next((*next_pos)->base());
  }
  return tensor->id();
}

etrtError LinearAllocator::free(TensorID tid) {

  // Search for the tensor in the allocated list
  auto buffer = findAllocatedTensor(tid);
  if (buffer == allocated_list_.end()) {
    return etrtErrorFreeUnknownTensor;
  }

  // Update the double linked list tracked in the tensor metadata
  if (buffer != allocated_list_.begin()) {
    auto prev_pos = std::prev(buffer);
    (*prev_pos)->next((*buffer)->next());
  }
  auto next_pos = std::next(buffer);
  if (next_pos != allocated_list_.end()) {
    (*next_pos)->prev((*buffer)->prev());
  }

  auto dead_tensor = *buffer;
  allocated_list_.erase(buffer);
  // Update the list, find and if necessary not concatenate the free region
  auto free_base = dead_tensor->mdBase();
  auto free_end_offset = dead_tensor->endOffset();
  auto free_size = dead_tensor->totalSize();

  auto free_list_neighbor = find_if(
      free_list_.begin(), free_list_.end(),
      [free_base](const decltype(free_list_)::value_type &elem) -> bool {
        return free_base < elem.mdBase();
      });

  if (free_list_neighbor == free_list_.end()) {
    // We have not found free region to the "right" of the new free-region
    if (free_list_.empty()) {
      free_list_.emplace_back(free_base, free_size);
    } else {
      auto &last_elem = free_list_.back();

      if (last_elem.endOffset() == free_base) {
        // If the previous buffer is directly next to this one, extend its size
        last_elem.tensor_info().hdr.size += free_size;
      } else {
        // otherwise add another free region at the end of the free list
        free_list_.emplace_back(free_base, free_size);
      }
    }
  } else {
    // There is a free-region to our right
    auto left_neighbor = std::prev(free_list_neighbor);
    // check if the free-region on the right is directly next to the new free
    // space and expand it
    if (free_end_offset == free_list_neighbor->mdBase()) {
      free_list_neighbor->tensor_info().hdr.base = free_base;
      free_list_neighbor->tensor_info().hdr.size += free_size;
      // Check if the expanded region on the right "touches" the existing region
      // to the left and merge them if true
      if (left_neighbor != free_list_.end() //
          && left_neighbor->endOffset() == free_list_neighbor->mdBase()) {
        // merge the 2 memory regions
        left_neighbor->tensor_info().hdr.size += free_list_neighbor->size();
        free_list_.erase(free_list_neighbor);
      }
    } else if (left_neighbor != free_list_.end() //
               && left_neighbor->endOffset() == free_base) {
      // The new free region is "touching" the left neighbor, extend the
      // left neighbor
      left_neighbor->tensor_info().hdr.size += free_size;
      // We do not need to check the right neighbor as we not from the above
      // that we are not
    } else {
      // We do not need to expand any of the existing free regions to the left
      // or the right, create a new one
      free_list_.insert(free_list_neighbor,
                        TensorInfo<FreeRegion>(free_base, free_size));
    }
  }

  return etrtSuccess;
}

TensorSizeTy LinearAllocator::freeMemory() {
  TensorSizeTy free_mem = 0;
  for (auto &i : free_list_) {
    free_mem += i.size();
  }
  return free_mem;
}

void LinearAllocator::printState() {
  std::cout << "Free List: \n";
  for (auto &i : free_list_) {
    std::cout << "\tRegion: " << i.base() //
              << " size: " << i.size()    //
              << "\n";
  }
  std::cout << "Alloc Region\n";
  for (auto &i : allocated_list_) {
    std::cout << "\t" << *i << "\n";
  }
  std::cout << "\n";
}
