#include <iostream>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "gc_implementation/teraCache/teraCache.hpp"
#include "gc_implementation/parallelScavenge/psVirtualspace.hpp"
#include "memory/cardTableModRefBS.hpp"
#include "memory/memRegion.hpp"
#include "memory/sharedDefines.h"
#include "runtime/globals.hpp"
#include "runtime/mutexLocker.hpp"
#include "oops/oop.inline.hpp"
#include "utilities/globalDefinitions.hpp"
#include <map>

char*        TeraCache::_start_addr = NULL;
char*        TeraCache::_stop_addr = NULL;

ObjectStartArray TeraCache::_start_array;
Stack<oop, mtGC> TeraCache::_tc_stack;
Stack<oop *, mtGC> TeraCache::_tc_adjust_stack;

uint64_t TeraCache::total_active_regions;
uint64_t TeraCache::total_merged_regions;
uint64_t TeraCache::total_objects;
uint64_t TeraCache::total_objects_size;
uint64_t TeraCache::fwd_ptrs_per_fgc;
uint64_t TeraCache::back_ptrs_per_fgc;
uint64_t TeraCache::trans_per_fgc;

uint64_t TeraCache::tc_ct_trav_time[16];
uint64_t TeraCache::heap_ct_trav_time[16];
		
uint64_t TeraCache::back_ptrs_per_mgc;
uint64_t TeraCache::intra_ptrs_per_mgc;

uint64_t TeraCache::obj_distr_size[3];	

#if NEW_FEAT
std::vector<HeapWord *> TeraCache::_mk_dirty;    //< These objects should make their cards dirty
#endif

uint64_t TeraCache::cur_obj_group_id;
uint64_t TeraCache::cur_obj_num_access;
int TeraCache::num_major_gc;

// Constructor of TeraCache
TeraCache::TeraCache() {
	
	uint64_t align = CardTableModRefBS::tc_ct_max_alignment_constraint();
	init(align);

	_start_addr = start_addr_mem_pool();
	_stop_addr  = stop_addr_mem_pool();

	// Initilize counters for TeraCache
	// These counters are used for experiments
	total_active_regions = 0;
	total_merged_regions = 0;

	total_objects = 0;
	total_objects_size = 0;

	// Initialize arrays for the next minor collection
	for (unsigned int i = 0; i < ParallelGCThreads; i++) {
		tc_ct_trav_time[i] = 0;
		heap_ct_trav_time[i] = 0;
	}

	back_ptrs_per_mgc = 0;
	intra_ptrs_per_mgc = 0;

#if PR_BUFFER
	_pr_buffer.size = 0;
	_pr_buffer.start_obj_addr_in_tc = NULL;
	_pr_buffer.buf_alloc_ptr = _pr_buffer.buffer;
#endif

	for (unsigned int i = 0; i < 3; i++) {
		obj_distr_size[i] = 0;
	}

	cur_obj_group_id = 0;
	num_major_gc = 0;
}
		
void TeraCache::tc_shutdown() {
	r_shutdown();
}


// Check if an object `ptr` belongs to the TeraCache. If the object belongs
// then the function returns true, either it returns false.
bool TeraCache::tc_check(oop ptr) {
	return ((HeapWord *)ptr >= (HeapWord *) _start_addr)     // if greater than start address
			&& ((HeapWord *) ptr < (HeapWord *)_stop_addr);  // if smaller than stop address
}

// Check if an object `p` belongs to TeraCache. If the object bolongs to
// TeraCache then the function returns true, either it returns false.
bool TeraCache::tc_is_in(void *p) {
	char* const cp = (char *)p;

	return cp >= _start_addr && cp < _stop_addr;
}

// Return the start address of the region
char* TeraCache::tc_get_addr_region(void) {
	assertf((char *)(_start_addr) != NULL, "Region is not allocated");

	return _start_addr;
}

// Return the end address of the region
char* TeraCache::tc_stop_addr_region(void) {
	assertf((char *)(_start_addr) != NULL, "Region is not allocated");
	assertf((char *)(_stop_addr) != NULL, "Region is not allocated");

	return _stop_addr;
}

