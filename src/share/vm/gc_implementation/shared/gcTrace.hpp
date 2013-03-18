/*
 * Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_GC_IMPLEMENTATION_SHARED_GCTRACE_HPP
#define SHARE_VM_GC_IMPLEMENTATION_SHARED_GCTRACE_HPP

#include "gc_interface/gcCause.hpp"
#include "gc_interface/gcName.hpp"
#include "gc_implementation/shared/gcWhen.hpp"
#include "memory/allocation.hpp"
#include "memory/referenceType.hpp"
#ifndef SERIALGC
#include "gc_implementation/g1/g1YCTypes.hpp"
#endif

typedef uint GCId;

class GCHeapSummary;
class PermGenSummary;
class PSHeapSummary;
class ReferenceProcessorStats;
class TimePartitions;

class SharedGCInfo VALUE_OBJ_CLASS_SPEC {
  static const jlong UNSET_TIMESTAMP = -1;

 public:
  static const GCId UNSET_GCID = (GCId)-1;

 private:
  GCId _id;
  GCName _name;
  GCCause::Cause _cause;
  jlong _start_timestamp;
  jlong _end_timestamp;
  jlong _sum_of_pauses;
  jlong _longest_pause;

 public:
  SharedGCInfo(GCName name) : _id(UNSET_GCID), _name(name), _cause(GCCause::_last_gc_cause),
      _start_timestamp(UNSET_TIMESTAMP), _end_timestamp(UNSET_TIMESTAMP), _sum_of_pauses(0), _longest_pause(0) {}

  void set_id(GCId id) { _id = id; }
  GCId id() const { return _id; }

  void set_start_timestamp(jlong timestamp) { _start_timestamp = timestamp; }
  jlong start_timestamp() const { return _start_timestamp; }

  void set_end_timestamp(jlong timestamp) { _end_timestamp = timestamp; }
  jlong end_timestamp() const { return _end_timestamp; }

  void set_name(GCName name) { _name = name; }
  GCName name() const { return _name; }

  void set_cause(GCCause::Cause cause) { _cause = cause; }
  GCCause::Cause cause() const { return _cause; }

  void set_sum_of_pauses(jlong duration) { _sum_of_pauses = duration; }
  jlong sum_of_pauses() const { return _sum_of_pauses; }

  void set_longest_pause(jlong duration) { _longest_pause = duration; }
  jlong longest_pause() const { return _longest_pause; }
};

class ParallelOldGCInfo VALUE_OBJ_CLASS_SPEC {
  void* _dense_prefix;
 public:
  ParallelOldGCInfo() : _dense_prefix(NULL) {}
  void report_dense_prefix(void* addr) {
    _dense_prefix = addr;
  }
  void* dense_prefix() const { return _dense_prefix; }
};

class YoungGCInfo VALUE_OBJ_CLASS_SPEC {
  bool _promotion_failed;
 public:
  YoungGCInfo() : _promotion_failed(false) {}
  void register_promotion_failed() {
    _promotion_failed = true;
  }
  bool promotion_failed() const { return _promotion_failed; }
};

#ifndef SERIALGC

class G1YoungGCInfo VALUE_OBJ_CLASS_SPEC {
  G1YCType _type;
 public:
  G1YoungGCInfo() : _type(G1YCTypeEndSentinel) {}
  void set_type(G1YCType type) {
    _type = type;
  }
  G1YCType type() const { return _type; }
};

#endif // SERIALGC

class GCTracer : public ResourceObj {
 protected:
  SharedGCInfo _shared_gc_info;

 public:
  void report_gc_start(GCCause::Cause cause, jlong timestamp);
  void report_gc_end(jlong timestamp, TimePartitions* time_partitions);
  void report_gc_heap_summary(GCWhen::Type when, const GCHeapSummary& heap_summary, const PermGenSummary& perm_gen_summary) const;
  void report_gc_reference_stats(const ReferenceProcessorStats& rp) const;

  bool has_reported_gc_start() const;

 protected:
  GCTracer(GCName name) : _shared_gc_info(name) {}
  virtual void report_gc_start_impl(GCCause::Cause cause, jlong timestamp);
  virtual void report_gc_end_impl(jlong timestamp, TimePartitions* time_partitions);

 private:
  void send_garbage_collection_event() const;
  void send_gc_heap_summary_event(GCWhen::Type when, const GCHeapSummary& heap_summary) const;
  void send_perm_gen_summary_event(GCWhen::Type when, const PermGenSummary& perm_gen_summary) const;
  void send_reference_stats_event(ReferenceType type, size_t count) const;
  void send_phase_events(TimePartitions* time_partitions) const;
};

class YoungGCTracer : public GCTracer {
  YoungGCInfo _young_gc_info;

 protected:
  YoungGCTracer(GCName name) : GCTracer(name) {}
  virtual YoungGCInfo& young_gc_info() { return _young_gc_info; }

 public:
  virtual void report_promotion_failed(size_t size, uint count);

 protected:
  virtual void report_gc_end_impl(jlong timestamp, TimePartitions* time_partitions);

 private:
  void send_young_gc_event() const;
  void send_promotion_failed_event(size_t size, uint count) const;
};

class OldGCTracer : public GCTracer {
 protected:
  OldGCTracer(GCName name) : GCTracer(name) {}

 protected:
  virtual void report_gc_end_impl(jlong timestamp, TimePartitions* time_partitions);

 private:
  void send_old_gc_event() const;
};

class ParallelOldTracer : public OldGCTracer {
  ParallelOldGCInfo _parallel_old_gc_info;

 public:
  ParallelOldTracer() : OldGCTracer(ParallelOld) {}
  void report_dense_prefix(void* dense_prefix);

 protected:
  void report_gc_end_impl(jlong timestamp, TimePartitions* time_partitions);

 private:
  void send_parallel_old_event() const;
};

class SerialOldTracer : public OldGCTracer {
 public:
  SerialOldTracer() : OldGCTracer(SerialOld) {}
};

class ParallelScavengeTracer : public YoungGCTracer {
 public:
  ParallelScavengeTracer() : YoungGCTracer(ParallelScavenge) {}
};

class DefNewTracer : public YoungGCTracer {
 public:
  DefNewTracer() : YoungGCTracer(DefNew) {}
};

class ParNewTracer : public YoungGCTracer {
 public:
  ParNewTracer() : YoungGCTracer(ParNew) {}
};

#ifndef SERIALGC
class G1NewTracer : public YoungGCTracer {
  G1YoungGCInfo _g1_young_gc_info;

 public:
  G1NewTracer() : YoungGCTracer(G1New) {}

  void report_yc_type(G1YCType type);
  void report_gc_end_impl(jlong timestamp, TimePartitions* time_partitions);

 private:
  void send_g1_young_gc_event();
};
#endif

class CMSTracer : public OldGCTracer {
 public:
  CMSTracer() : OldGCTracer(ConcurrentMarkSweep) {}
};

class G1OldTracer : public OldGCTracer {
 public:
  G1OldTracer() : OldGCTracer(G1Old) {}
};

#endif // SHARE_VM_GC_IMPLEMENTATION_SHARED_GCTRACE_HPP
