#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "common.h"

#define MEM_CACHE_SIZE 500 * 1024 * 1024	// 500MB dont need as much memory only for client apps
#define DEFAULT_POOL_SIZE 10				// how many blocks to pre-allocate

typedef struct freeblock
{
	size_t index;
	size_t size;
	struct freeblock *next;
} freeblock_t;

static unsigned char *memcache;
static size_t memcacheused;
static size_t lastreported;

static freeblock_t *freeblocks;		// this is the actual free block list which stores alloc info
static freeblock_t *pool;			// this is the pool of free blocks to use instead of allocating new blocks every time

static freeblock_t *GetFreeBlock(void)
{
	freeblock_t *block;
	if (pool)
	{
		block = pool;
		pool = block->next;
	}

	else
		block = malloc(sizeof(*block));

	return(block);
}

static void ReturnFreeBlock(freeblock_t *block)
{
	block->next = pool;
	pool = block;
}

bool MemCache_Init(void)
{
	memcache = malloc(MEM_CACHE_SIZE);
	if (!memcache)
		return(false);

	memset(memcache, 0, MEM_CACHE_SIZE);

	freeblocks = malloc(sizeof(*freeblocks));
	if (!freeblocks)
	{
		free(memcache);
		return(false);
	}

	freeblocks->index = 0;
	freeblocks->size = MEM_CACHE_SIZE;
	freeblocks->next = NULL;

	for (int i=0; i<DEFAULT_POOL_SIZE; i++)
	{
		freeblock_t *block = malloc(sizeof(*block));
		if (!block)
		{
			free(memcache);
			free(freeblocks);

			freeblock_t *current = pool;
			while (current)
			{
				freeblock_t *next = current->next;
				free(current);
				current = next;
			}

			return(false);
		}

		block->index = 0;
		block->size = 0;
		block->next = pool;
		pool = block;
	}

	return(true);
}

void MemCache_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down memory cache");

	if (memcache)
	{
		free(memcache);
		memcache = NULL;
	}

	freeblock_t *current = freeblocks;
	while (current)
	{
		freeblock_t *next = current->next;
		free(current);
		current = next;
	}

	freeblocks = NULL;

	current = pool;
	while (current)
	{
		freeblock_t *next = current->next;
		free(current);
		current = next;
	}

	pool = NULL;
}

void *MemCache_Alloc(size_t size)
{
	assert(size > 0);
	if (!size)
		return(NULL);

	size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);

	freeblock_t *prev = NULL;
	freeblock_t *current = freeblocks;

	while (current)
	{
		if (current->size >= size)
		{
			void *ptr = memcache + current->index;
			*(size_t *)ptr = size;	// store size at start of allocation

			ptr = (unsigned char *)ptr + sizeof(size_t);

			if (current->size > size) // shrink block if it is too big
			{
				current->index += size;
				current->size -= size;
			}

			else // remove block from free list if it is the exact size
			{
				if (!prev)
					freeblocks = current->next;

				else
					prev->next = current->next;

				ReturnFreeBlock(current);
			}

			memcacheused += size;

			if (memcacheused >= (lastreported + (1024 * 1024))) // report every 1MB
			{
				Log_Write(LOG_INFO, "Memory Cache usage [bytes: %zu]", memcacheused);
				lastreported = memcacheused;
			}

			return(ptr);
		}

		prev = current;
		current = current->next;
	}

	return(NULL);
}

void MemCache_Free(void *ptr)
{
	assert(ptr);
	if (!ptr)
		return;


	ptr = (unsigned char *)ptr - sizeof(size_t);
	size_t size = *(size_t *)ptr;

	memcacheused -= size;

	size_t index = (unsigned char *)ptr - memcache;

	freeblock_t *newblock = GetFreeBlock();
	if (!newblock)
		return;

	newblock->index = index;
	newblock->size = size;

	freeblock_t *prev = NULL;
	freeblock_t *current = freeblocks;
	while (current && current->index < index)
	{
		prev = current;
		current = current->next;
	}

	if (!prev)
	{
		newblock->next = freeblocks;
		freeblocks = newblock;
	}

	else
	{
		newblock->next = prev->next;
		prev->next = newblock;
	}

	// merge with the previous block if they are adjacent
	if (prev && prev->index + prev->size == newblock->index)
	{
		prev->size += newblock->size;
		prev->next = newblock->next;
		ReturnFreeBlock(newblock);
		newblock = prev;
	}

	// merge with the next block if they are adjacent
	if (newblock->next && newblock->index + newblock->size == newblock->next->index)
	{
		newblock->size += newblock->next->size;
		freeblock_t *next = newblock->next;
		newblock->next = next->next;
		ReturnFreeBlock(next);
	}

	memset(ptr, 0, size);	// 0 out the previous allocation, no need to do this
}

void MemCache_Reset(void)
{

	Log_Write(LOG_INFO, "Resetting memory cache [last allocation: %zu bytes]", memcacheused);

	freeblock_t *current = freeblocks;
	while (current)
	{
		freeblock_t *next = current->next;
		ReturnFreeBlock(current);
		current = next;
	}

	freeblocks = GetFreeBlock();
	if (!freeblocks)
		return;

	freeblocks->index = 0;
	freeblocks->size = MEM_CACHE_SIZE;
	freeblocks->next = NULL;

	// faster than memsetting MEM_CACHE_SIZE directly, but slower if the cache is completely full, no need to do this
	freeblock_t *allocated = freeblocks->next;
	while (allocated)
	{
		memset(memcache + allocated->index, 0, allocated->size);
		allocated = allocated->next;
	}

	memcacheused = 0;
	lastreported = 0;
}

size_t MemCahce_GetMemUsed(void)
{
	return(memcacheused);
}

void MemCache_Dump(void)
{
	Log_Write(LOG_INFO, "Memory Cache Dump [bytes used by the memory cache: %zu]:", memcacheused);

	freeblock_t *current = freeblocks;
	while (current)
	{
		Log_Write(LOG_INFO, "index: %zu, size: %zu", current->index, current->size);
		current = current->next;
	}

	Log_Write(LOG_INFO, "End of Memory Cache Dump");
}

size_t MemCache_GetTotalMemory(void)
{
	return(freeblocks->size);
}