// Get the size of TeraCache
size_t TeraCache::tc_get_size_region(void) {
	return mem_pool_size();
}

// Allocate new object 'obj' with 'size' in words in TeraCache.
// Return the allocated 'pos' position of the object
char* TeraCache::tc_region_top(oop obj, size_t size) {
	char *pos;			// Allocation position

	//MutexLocker x(tera_cache_lock);

	// Update Statistics
	total_objects_size += size;
	total_objects++;
	trans_per_fgc++;

#if DEBUG_TERACACHE
	printf("[BEFORE TC_REGION_TOP] | OOP(PTR) = %p | NEXT_POS = %p | SIZE = %lu | NAME %s\n",
			(HeapWord *)obj, cur_alloc_ptr(), size, obj->klass()->internal_name());
#endif

	if (TeraCacheStatistics) {
		size_t obj_size = (size * HeapWordSize) / 1024;
		int count = 0;

		while (obj_size > 0) {
			count++;
			obj_size/=1024;
		}

		assertf(count <=2, "Array out of range");

		obj_distr_size[count]++;
	}

#if ALIGN
	if (r_is_obj_fits_in_region(size))
		pos = allocate(size);
	else {
		HeapWord *cur = (HeapWord *) cur_alloc_ptr();
		HeapWord *region_end = (HeapWord *) r_region_top_addr();

		typeArrayOop filler_oop = (typeArrayOop) cur;
		filler_oop->set_mark(markOopDesc::prototype());
		filler_oop->set_klass(Universe::intArrayKlassObj());
  
		const size_t array_length =
			pointer_delta(region_end, cur) - typeArrayOopDesc::header_size(T_INT);

		assertf( (array_length * (HeapWordSize/sizeof(jint))) < (size_t)max_jint, 
				"array too big in PSPromotionLAB");

		filler_oop->set_length((int)(array_length * (HeapWordSize/sizeof(jint))));
	
		//_start_array.tc_allocate_block((HeapWord *)cur);
		if (TeraCacheStatistics)
			tclog_or_tty->print_cr("[STATISTICS] | DUMMY_OBJECT SIZE = %d\n", filler_oop->size());
	
		_start_array.tc_allocate_block(cur);
		
		pos = allocate(size);
	}
#else
	pos = allocate(size);
#endif


#if VERBOSE_TC
	if (TeraCacheStatistics)
		tclog_or_tty->print_cr("[STATISTICS] | OBJECT: %p | SIZE = %lu | ID = %d | NAME = %s",
				pos, size, obj->get_obj_group_id(), obj->klass()->internal_name());
#endif

	_start_array.tc_allocate_block((HeapWord *)pos);

	return pos;
}

// Get the allocation top pointer of the TeraCache
char* TeraCache::tc_region_cur_ptr(void) {
	return cur_alloc_ptr();
}

// Pop the objects that are in `_tc_stack` and mark them as live object. These
// objects are located in the Java Heap and we need to ensure that they will be
// kept alive.
void TeraCache::scavenge()
{
	struct timeval start_time;
	struct timeval end_time;

	gettimeofday(&start_time, NULL);

	static int i = 0;

	while (!_tc_stack.is_empty()) {
		oop obj = _tc_stack.pop();

		if (TeraCacheStatistics)
			back_ptrs_per_fgc++;

		MarkSweep::mark_and_push(&obj);
	}
	
	gettimeofday(&end_time, NULL);

	if (TeraCacheStatistics)
		tclog_or_tty->print_cr("[STATISTICS] | TC_MARK = %llu\n", 
				(unsigned long long)((end_time.tv_sec - start_time.tv_sec) * 1000) + // convert to ms
				(unsigned long long)((end_time.tv_usec - start_time.tv_usec) / 1000)); // convert to ms
}
		
