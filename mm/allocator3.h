#include "mm3.h"

#ifndef __allocator_protocol__
#define __allocator_protocol__

void   allocator_init();
void   allocator_close();
void   allocator_reset();

void*  allocator_alloc(cacao_size_t size);

void   allocator_free_block(cacao_ptr_t block_addr, cacao_size_t size);

void   allocator_free_prejoin   (cacao_ptr_t block_addr, cacao_ptr_t pre_addr,  cacao_size_t pre_size);
void   allocator_free_postjoin  (cacao_ptr_t block_addr, cacao_ptr_t post_addr, cacao_size_t post_size);
void   allocator_free_blockjoin (cacao_ptr_t first_addr, cacao_ptr_t last_addr, cacao_size_t extra_size);

#endif
