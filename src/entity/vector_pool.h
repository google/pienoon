// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VECTOR_POOL_H
#define VECTOR_POOL_H

#include <stddef.h>
#include <vector>
#include "assert.h"

namespace fpl {

enum AllocationLocation { kAddToFront, kAddToBack };

// Pool allocator, implemented as a vector-based pair of linked lists.
template <typename T>
class VectorPool {
  template <bool>
  friend class IteratorTemplate;
  friend class VectorPoolReference;
  typedef uint32_t UniqueIdType;

 public:
  template <bool>
  class IteratorTemplate;

  typedef IteratorTemplate<false> Iterator;
  typedef IteratorTemplate<true> ConstIterator;

  // ---------------------------
  // Reference object for pointing into the vector pool.
  // Basically works as a pointer for vector pool elements, except
  // it can tell if it is no longer valid.  (i. e. if the element it
  // pointed at has either been deallocated, or replaced with a new
  // element.)
  // Also correctly handles situations where the underlying vector resizes,
  // moving the elements around in memory.
  class VectorPoolReference {
    friend class VectorPool<T>;
    template <bool>
    friend class IteratorTemplate;

   public:
    VectorPoolReference() : container_(nullptr), index_(0), unique_id_(0) {}

    VectorPoolReference(VectorPool<T>* container, size_t index)
        : container_(container), index_(index) {
      unique_id_ = container->GetElement(index)->unique_id;
    }

    // Standard equality operator
    bool operator==(const VectorPoolReference& other) const {
      return container_ == other.container_ && index_ == other.index_;
    }

    // Standard inequality operator
    bool operator!=(const VectorPoolReference& other) const {
      return !operator==(other);
    }

    // Check to make sure that the reference is still valid.  Will return false
    // if the object pointed to has been freed, even if the location was
    // later filled with a new object.
    bool IsValid() const {
      return container_ != nullptr &&
             (container_->GetElement(index_)->unique_id == unique_id_);
    }

    // Member access operator.  Returns a pointer to the data the
    // VectorPoolReference is referring to, allowing you to use
    // VectorPoolReferences like pointers, syntactically.
    // (e. g. myVectorPoolReference->MyDataMember = x;)
    // Throws an assert if the VectorPoolReference is no longer valid.
    // (i. e. if something has deleted the thing we were pointing to.)
    T* operator->() {
      assert(IsValid());
      VectorPoolElement* element = container_->GetElement(index_);
      return &(element->data);
    }

    // Const member access operator.  Returns a const pointer to the data the
    // VectorPoolReference is referring to, allowing you to use
    // VectorPoolReferences like const pointers, syntactically.
    // (e. g. x = myVectorPoolReference->MyDataMember;)
    // Throws an assert if the VectorPoolReference is no longer valid.
    // (i. e. if something has deleted the thing we were pointing to.)
    const T* operator->() const {
      return const_cast<VectorPoolReference*>(this)->operator->();
    }

    // Dereference operator.  Returns a reference variable for the data
    // the VectorPoolReference points to, allowing you to use
    // VectorPoolReference like a pointer:
    // (e. g. MyDataVariable = (*MyVectorPoolReference);
    T& operator*() {
      assert(IsValid());
      VectorPoolElement* element = container_->GetElement(index_);
      return element->data;
    }

    // Const dereference operator.  Returns a const reference variable for the
    //  data the VectorPoolReference points to, allowing you to use
    // VectorPoolReference like a pointer:
    // (e. g. MyDataVariable = (*MyVectorPoolReference);
    const T& operator*() const {
      return const_cast<VectorPoolReference*>(this)->operator*();
    }

    // Returns a direct pointer to the element the VectorPoolReference is
    // referring to.  Note that this pointer is not guaranteed to remain
    // legal, since the vector may need to relocate the data in memory.
    // It's recommended that when working with data, it be left as a
    // VectorPoolReference, and only converted to a pointer when needed.
    T* ToPointer() {
      return IsValid() ? &(container_->GetElement(index_)->data) : nullptr;
    }

    // Returns a direct const pointer to the element the VectorPoolReference is
    // referring to.  Note that this pointer is not guaranteed to remain
    // legal, since the vector may need to relocate the data in memory.
    // It's recommended that when working with data, it be left as a
    // VectorPoolReference, and only converted to a pointer when needed.
    const T* ToPointer() const {
      return const_cast<VectorPoolReference*>(this)->ToPointer();
    }

    // Returns an iterator pointing at the element we're referencing
    Iterator ToIterator() const { return Iterator(container_, index_); }

    // Returns the raw index into the underlying vector for this object.
    size_t index() { return index_; }

   private:
    VectorPool<T>* container_;
    size_t index_;
    UniqueIdType unique_id_;
  };

  // ---------------------------
  // Iterator for the vector pool.
  // Has constant-time access, so is a good choice for iterating
  // over the active elements that the pool owns.
  template <bool is_const>
  class IteratorTemplate {
    typedef typename std::conditional<is_const, const T&, T&>::type reference;
    typedef typename std::conditional<is_const, const T*, T*>::type pointer;