void TeraCache::tc_push_object(void *p, oop o) {

#if MT_STACK
	MutexLocker x(tera_cache_lock);
#endif
	_tc_stack.push(o);
	_tc_adjust_stack.push((oop *)p);
	
	back_ptrs_per_mgc++;

	assertf(!_tc_stack.is_empty(), "Sanity Check");
	assertf(!_tc_adjust_stack.is_empty(), "Sanity Check");
}

// Adjust backwards pointers during Full GC.  
void TeraCache::tc_adjust() {
	struct timeval start_time;
	struct timeval end_time;

	gettimeofday(&start_time, NULL);

	while (!_tc_adjust_stack.is_empty()) {
		oop* obj = _tc_adjust_stack.pop();
		MarkSweep::adjust_pointer(obj);
	}
	
	gettimeofday(&end_time, NULL);

	if (TeraCacheStatistics)
		tclog_or_tty->print_cr("[STATISTICS] | TC_ADJUST %llu\n",
				(unsigned long long)((end_time.tv_sec - start_time.tv_sec) * 1000) + // convert to ms
				(unsigned long long)((end_time.tv_usec - start_time.tv_usec) / 1000)); // convert to ms
}

// Increase the number of forward ptrs from JVM heap to TeraCache
void TeraCache::tc_increase_forward_ptrs() {
	fwd_ptrs_per_fgc++;
}

// Check if the TeraCache is empty. If yes, return 'true', 'false' otherwise
bool TeraCache::tc_empty() {
	return r_is_empty();
}
		
void TeraCache::tc_clear_stacks() {
	if (TeraCacheStatistics) {
		back_ptrs_per_mgc = 0;
		intra_ptrs_per_mgc = 0;
	}
		
	_tc_adjust_stack.clear(true);
	_tc_stack.clear(true);
}

// Init the statistics counters of TeraCache to zero when a Full GC
// starts
void TeraCache::tc_init_counters() {
	fwd_ptrs_per_fgc  = 0;	
	back_ptrs_per_fgc = 0;
	trans_per_fgc     = 0;
}

// Print the statistics of TeraCache at the end of each FGC
// Will print:
//	- the total forward pointers from the JVM heap to the TeraCache
//	- the total back pointers from TeraCache to the JVM heap
//	- the total objects that has been transfered to the TeraCache
//	- the current total size of objects in TeraCache until
//	- the current total objects that are located in TeraCache
void TeraCache::tc_print_statistics() {
	tclog_or_tty->print_cr("[STATISTICS] | TOTAL_FORWARD_PTRS = %lu\n", fwd_ptrs_per_fgc);
	tclog_or_tty->print_cr("[STATISTICS] | TOTAL_BACK_PTRS = %lu\n", back_ptrs_per_fgc);
	tclog_or_tty->print_cr("[STATISTICS] | TOTAL_TRANS_OBJ = %lu\n", trans_per_fgc);

	tclog_or_tty->print_cr("[STATISTICS] | TOTAL_OBJECTS  = %lu\n", total_objects);
	tclog_or_tty->print_cr("[STATISTICS] | TOTAL_OBJECTS_SIZE = %lu\n", total_objects_size);
	tclog_or_tty->print_cr("[STATISTICS] | DISTRIBUTION | B = %lu | KB = %lu | MB = %lu\n",
			obj_distr_size[0], obj_distr_size[1], obj_distr_size[2]);
}
		
// Keep for each thread the time that need to traverse the TeraCache
// card table.
// Each thread writes the time in a table based on each ID and then we
// take the maximum time from all the threads as the total time.
void TeraCache::tc_ct_traversal_time(unsigned int tid, uint64_t total_time) {
	if (tc_ct_trav_time[tid]  < total_time)
		tc_ct_trav_time[tid] = total_time;
}

// Keep for each thread the time that need to traverse the Heap
// card table
// Each thread writes the time in a table based on each ID and then we
// take the maximum time from all the threads as the total time.
void TeraCache::heap_ct_traversal_time(unsigned int tid, uint64_t total_time) {
	if (heap_ct_trav_time[tid]  < total_time)
		heap_ct_trav_time[tid] = total_time;
}

