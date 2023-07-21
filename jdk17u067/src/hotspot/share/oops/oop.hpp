/*
 * Copyright (c) 1997, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_OOPS_OOP_HPP
#define SHARE_OOPS_OOP_HPP

#include "memory/iterator.hpp"
#include "memory/memRegion.hpp"
#include "memory/sharedDefines.h"
#include "oops/accessDecorators.hpp"
#include "oops/markWord.hpp"
#include "oops/metadata.hpp"
#include "runtime/atomic.hpp"
#include "utilities/macros.hpp"

// oopDesc is the top baseclass for objects classes. The {name}Desc classes describe
// the format of Java objects so the fields can be accessed from C++.
// oopDesc is abstract.
// (see oopHierarchy for complete oop class hierarchy)
//
// no virtual functions allowed

// Forward declarations.
class OopClosure;
class FilteringClosure;

class PSPromotionManager;
class ParCompactionManager;

class oopDesc {
  friend class VMStructs;
  friend class JVMCIVMStructs;
 private:
  volatile markWord _mark;
  union _metadata {
    Klass*      _klass;
    narrowKlass _compressed_klass;
  } _metadata;

#ifdef TERA_FLAG
  // TeraFlag word is used by the TeraCache. TeraFlag is a 64-bit word and is
  // divided in three parts:
  //
  //+-------+----------------------------------------------------------------+
  //| Bits  | Description                                                    |
  //+-------+----------------------------------------------------------------+
  //| 63-48 | Represent the sublabel Id                                      |
  //+-------+----------------------------------------------------------------+
  //| 32-47 | Represent the label Id										                     |
  //+-------+----------------------------------------------------------------+
  //| 16-47 | Represent if object is leaf or primitive_array                 |
  //+-------+----------------------------------------------------------------+
  //| 15-0  | Represent the state of the object                              |
  //+-------+----------------------------------------------------------------+

  volatile uint64_t _tera_flag;      //< Mark TeraHeap objects
#endif // TERA_FLAG

 public:
  inline markWord  mark()          const;
  inline markWord* mark_addr() const;

#ifdef TERA_FLAG
  // Save the H2 destination address of the object. By saving the
  // destination address to the teraflag we avoid to overwrite the
  // mark word of the object
  inline void set_h2_dst_addr(uint64_t addr);
  
  // Get the H2 destination address of the candidate object.
  inline uint64_t get_h2_dst_addr();

  // Mark an object with 'id' to be moved in H2. H2 allocator uses the
  // 'id' to locate objects with the same 'id' by to the same region.
  // 'id' is defined by the application.
  inline void mark_move_h2(uint64_t rdd_id, uint64_t part_id);

  // Check if an object is marked to be moved in H2
  inline bool is_marked_move_h2();

  // Mark this object that is located in TeraCache
  inline void set_in_h2();

  // Get the state of the object
  inline uint64_t get_obj_state();
  
  // Init the object state 
  inline void init_obj_state();

  // Get the object group id
  inline int get_obj_group_id();

  // Get object partition Id
  inline uint64_t get_obj_part_id();

  inline bool is_live();

  inline void reset_live();

  inline void set_live();

  inline void set_visited();

  inline bool is_visited();

#ifdef P_PRIMITIVE
  // Set object flag if is promitive array or leaf object. Leaf
  // objects are the objects that contain only primitive fields and no
  // references to other objects
  inline void set_primitive(bool is_primitive_array);

  // Set object flag if is a non primitive object
  inline void set_non_primitive();
  
  // Check if the object is primitive array or leaf object. Leaf
  // objects are the objects that contain only primitive fields and no
  // references to other objects
  inline bool is_primitive();

  // Check if the object is non-primitive. 
  inline bool is_non_primitive();
  
#endif // P_PRIMITIVE
#endif // TERA_FLAG

  inline void set_mark(markWord m);
  static inline void set_mark(HeapWord* mem, markWord m);

  inline void release_set_mark(markWord m);
  inline markWord cas_set_mark(markWord new_mark, markWord old_mark);
  inline markWord cas_set_mark(markWord new_mark, markWord old_mark, atomic_memory_order order);

  // Used only to re-initialize the mark word (e.g., of promoted
  // objects during a GC) -- requires a valid klass pointer
  inline void init_mark();

  inline Klass* klass() const;
  inline Klass* klass_or_null() const;
  inline Klass* klass_or_null_acquire() const;

  void set_narrow_klass(narrowKlass nk) NOT_CDS_JAVA_HEAP_RETURN;
  inline void set_klass(Klass* k);
  static inline void release_set_klass(HeapWord* mem, Klass* k);

  // For klass field compression
  inline int klass_gap() const;
  inline void set_klass_gap(int z);
  static inline void set_klass_gap(HeapWord* mem, int z);

  // size of object header, aligned to platform wordSize
  static int header_size() { return sizeof(oopDesc)/HeapWordSize; }

  // Returns whether this is an instance of k or an instance of a subclass of k
  inline bool is_a(Klass* k) const;

  // Returns the actual oop size of the object
  inline int size();

  // Sometimes (for complicated concurrency-related reasons), it is useful
  // to be able to figure out the size of an object knowing its klass.
  inline int size_given_klass(Klass* klass);

  // type test operations (inlined in oop.inline.hpp)
  inline bool is_instance()            const;
  inline bool is_array()               const;
  inline bool is_objArray()            const;
  inline bool is_typeArray()           const;

  // type test operations that don't require inclusion of oop.inline.hpp.
  bool is_instance_noinline()          const;
  bool is_array_noinline()             const;
  bool is_objArray_noinline()          const;
  bool is_typeArray_noinline()         const;

 protected:
  inline oop        as_oop() const { return const_cast<oopDesc*>(this); }

 public:
  // field addresses in oop
  inline void* field_addr(int offset) const;

  // Need this as public for garbage collection.
  template <class T> inline T* obj_field_addr(int offset) const;

  template <typename T> inline size_t field_offset(T* p) const;

  // Standard compare function returns negative value if o1 < o2
  //                                   0              if o1 == o2
  //                                   positive value if o1 > o2
  inline static int  compare(oop o1, oop o2) {
    void* o1_addr = (void*)o1;
    void* o2_addr = (void*)o2;
    if (o1_addr < o2_addr) {
      return -1;
    } else if (o1_addr > o2_addr) {
      return 1;
    } else {
      return 0;
    }
  }

  // Access to fields in a instanceOop through these methods.
  template <DecoratorSet decorator>
  oop obj_field_access(int offset) const;
  oop obj_field(int offset) const;
  void obj_field_put(int offset, oop value);
  void obj_field_put_raw(int offset, oop value);
  void obj_field_put_volatile(int offset, oop value);

  Metadata* metadata_field(int offset) const;
  Metadata* metadata_field_raw(int offset) const;
  void metadata_field_put(int offset, Metadata* value);

  Metadata* metadata_field_acquire(int offset) const;
  void release_metadata_field_put(int offset, Metadata* value);

  jbyte byte_field(int offset) const;
  void byte_field_put(int offset, jbyte contents);

  jchar char_field(int offset) const;
  void char_field_put(int offset, jchar contents);

  jboolean bool_field(int offset) const;
  void bool_field_put(int offset, jboolean contents);
  jboolean bool_field_volatile(int offset) const;
  void bool_field_put_volatile(int offset, jboolean contents);

  jint int_field(int offset) const;
  jint int_field_raw(int offset) const;
  void int_field_put(int offset, jint contents);

  jshort short_field(int offset) const;
  void short_field_put(int offset, jshort contents);

  jlong long_field(int offset) const;
  void long_field_put(int offset, jlong contents);

  jfloat float_field(int offset) const;
  void float_field_put(int offset, jfloat contents);

  jdouble double_field(int offset) const;
  void double_field_put(int offset, jdouble contents);

  address address_field(int offset) const;
  void address_field_put(int offset, address contents);

  oop obj_field_acquire(int offset) const;
  void release_obj_field_put(int offset, oop value);

  jbyte byte_field_acquire(int offset) const;
  void release_byte_field_put(int offset, jbyte contents);

  jchar char_field_acquire(int offset) const;
  void release_char_field_put(int offset, jchar contents);

  jboolean bool_field_acquire(int offset) const;
  void release_bool_field_put(int offset, jboolean contents);

  jint int_field_acquire(int offset) const;
  void release_int_field_put(int offset, jint contents);

  jshort short_field_acquire(int offset) const;
  void release_short_field_put(int offset, jshort contents);

  jlong long_field_acquire(int offset) const;
  void release_long_field_put(int offset, jlong contents);

  jfloat float_field_acquire(int offset) const;
  void release_float_field_put(int offset, jfloat contents);

  jdouble double_field_acquire(int offset) const;
  void release_double_field_put(int offset, jdouble contents);

  address address_field_acquire(int offset) const;
  void release_address_field_put(int offset, address contents);

  // printing functions for VM debugging
  void print_on(outputStream* st) const;        // First level print
  void print_value_on(outputStream* st) const;  // Second level print.
  void print_address_on(outputStream* st) const; // Address printing

  // printing on default output stream
  void print();
  void print_value();
  void print_address();

  // return the print strings
  char* print_string();
  char* print_value_string();

  // verification operations
  static void verify_on(outputStream* st, oopDesc* oop_desc);
  static void verify(oopDesc* oopDesc);

  // locking operations
  inline bool is_locked()   const;
  inline bool is_unlocked() const;
  inline bool has_bias_pattern() const;

  // asserts and guarantees
  static bool is_oop(oop obj, bool ignore_mark_word = false);
  static bool is_oop_or_null(oop obj, bool ignore_mark_word = false);

  // garbage collection
  inline bool is_gc_marked() const;

  // Forward pointer operations for scavenge
  inline bool is_forwarded() const;

  void verify_forwardee(oop forwardee) NOT_DEBUG_RETURN;

  inline void forward_to(oop p);
  inline bool cas_forward_to(oop p, markWord compare, atomic_memory_order order = memory_order_conservative);

  // Like "forward_to", but inserts the forwarding pointer atomically.
  // Exactly one thread succeeds in inserting the forwarding pointer, and
  // this call returns "NULL" for that thread; any other thread has the
  // value of the forwarding pointer returned and does not modify "this".
  inline oop forward_to_atomic(oop p, markWord compare, atomic_memory_order order = memory_order_conservative);

  inline oop forwardee() const;

  // Age of object during scavenge
  inline uint age() const;
  inline void incr_age();

  template <typename OopClosureType>
  inline void oop_iterate(OopClosureType* cl);

  template <typename OopClosureType>
  inline void oop_iterate(OopClosureType* cl, MemRegion mr);

  template <typename OopClosureType>
  inline int oop_iterate_size(OopClosureType* cl);

  template <typename OopClosureType>
  inline int oop_iterate_size(OopClosureType* cl, MemRegion mr);

  template <typename OopClosureType>
  inline void oop_iterate_backwards(OopClosureType* cl);

  template <typename OopClosureType>
  inline void oop_iterate_backwards(OopClosureType* cl, Klass* klass);

  inline static bool is_instanceof_or_null(oop obj, Klass* klass);

  // identity hash; returns the identity hash key (computes it if necessary)
  // NOTE with the introduction of UseBiasedLocking that identity_hash() might reach a
  // safepoint if called on a biased object. Calling code must be aware of that.
  inline intptr_t identity_hash();
  intptr_t slow_identity_hash();

  // marks are forwarded to stack when object is locked
  inline bool     has_displaced_mark() const;
  inline markWord displaced_mark() const;
  inline void     set_displaced_mark(markWord m);

  // Checks if the mark word needs to be preserved
  inline bool mark_must_be_preserved() const;
  inline bool mark_must_be_preserved(markWord m) const;
  inline bool mark_must_be_preserved_for_promotion_failure(markWord m) const;

  static bool has_klass_gap();

  // for code generation
  static int mark_offset_in_bytes()      { return offset_of(oopDesc, _mark); }
  static int klass_offset_in_bytes()     { return offset_of(oopDesc, _metadata._klass); }
  static int klass_gap_offset_in_bytes() {
    assert(has_klass_gap(), "only applicable to compressed klass pointers");
#ifdef TERA_FLAG
    return klass_offset_in_bytes() + sizeof(narrowKlass) + sizeof(int64_t);
#else
    return klass_offset_in_bytes() + sizeof(narrowKlass);
#endif
  }

#ifdef TERA_FLAG
  // 0 +-----------+
  //   | _mark     |
  // 8 +-----------+
  //   | _klass    |
  // 16+-----------+
  //   | _tera_flag|
  //   +-----------+
  static int teraflag_offset_in_bytes() { return offset_of(oopDesc, _tera_flag); }
#endif

  // for error reporting
  static void* load_klass_raw(oop obj);
  static void* load_oop_raw(oop obj, int offset);

  // Avoid include gc_globals.hpp in oop.inline.hpp
  DEBUG_ONLY(bool get_UseParallelGC();)
  DEBUG_ONLY(bool get_UseG1GC();)
};

#endif // SHARE_OOPS_OOP_HPP
