#include "khash.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#define gettid() syscall(SYS_gettid)

typedef unsigned __int128 uint128_t;
typedef uint128_t khint128_t;

#define INIT_NUM_OF_STORED_TRANSITIONS 0xfffff

/*! @function
  @abstract     64-bit integer hash function
  @param  key   The integer [khint64_t]
  @return       The hash value [khint_t]
 */
#define kh_int128_hash_func(key) (khint32_t)((key)>>33^(key)^(key)<<11) ^ (((key>>64))>>33^((key>>64))^((key>>64))<<11)
/*! @function
  @abstract     64-bit integer comparison function
 */
#define kh_int128_hash_equal(a, b) ((a) == (b))
/*! @function
  @abstract     Instantiate a hash map containing 64-bit integer keys
  @param  name  Name of the hash table [symbol]
  @param  khval_t  Type of values [type]
 */
#define KHASH_MAP_INIT_INT128(name, khval_t)								\
	KHASH_INIT(name, khint128_t, khval_t, 1, kh_int128_hash_func, kh_int128_hash_equal)

KHASH_MAP_INIT_INT128(RQ_TRACE, uint64_t)

#define INIT_TRACE_IP 0xFFFFFFFFFFFFFFFFULL

typedef struct redqueen_trace_s{
	khash_t(RQ_TRACE) *lookup;
    int fd;
	uint64_t last_bb;
} redqueen_trace_t;

static inline redqueen_trace_t* redqueen_trace_new(void){
	redqueen_trace_t* self = malloc(sizeof(redqueen_trace_t));
    pid_t tid = gettid();
    char* path = NULL;
		assert(getenv("TRACE_OUT_DIR"));
    assert(asprintf(&path, "%s/trace_thread_%d.qemu", getenv("TRACE_OUT_DIR"),tid)!=-1);
		printf("open trace file %s\n", path);
		self->fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    free(path);
	self->lookup = kh_init(RQ_TRACE);
	self->last_bb = INIT_TRACE_IP;
	return self;
}

static inline void redqueen_trace_free(redqueen_trace_t* self){
	kh_destroy(RQ_TRACE, self->lookup);
    close(self->fd);
	free(self);
}

static inline void redqueen_trace_register_transition(redqueen_trace_t* self, uint64_t to, uint16_t size){
	khiter_t k;
	int ret;
    if(to == INIT_TRACE_IP && self->last_bb == INIT_TRACE_IP){
        return;
    }
	uint128_t key = (((uint128_t)self->last_bb)<<64) | ((uint128_t)to);
	k = kh_get(RQ_TRACE, self->lookup, key); 
	//printf("%lx,%lx,%d (known %d)\n",  self->last_bb, to,size, k != kh_end(self->lookup));
	if(k != kh_end(self->lookup)){
		kh_value(self->lookup, k) += 1; 
	} else{
		k = kh_put(RQ_TRACE, self->lookup, key, &ret); 
		kh_value(self->lookup, k) = 1;
		dprintf(self->fd, "%lx,%lx,%d\n",  self->last_bb, to,size);
	}
    self->last_bb = to;
}


static inline void trace_bb(abi_ulong cur_loc, uint16_t size){
  static __thread redqueen_trace_t *trace_info;
    if(!trace_info){trace_info = redqueen_trace_new();}

    redqueen_trace_register_transition(trace_info, cur_loc, size);
}