// Print the statistics of TeraCache at the end of each minorGC
// Will print:
//	- the time to traverse the TeraCache dirty card tables
//	- the time to traverse the Heap dirty card tables
//	- TODO number of dirty cards in TeraCache
//	- TODO number of dirty cards in Heap
void TeraCache::tc_print_mgc_statistics() {
	uint64_t max_tc_ct_trav_time = 0;		//< Maximum traversal time of
											// TeraCache card tables from all
											// threads
	uint64_t max_heap_ct_trav_time = 0;     //< Maximum traversal time of Heap
											// card tables from all the threads

	for (unsigned int i = 0; i < ParallelGCThreads; i++) {
		if (max_tc_ct_trav_time < tc_ct_trav_time[i])
			max_tc_ct_trav_time = tc_ct_trav_time[i];
		
		if (max_heap_ct_trav_time < heap_ct_trav_time[i])
			max_heap_ct_trav_time = heap_ct_trav_time[i];
	}

	tclog_or_tty->print_cr("[STATISTICS] | TC_CT_TIME = %lu\n", max_tc_ct_trav_time);
	tclog_or_tty->print_cr("[STATISTICS] | HEAP_CT_TIME = %lu\n", max_heap_ct_trav_time);
	tclog_or_tty->print_cr("[STATISTICS] | BACK_PTRS_PER_MGC = %lu\n", back_ptrs_per_mgc);
#if !MT_STACK
	tclog_or_tty->print_cr("[STATISTICS] | INTRA_PTRS_PER_MGC = %lu\n", intra_ptrs_per_mgc);
#endif
	
	// Initialize arrays for the next minor collection
	for (unsigned int i = 0; i < ParallelGCThreads; i++) {
		tc_ct_trav_time[i] = 0;
		heap_ct_trav_time[i] = 0;
	}

	// Initialize counters
	back_ptrs_per_mgc = 0;
}

// Give advise to kernel to expect page references in sequential order
void TeraCache::tc_enable_seq() {
#if FMAP_HYBRID
	r_enable_huge_flts();
#else
	r_enable_seq();
#endif
}

// Give advise to kernel to expect page references in random order
void TeraCache::tc_enable_rand() {
#if FMAP_HYBRID
	r_enable_regular_flts();
#else
	r_enable_rand();
#endif
}
		
// Explicit (using systemcall) write 'data' with 'size' to the specific
// 'offset' in the file.
void TeraCache::tc_write(char *data, char *offset, size_t size) {
	r_write(data, offset, size);
}

// Explicit (using systemcall) asynchronous write 'data' with 'size' to
// the specific 'offset' in the file.
void TeraCache::tc_awrite(char *data, char *offset, size_t size) {
	r_awrite(data, offset, size);
}
		
// We need to ensure that all the writes in TeraCache using asynchronous
// I/O have been completed succesfully.
int TeraCache::tc_areq_completed() {
	return r_areq_completed();

}
		
// Fsync writes in TeraCache
// We need to make an fsync when we use fastmap
void TeraCache::tc_fsync() {
	r_fsync();
}

// Count the number of references between objects that are located in
// TC.
// This function works only when ParallelGCThreads = 1
void TeraCache::incr_intra_ptrs_per_mgc(void) {
	intra_ptrs_per_mgc++;
}

#if NEW_FEAT
// New feature
void TeraCache::tc_mk_dirty(oop obj) {
	_mk_dirty.push_back((HeapWord *)obj);

}

// New feature
bool TeraCache::tc_should_mk_dirty(HeapWord* obj) {
	for (unsigned long int i = 0; i < _mk_dirty.size(); i++) {
		if (_mk_dirty[i] == obj) {
			_mk_dirty.erase(_mk_dirty.begin() + i);
			std::cout << "BKPTR | SIZE = " << _mk_dirty.size() << std::endl;
			return true;
		}
	}
	return false;
}
#endif

