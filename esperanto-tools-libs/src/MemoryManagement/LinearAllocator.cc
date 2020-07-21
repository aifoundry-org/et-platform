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

#include "Tracing/Tracing.h"
#include "esperanto/runtime/Support/STLHelpers.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace et_runtime;
using namespace et_runtime::device::memory_management;

LinearAllocator::LinearAllocator(BufferOffsetTy base, BufferSizeTy size)
    : free_list_(), allocated_list_() {
  free_list_.push_back(BufferInfo<FreeRegion>(base, size));
  // FIXME add the device-ID of the memory allocator)
  TRACE_MemoryManager_LinearAllocator_Constructor(0, base, size);
}

LinearAllocator::allocated_buffer_info::iterator
LinearAllocator::findAllocatedBuffer(BufferID tid) {
  return find_if(
      allocated_list_.begin(), allocated_list_.end(),
      [tid](const decltype(allocated_list_)::value_type &elem) -> bool {
        return tid == elem->id();
      });
}

ErrorOr<BufferID> LinearAllocator::malloc(BufferType type, BufferSizeTy size,
                                          BufferSizeTy alignment) {
  // Compute the buffer type's metadata header size
  auto md_size = mdSize(type);
  auto total_size = alignmentFixSize(size, md_size, alignment);
  // find the first free buffer where we can allocate a buffer
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
    res->base(res->base() + total_size);
    res->size(res->size() - total_size);
  }

  auto aligned_start = alignedStart(base, md_size, alignment);
  assert((aligned_start + size) <= (base + total_size));

  auto buffer = createBufferInfo(type, base, aligned_start, size, total_size);

  // Insert the buffer in the allocated list
  auto alloc_pos = std::find_if(
      allocated_list_.begin(), allocated_list_.end(),
      [&buffer](const decltype(allocated_list_)::value_type &elem) -> bool {
        return *buffer.get() < *elem.get();
      });
  auto insert_pos = allocated_list_.insert(alloc_pos, buffer);

  // Update the prev/next pointers of the adjacent buffers
  // and maintain the double linked list in the buffer metadata
  BufferID left_buffer = 0, right_buffer = 0;
  if (insert_pos != allocated_list_.begin()) {
    auto prev_pos = std::prev(insert_pos);
    left_buffer = (*prev_pos)->id();
    (*prev_pos)->next(buffer->base());
    buffer->prev((*prev_pos)->base());
  }
  auto next_pos = std::next(insert_pos);
  if (next_pos != allocated_list_.end()) {
    right_buffer = (*next_pos)->id();
    (*next_pos)->prev(buffer->base());
    buffer->next((*next_pos)->base());
  }
  // FIXME add the device-id
  TRACE_MemoryManager_LinearAllocator_malloc(
      0, static_cast<int>(type), buffer->id(), base, aligned_start, md_size,
      size, total_size, left_buffer, right_buffer);

  return buffer->id();
}

ErrorOr<BufferID> LinearAllocator::emplace(BufferType type,
                                           BufferOffsetTy offset,
                                           BufferSizeTy size) {
  // Compute the buffer type's metadata header size
  auto md_size = mdSize(type);
  // The beggining of the buffer starts from the meta-data header
  auto buffer_base = offset - md_size;

  // Integer underflow
  if (offset < buffer_base) {
    return etrtErrorMemoryAllocation;
  }

  auto total_size = md_size + size;

  // find the first free buffer where we can allocate a buffer
  auto res =
      std::find_if(free_list_.begin(), free_list_.end(),
                   [=](const decltype(free_list_)::value_type &elem) -> bool {
                     return elem.base() <= buffer_base &&
                            buffer_base + total_size <= elem.endOffset();
                   });

  if (res == free_list_.end()) {
    return etrtErrorMemoryAllocation;
  }

  // we ran out of space in this buffer
  if (res->size() == total_size) {
    // Remove free bucket from the list
    free_list_.erase(res);
  } else {
    decltype(free_list_)::value_type left_part(res->base(),
                                               buffer_base - res->base()),
        right_part(buffer_base + total_size,
                   res->endOffset() - (buffer_base + total_size));

    auto it = res;
    if (right_part.size() > 0) {
      // insert before the element that we found
      it = free_list_.insert(it, right_part);
    }
    if (left_part.size() > 0) {
      // insert before the right patr
      it = free_list_.insert(it, left_part);
    }
    free_list_.erase(res);
  }

  auto buffer =
      createBufferInfo(type, buffer_base, buffer_base + md_size, size, size);

  // Insert the buffer in the allocated list
  auto alloc_pos = std::find_if(
      allocated_list_.begin(), allocated_list_.end(),
      [&buffer](const decltype(allocated_list_)::value_type &elem) -> bool {
        return *buffer.get() < *elem.get();
      });
  auto insert_pos = allocated_list_.insert(alloc_pos, buffer);

  // Update the prev/next pointers of the adjacent buffers
  // and maintain the double linked list in the buffer metadata
  BufferID left_buffer = 0, right_buffer = 0;
  if (insert_pos != allocated_list_.begin()) {
    auto prev_pos = std::prev(insert_pos);
    left_buffer = (*prev_pos)->id();
    (*prev_pos)->next(buffer->base());
    buffer->prev((*prev_pos)->base());
  }
  auto next_pos = std::next(insert_pos);
  if (next_pos != allocated_list_.end()) {
    right_buffer = (*next_pos)->id();
    (*next_pos)->prev(buffer->base());
    buffer->next((*next_pos)->base());
  }
  // FIXME add the device-id
  TRACE_MemoryManager_LinearAllocator_emplace(
      0, static_cast<int>(type), buffer->id(), buffer_base, md_size, size,
      left_buffer, right_buffer);

  return buffer->id();
}

