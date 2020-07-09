//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "BidirectionalAllocator.h"

#include "Tracing/Tracing.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace et_runtime;
using namespace et_runtime::device::memory_management;

BidirectionalAllocator::BidirectionalAllocator(TensorOffsetTy base,
                                               TensorSizeTy size)
    : free_list_(), allocated_front_list_(), allocated_back_list_() {
  free_list_.push_back(TensorInfo<FreeRegion>(base, size));
  // FIXME add the device-id
  TRACE_MemoryManager_BidirectionalAllocator_Constructor(0, base, size);
}

BidirectionalAllocator::allocated_tensor_info::value_type
BidirectionalAllocator::findAllocatedTensor(TensorID tid) {
  for (auto &i : {&allocated_front_list_, &allocated_back_list_}) {
    auto res = find_if(
        i->begin(), i->end(),
        [tid](const std::remove_reference<decltype(*i)>::type::value_type &elem)
            -> bool { return tid == elem->id(); });
    if (res != i->end()) {
      return *res;
    }
  }
  return allocated_tensor_info::value_type();
}

ErrorOr<TensorID> BidirectionalAllocator::mallocFront(TensorType type,
                                                      TensorSizeTy size) {
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
      allocated_front_list_.begin(), allocated_front_list_.end(),
      [&tensor](const decltype(allocated_front_list_)::value_type &elem)
          -> bool { return *tensor.get() < *elem.get(); });
  auto insert_pos = allocated_front_list_.insert(alloc_pos, tensor);

  TensorID left_tensor = 0, right_tensor = 0;
  // Update the prev/next pointers of the adjacent tensors
  // and maintain the double linked list in the tensor metadata
  if (insert_pos != allocated_front_list_.begin()) {
    auto prev_pos = std::prev(insert_pos);
    left_tensor = (*prev_pos)->id();
    (*prev_pos)->next(tensor->base());
    tensor->prev((*prev_pos)->base());
  }
  auto next_pos = std::next(insert_pos);
  if (next_pos != allocated_front_list_.end()) {
    (*next_pos)->prev(tensor->base());
    right_tensor = (*next_pos)->id();
    tensor->next((*next_pos)->base());
  }

  // FIXME add the device-id
  TRACE_MemoryManager_BidirectionalAllocator_mallocFront(
      0, static_cast<int>(type), tensor->id(), base, md_size, size, left_tensor,
      right_tensor);
  return tensor->id();
}

ErrorOr<TensorID> BidirectionalAllocator::mallocBack(TensorType type,
                                                     TensorSizeTy size) {
  // Compute the tensor type's metadata header size
  auto md_size = mdSize(type);
  auto total_size = md_size + size;
  // find the first free buffer where we can allocate a tensor, starting the
  // search from the end.
  auto res =
      std::find_if(free_list_.rbegin(), free_list_.rend(),
                   [=](const decltype(free_list_)::value_type &elem) -> bool {
                     return elem.size() >= total_size;
                   });

  if (res == free_list_.rend()) {
    return etrtErrorMemoryAllocation;
  }
  auto base = res->endOffset() - total_size;
  // we ran out of space in this buffer
  if (res->size() == total_size) {
    // we cannot use a reverse iterator in erase :(, find the element again
    // FIXME the bellow is not efficient we should revisit
    auto fwit_elem =
        std::find_if(free_list_.begin(), free_list_.end(),
                     [=](const decltype(free_list_)::value_type &elem) -> bool {
                       return elem.base() == res->base();
                     });
    assert(fwit_elem != free_list_.end());
    // Remove free bucket from the list
    free_list_.erase(fwit_elem);
  } else {
    // Resize the free buffer, remove from the end of the buffer
    res->tensor_info().hdr.size -= total_size;
  }

  auto tensor = createTensorInfo(type, base + md_size, size);

  // Insert the tensor in the allocated list
  auto alloc_pos = std::find_if(
      allocated_back_list_.rbegin(), allocated_back_list_.rend(),
      [&tensor](const decltype(allocated_back_list_)::value_type &elem)
          -> bool { return *elem.get() < *tensor.get(); });
  auto insert_pos = allocated_back_list_.insert(alloc_pos.base(), tensor);

  TensorID left_tensor = 0, right_tensor = 0;
  // Update the prev/next pointers of the adjacent tensors
  // and maintain the double linked list in the tensor metadata
  if (insert_pos != allocated_back_list_.begin()) {
    auto prev_pos = std::prev(insert_pos);
    (*prev_pos)->next(tensor->base());
    left_tensor = (*prev_pos)->id();
    tensor->prev((*prev_pos)->base());
  }
  auto next_pos = std::next(insert_pos);
  if (next_pos != allocated_back_list_.end()) {
    (*next_pos)->prev(tensor->base());
    right_tensor = (*next_pos)->id();
    tensor->next((*next_pos)->base());
  }

  // FIXME add the device-id
  TRACE_MemoryManager_BidirectionalAllocator_mallocBack(
      0, static_cast<int>(type), tensor->id(), base, md_size, size, left_tensor,
      right_tensor);
  return tensor->id();
}