#if PR_BUFFER
// Add an object 'q' with size 'size' to the promotion buffer. We use
// promotion buffer to reduce the number of system calls for small sized
// objects.
void  TeraCache::tc_prbuf_insert(char* q, char* new_adr, size_t size) {
	char* start_adr = _pr_buffer.start_obj_addr_in_tc;
	size_t cur_size = _pr_buffer.size;
	size_t free_space = PR_BUFFER_SIZE - cur_size;

	// Case1: Buffer is empty
	if (cur_size == 0) {
		assertf(start_adr == NULL, "Sanity check");
		
		if ((size * HeapWordSize) > PR_BUFFER_SIZE)
			r_awrite(q, new_adr, size);
		else {
			memcpy(_pr_buffer.buf_alloc_ptr, q, size * HeapWordSize);

			_pr_buffer.start_obj_addr_in_tc = new_adr;
			_pr_buffer.buf_alloc_ptr += (size * HeapWordSize);
			_pr_buffer.size = (size * HeapWordSize);
		}

		return;
	}
	
	// Case2: Object size is grater than the available free space in buffer
	// Case3: Object's new address is not contignious with the other objects -
	// object belongs to other region in TeraCache
	// In both cases we flush the buffer and allocate the object in a new
	// buffer.
	if (((size * HeapWordSize) > free_space) || ((start_adr + cur_size) != new_adr)) {
		tc_flush_buffer();

		// If the size of the object is still grater than the total promotion
		// buffer size then bypass the buffer and write the object to TeraCache
		// directly
		if ((size * HeapWordSize) > PR_BUFFER_SIZE)
			r_awrite(q, new_adr, size);
		else {
			memcpy(_pr_buffer.buf_alloc_ptr, q, size * HeapWordSize);

			_pr_buffer.start_obj_addr_in_tc = new_adr;
			_pr_buffer.buf_alloc_ptr += (size * HeapWordSize);
			_pr_buffer.size = (size * HeapWordSize);
		}

		return;
	}
		
	memcpy(_pr_buffer.buf_alloc_ptr, q, size * HeapWordSize);

	_pr_buffer.buf_alloc_ptr += (size * HeapWordSize);
	_pr_buffer.size += (size * HeapWordSize);
}

// Flush the promotion buffer to TeraCache and re-initialize the state of the
// promotion buffer.
void TeraCache::tc_flush_buffer() {
	if (_pr_buffer.size != 0) {
		assertf(_pr_buffer.size <= PR_BUFFER_SIZE, "Sanity check");
		
		// Write the buffer to TeraCache
		r_awrite(_pr_buffer.buffer, _pr_buffer.start_obj_addr_in_tc, _pr_buffer.size / HeapWordSize);

		memset(_pr_buffer.buffer, 0, sizeof(_pr_buffer.buffer));

		_pr_buffer.buf_alloc_ptr = _pr_buffer.buffer;
		_pr_buffer.start_obj_addr_in_tc = NULL;
		_pr_buffer.size = 0;
	}
}

#endif

#if ALIGN
bool TeraCache::tc_obj_fit_in_region(size_t size) {
	return r_is_obj_fits_in_region(size);
}
		
// We save the current object group 'id' for tera-marked object to
// promote this 'id' to its reference objects
void TeraCache::set_cur_obj_group_id(uint64_t id) {
	cur_obj_group_id = id;
}

// Get the saved current object group id 
uint64_t TeraCache::get_cur_obj_group_id(void) {
	return cur_obj_group_id;
}
		
// We save the current object number of accesses 'num_access' for
// tera-marked object to promote these number of accesses its reference
// objects
void TeraCache::set_cur_obj_num_access(uint64_t num_access) {
	cur_obj_num_access = num_access;
}

// Get the saved current object number of accesses
uint64_t TeraCache::get_cur_obj_group_num_access(void) {
	return cur_obj_num_access;
}
		
// Increase the number of major gc
void TeraCache::tc_incr_major_gc() {
	num_major_gc++;
}
		
// Get the number of major gc
int TeraCache::tc_get_major_gc() {
	return num_major_gc;
}

#endif