etrtError LinearAllocator::free(BufferID tid) {

  // FIXME add device-id
  TRACE_MemoryManager_LinearAllocator_free(0, tid);

  // Search for the buffer in the allocated list
  auto buffer = findAllocatedBuffer(tid);
  if (buffer == allocated_list_.end()) {
    return etrtErrorFreeUnknownBuffer;
  }

  // Update the double linked list tracked in the buffer metadata
  if (buffer != allocated_list_.begin()) {
    auto prev_pos = std::prev(buffer);
    (*prev_pos)->next((*buffer)->next());
  }
  auto next_pos = std::next(buffer);
  if (next_pos != allocated_list_.end()) {
    (*next_pos)->prev((*buffer)->prev());
  }

  auto dead_buffer = *buffer;
  allocated_list_.erase(buffer);
  // Update the list, find and if necessary not concatenate the free region
  auto free_base = dead_buffer->base();
  auto free_end_offset = dead_buffer->endOffset();
  auto free_size = dead_buffer->size();

  auto free_list_neighbor = find_if(
      free_list_.begin(), free_list_.end(),
      [free_base](const decltype(free_list_)::value_type &elem) -> bool {
        return free_base < elem.base();
      });

  if (free_list_neighbor == free_list_.end()) {
    // We have not found free region to the "right" of the new free-region
    if (free_list_.empty()) {
      free_list_.emplace_back(free_base, free_size);
    } else {
      auto &last_elem = free_list_.back();

      if (last_elem.endOffset() == free_base) {
        // If the previous buffer is directly next to this one, extend its size
        last_elem.size(last_elem.size() + free_size);
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
    if (free_end_offset == free_list_neighbor->base()) {
      free_list_neighbor->base(free_base);
      free_list_neighbor->size(free_list_neighbor->size() + free_size);
      // Check if the expanded region on the right "touches" the existing region
      // to the left and merge them if true
      if (left_neighbor != free_list_.end() //
          && left_neighbor->endOffset() == free_list_neighbor->base()) {
        // merge the 2 memory regions
        left_neighbor->size(left_neighbor->size() + free_list_neighbor->size());
        free_list_.erase(free_list_neighbor);
      }
    } else if (left_neighbor != free_list_.end() //
               && left_neighbor->endOffset() == free_base) {
      // The new free region is "touching" the left neighbor, extend the
      // left neighbor
      left_neighbor->size(left_neighbor->size() + free_size);
      // We do not need to check the right neighbor as we not from the above
      // that we are not
    } else {
      // We do not need to expand any of the existing free regions to the left
      // or the right, create a new one
      free_list_.insert(free_list_neighbor,
                        BufferInfo<FreeRegion>(free_base, free_size));
    }
  }

  return etrtSuccess;
}

BufferSizeTy LinearAllocator::freeMemory() {
  BufferSizeTy free_mem = 0;
  for (auto &i : free_list_) {
    free_mem += i.size();
  }
  return free_mem;
}

void LinearAllocator::printState() {
  std::cout << "Free List: \n";
  for (auto &i : free_list_) {
    std::cout << "\t" << i << "\n";
  }
  std::cout << "Alloc Region\n";
  for (auto &i : allocated_list_) {
    std::cout << "\t" << *i << "\n";
  }
  std::cout << "\n";
}

void LinearAllocator::printStateJSON() {
  std::stringstream sstr;
  sstr << " { \"FreeList\": [";
  decltype(free_list_)::size_type cnt = 0;
  for (auto i = free_list_.begin(); i != free_list_.end(); ++i, ++cnt) {
    sstr << "{ \"Base\":" << i->base()  //
         << ", \"size\": " << i->size() //
         << "}";
    if (cnt + 1 < free_list_.size()) {
      sstr << ",";
    }
  }
  sstr << "],";

  sstr << "\"AllocRegion\": [";
  cnt = 0;
  for (auto i = allocated_list_.begin(); i != allocated_list_.end();
       ++i, ++cnt) {
    sstr << "{"                                            //
         << "\"ID\": " << (*i)->id()                       //
         << ", \"base\": " << (*i)->base()                 //
         << ", \"mdbase\": " << (*i)->mdBase()             //
         << ", \"alignedStart\": " << (*i)->alignedStart() //
         << ", \"size\": " << (*i)->size()                 //
         << "}";
    if (cnt + 1 < allocated_list_.size()) {
      sstr << ",";
    }
  }
  sstr << "] }";
  TRACE_MemoryManager_LinearAllocator_jsonStatus(sstr.str());
}