    friend class VectorPool<T>;

   public:
    IteratorTemplate(VectorPool<T>* container, size_t index)
        : container_(container), index_(index) {}
    ~IteratorTemplate() {}

    // Standard equality operator
    bool operator==(const IteratorTemplate& other) const {
      return container_ == other.container_ && index_ == other.index_;
    }

    // Standard inequality operator
    bool operator!=(const IteratorTemplate& other) const {
      return !operator==(other);
    }

    // Prefix increment - moves the iterator one forward in the
    // list.
    IteratorTemplate& operator++() {
      index_ = container_->elements_[index_].next;
      return (*this);
    }

    // Postfix increment - moves the iterator one forward in the
    // list, but returns the original (unincremented) iterator.
    IteratorTemplate operator++(int) {
      IteratorTemplate temp = *this;
      ++(*this);
      return temp;
    }

    // Prefix decrement - moves the iterator one back in the list
    IteratorTemplate& operator--() {
      index_ = container_->elements_[index_].prev;
      return (*this);
    }

    // Prefix decrement - moves the iterator one back in the list, but
    // returns the original (undecremented) iterator.
    IteratorTemplate operator--(int) {
      IteratorTemplate temp = *this;
      --(*this);
      return temp;
    }

    // Iterator dereference
    reference operator*() { return *(container_->GetElementData(index_)); }

    // Member access on the object
    pointer operator->() { return container_->GetElementData(index_); }

    // Converts the iterator into a VectorPoolReference, which is the preferred
    // way for holding onto references into the vector pool.
    VectorPoolReference ToReference() const {
      return VectorPoolReference(container_, index_);
    }

    size_t index() const { return index_; }

   private:
    VectorPool<T>* container_;
    size_t index_;
  };

  // ---------------------------

  static const size_t kOutOfBounds = static_cast<size_t>(-1);
  static const UniqueIdType kInvalidId = 0;
  struct VectorPoolElement {
    VectorPoolElement()
        : next(kOutOfBounds), prev(kOutOfBounds), unique_id(kInvalidId) {}
    T data;
    size_t next;
    size_t prev;
    UniqueIdType unique_id;
  };

  // Constants for our first/last elements. They're never given actual data,
  //  but are used as list demarcations.
  static const size_t kFirstUsed = 0;
  static const size_t kLastUsed = 1;
  static const size_t kFirstFree = 2;
  static const size_t kLastFree = 3;
  static const size_t kTotalReserved = 4;

  // Basic constructor.
  VectorPool() : active_count_(0), next_unique_id_(kInvalidId + 1) { Clear(); }

  // Get data at the specified element index.
  // Returns a pointer to the data.  Asserts if the index is obviously illegal.
  // (i. e. out of range for the underlying vector)
  // Note that the pointer is not guaranteed to remain valid, if the vector
  // needs to relocate the data in memory.  In general, if you need to hold
  // on to a reference to a data element, it is recommended that you use
  // VectorPoolReference.
  T* GetElementData(size_t index) {
    assert(index < elements_.size());
    return (&elements_[index].data);
  }

  // Get data at the specified element index.
  // Returns a const pointer to the data.  Asserts if the index is obviously
  // illegal.  (i. e. out of range for the underlying vector)
  // Note that the pointer is not guaranteed to remain valid, if the vector
  // needs to relocate the data in memory.  In general, if you need to hold
  // on to a reference to a data element, it is recommended that you use
  // VectorPoolReference.
  const T* GetElementData(size_t index) const {
    assert(index < elements_.size());
    return (&elements_[index].data);
  }

  // Returns a reference to a new element.  Grabs the first free element if one
  // exists, otherwise allocates a new one on the vector.
  VectorPoolReference GetNewElement(AllocationLocation alloc_location) {
    size_t index;
    if (elements_[kFirstFree].next != kLastFree) {
      index = elements_[kFirstFree].next;
      RemoveFromList(index);  // remove it from the list of free elements.
    } else {
      index = elements_.size();
      elements_.push_back(VectorPoolElement());
    }
    switch (alloc_location) {
      case kAddToFront:
        AddToListFront(index, kFirstUsed);
        break;
      case kAddToBack:
        AddToListBack(index, kLastUsed);
        break;
      default:
        assert(0);
    }
    active_count_++;
    // Placement new, to make sure we always give back a cleanly constructed
    // element:
    new (&(elements_[index].data)) T;
    elements_[index].unique_id = AllocateUniqueId();
    return VectorPoolReference(this, index);
  }

  // Frees up an element.  Removes it from the list of active elements, and
  // adds it to the front of the inactive list.
  void FreeElement(size_t index) {
    RemoveFromList(index);
    AddToListFront(index, kFirstFree);
    elements_[index].unique_id = kInvalidId;
    active_count_--;
  }