ErrorOr<BidirectionalAllocator::allocated_tensor_info::value_type>
BidirectionalAllocator::removeFromAllocatedList(
    allocated_tensor_info *alloc_list, TensorID tid) {
  auto res =
      find_if(alloc_list->begin(), alloc_list->end(),
              [tid](const allocated_tensor_info::value_type &elem) -> bool {
                return tid == elem->id();
              });

  if (res == alloc_list->end()) {
    return etrtErrorFreeUnknownTensor;
  }
  // Update the double linked list tracked in the tensor metadata
  if (res != alloc_list->begin()) {
    auto prev_pos = std::prev(res);
    (*prev_pos)->next((*res)->next());
  }
  auto next_pos = std::next(res);
  if (next_pos != alloc_list->end()) {
    (*next_pos)->prev((*res)->prev());
  }

  auto ptr = *res;
  alloc_list->erase(res);
  return ptr;
}

etrtError BidirectionalAllocator::free(TensorID tid) {

  // FIXME add device_id
  TRACE_MemoryManager_BidirectionalAllocator_free(0, tid);
  // Search for the tensor in the forward list allocated list
  auto dead_tensor_res = removeFromAllocatedList(&allocated_front_list_, tid);
  if (dead_tensor_res.getError() != etrtSuccess) {
    // Search the back list
    dead_tensor_res = removeFromAllocatedList(&allocated_back_list_, tid);
    if (dead_tensor_res.getError() != etrtSuccess) {
      return dead_tensor_res.getError();
    }
  }

  auto dead_tensor = dead_tensor_res.get();
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
        // If the previous buffer is directly next to this one, extend its
        // size
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
      // Check if the expanded region on the right "touches" the existing
      // region
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

TensorSizeTy BidirectionalAllocator::freeMemory() {
  TensorSizeTy free_mem = 0;
  for (auto &i : free_list_) {
    free_mem += i.size();
  }
  return free_mem;
}

void BidirectionalAllocator::printState() {
  std::cout << "Free List: \n";
  for (auto &i : free_list_) {
    std::cout << "\tRegion: " << i.base() //
              << " size: " << i.size()    //
              << "\n";
  }
  std::cout << "Alloc Region\n";
  for (auto &i : allocated_front_list_) {
    std::cout << "\t" << *i << "\n";
  }
  std::cout << "\n";

  for (auto &i : allocated_back_list_) {
    std::cout << "\t" << *i << "\n";
  }
  std::cout << "\n";
}

void BidirectionalAllocator::printStateJSON() {
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
  for (auto i = allocated_front_list_.begin(); i != allocated_front_list_.end();
       ++i) {
    sstr << "{"
         << "\"ID\": " << (*i)->id() << ", \"mdbase\": " << (*i)->mdBase()
         << ", \"base\": " << (*i)->base() << ", \"size\": " << (*i)->size()
         << "}, ";
  }
  cnt = 0;
  for (auto i = allocated_back_list_.begin(); i != allocated_back_list_.end();
       ++i, ++cnt) {
    sstr << "{"
         << "\"ID\": " << (*i)->id() << ", \"mdbase\": " << (*i)->mdBase()
         << ", \"base\": " << (*i)->base() << ", \"size\": " << (*i)->size()
         << "}";
    if (cnt + 1 < allocated_back_list_.size()) {
      sstr << ",";
    }
  }

  sstr << "] }";
  TRACE_MemoryManager_BidirectionalAllocator_jsonStatus(sstr.str());
}