  // Frees up an element.  The element will be added to the "empty list", and
  // used later, when we add elements to the vector pool.
  void FreeElement(VectorPoolReference element) {
    if (element.IsValid()) {
      FreeElement(element.index_);
    }
  }

  // Free element, except it accepts an iterator instead of a
  // vectorpoolreference as an argument.
  Iterator FreeElement(Iterator iter) {
    Iterator temp = iter++;
    FreeElement(temp.index_);
    return iter;
  }

  // Returns the total size of the vector pool.  This is the total number of
  // allocated elements (used AND free) by the underlying vector.
  size_t Size() const { return elements_.size(); }

  // Returns the total number of active elements.
  size_t active_count() const { return active_count_; }

  // Clears out all elements of the vectorpool, and resizes the underlying
  // vector to the minimum.
  void Clear() {
    elements_.resize(kTotalReserved);
    elements_[kFirstUsed].next = kLastUsed;
    elements_[kLastUsed].prev = kFirstUsed;
    elements_[kFirstFree].next = kLastFree;
    elements_[kLastFree].prev = kFirstFree;
    active_count_ = 0;
  }

  // Returns an iterator suitable for traversing all of the active elements
  // in the vectorpool.
  Iterator begin() { return Iterator(this, elements_[kFirstUsed].next); }

  // Returns an iterator at the end of the vectorpool, suitable for use as
  // an end condition when iterating over the active elements.
  Iterator end() { return Iterator(this, kLastUsed); }

  // Returns a const iterator suitable for traversing all of the active elements
  // in the vectorpool.
  ConstIterator cbegin() {
    return ConstIterator(this, elements_[kFirstUsed].next);
  }

  // Returns a const iterator at the end of the vectorpool, suitable for use as
  // an end condition when iterating over the active elements.
  ConstIterator cend() { return ConstIterator(this, kLastUsed); }

  // Expands the vector until it is at least new_size.  If the vector
  // already contains at least new_size elements, then there is no effect.
  void Reserve(size_t new_size) {
    size_t current_size = elements_.size();
    if (current_size >= new_size) return;

    elements_.resize(new_size);
    for (; current_size < new_size; current_size++) {
      elements_[current_size].unique_id = kInvalidId;
      AddToListFront(current_size, kFirstFree);
    }
  }

 private:
  // Utility function for removing an element from whatever list it is a part
  // of.  Should always be followed by AddToList, to reassign it, so we don't
  // lose track of it.
  void RemoveFromList(size_t index) {
    assert(index < elements_.size() && index >= kTotalReserved);
    VectorPoolElement& element = elements_[index];
    elements_[element.prev].next = element.next;
    elements_[element.next].prev = element.prev;
  }

  // Utility function to add an element to a list.  Adds it to whichever list is
  // specified.  (start_index is the start of the list we want to add it to,
  // usually either kFirstUsed or kFirstFree.
  // This function adds to the front of the list.
  void AddToListFront(size_t index, size_t start_index) {
    assert(index < elements_.size() && index >= kTotalReserved);
    VectorPoolElement& list_start = elements_[start_index];
    elements_[list_start.next].prev = index;
    elements_[index].prev = start_index;
    elements_[index].next = list_start.next;
    list_start.next = index;
  }

  // Same as AddToListFront, except it sends it to the back of the list.
  // Note that this means you have to give it the end of a list instead of
  // the front of one.  (i. e. kLastUsed instead of kFirstUsed, etc.)
  void AddToListBack(size_t index, size_t end_index) {
    assert(index < elements_.size() && index >= kTotalReserved);
    VectorPoolElement& list_end = elements_[end_index];
    elements_[list_end.prev].next = index;
    elements_[index].next = end_index;
    elements_[index].prev = list_end.prev;
    list_end.prev = index;
  }

  // Returns an element specified by an index.  Note that this is different from
  // GetElementData, in that this one returns a pointer to the full
  // VectorPoolElement, as opposed to just the user data.
  VectorPoolElement* GetElement(size_t index) {
    return index < elements_.size() ? &elements_[index] : nullptr;
  }

  // Const version of the above.
  const VectorPoolElement* GetElement(size_t index) const {
    return index < elements_.size() ? &elements_[index] : nullptr;
  }

  // Allocates a unique ID.  Usually used when a new element is allocated.
  // Since the unique ID is done by simply allocating a counter, it's
  // theoretically possible that this could wrap around, especially if
  // something has kept a constant reference the whole time.  This will
  // cause a lot of things to break, so ideally, don't use this class in
  // situations where there are likely to be over 4,294,967,295 allocations.
  // (Or change UniqueTypeID into uint_64 or something.)
  UniqueIdType AllocateUniqueId() {
    // untility function to make sure it rolls over correctly.
    UniqueIdType result = next_unique_id_;
    next_unique_id_++;
    if (next_unique_id_ == kInvalidId) next_unique_id_++;
    return result;
  }

  std::vector<VectorPoolElement> elements_;
  size_t active_count_;
  UniqueIdType next_unique_id_;
};

}  // fpl

#endif  // VECTOR_POOL_H
