#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "sys/sys.h"
#include "common.h"

#define MEM_CACHE_SIZE 500 * 1024 * 1024	// 500MB
#define DEFAULT_POOL_SIZE 10				// how many blocks to pre-allocate

typedef struct freeblock
{
	size_t index;
	size_t size;
	struct freeblock *next;
} freeblock_t;

typedef struct
{
	bool usecache;
	void *(*Allocate)(size_t size);
	void (*Deallocate)(void *ptr);
	void (*Reset)(void);
	void (*Dump)(void);
	size_t (*CacheGetTotalMemory)(void);
} allocator_t;

typedef struct taglist
{
	void *data;
	size_t size;
	struct taglist *next;
} taglist_t;

static unsigned char *memcache;		// this is the actual memory cache data, its just a big block of bytes
static size_t memcacheused;
static size_t lastreported;

static freeblock_t *freeblocks;		// this is the actual free block list which stores alloc info
static freeblock_t *pool;			// this is the pool of free blocks to use instead of allocating new blocks every time
static taglist_t *taglist;			// this is used by the default allocator only

static allocator_t allocator;		// if system memory is less than 4GB or the nocache param is active, use malloc/free, else use the cache allocator

static int numallocs;
static int numfrees;

static bool initialized;

/*
* Function: DefaultAlloc
* Default memory allocator using malloc instead of the cache
* 
* 	size: The size of the memory to allocate
* 
* Returns: A pointer to the allocated memory
*/
static void *DefaultAlloc(size_t size)
{
	assert(size > 0);
	if (!size || (size < 0))
		return(NULL);

	void *ptr = malloc(size);
	if (!ptr)
		return(NULL);

	taglist_t *tag = malloc(sizeof(*tag));
	if (!tag)
	{
		free(ptr);
		return(NULL);
	}

	tag->data = ptr;
	tag->size = size;
	tag->next = taglist;
	taglist = tag;

	memcacheused += size;

	if (memcacheused >= (lastreported + (1024 * 1024))) // report every 1MB
	{
		Log_Writef(LOG_INFO, "Memory Cache usage [bytes: %zu]", memcacheused);
		lastreported = memcacheused;
	}

	numallocs++;

	return(ptr);
}

/*
* Function: DefaultFree
* Default memory deallocator using free instead of the cache
* 
* 	ptr: The pointer to the memory to deallocate
*/
static void DefaultFree(void *ptr)
{
	assert(ptr);
	if (!ptr)
		return;

	taglist_t *prev = NULL;
	taglist_t *current = taglist;

	while (current)
	{
		if (current->data == ptr)
		{
			if (!prev)
				taglist = current->next;

			else
				prev->next = current->next;

			free(current->data);
			free(current);
			break;
		}

		prev = current;
		current = current->next;
	}

	numfrees++;
}

/*
* Function: DefaultReset
* Resets all memory allocated by the default allocator, freeing all memory
*/
static void DefaultReset(void)
{
	Log_Writef(LOG_INFO, "Resetting default allocator [last allocation: %zu bytes]", memcacheused);

	taglist_t *current = taglist;
	while (current)
	{
		taglist_t *next = current->next;
		free(current->data);
		free(current);
		current = next;
	}

	taglist = NULL;
	memcacheused = 0;
	lastreported = 0;

	numallocs = 0;
	numfrees = 0;
}

/*
* Function: DefaultDump
* Dumps all memory information allocated by the default allocator
*/
static void DefaultDump(void)
{
	Log_Writef(LOG_INFO, "\t\tDefault Allocator Dump [bytes used by the default allocator: %zu]:", memcacheused);

	int count = 0;
	taglist_t *current = taglist;
	while (current)
	{
		Log_Writef(LOG_INFO, "\t\t\talloc: %d, size: %zu", count, current->size);
		current = current->next;
		count++;
	}

	Log_Write(LOG_INFO, "\t\tEnd of Default Allocator Dump");
}

/*
* Function: DefaultTotalMemory
* Returns the total memory used by the default allocator
* 
* Returns: The total memory used by the default allocator
*/
static size_t DefaultTotalMemory(void)
{
	return(memcacheused);
}

/*
* Function: GetFreeBlock
* Gets a free block from the pool or allocates a new one
*
* Returns: A free block, either from the pool or a new allocation
*/
static freeblock_t *GetFreeBlock(void)
{
	freeblock_t *block;
	if (pool)
	{
		block = pool;
		pool = block->next;
	}

	else
	{
		block = malloc(sizeof(*block));

		if (block)
		{
			block->index = 0;
			block->size = 0;
			block->next = NULL;
		}
	}

	return(block);
}

/*
* Function: ReturnFreeBlock
* Returns a free block to the pool
*
* 	block: The block to return
*/
static void ReturnFreeBlock(freeblock_t *block)
{
	block->next = pool;
	pool = block;
}

/*
* Function: CacheAlloc
* Allocates memory from the pre-allocated cache
* 
* 	size: The size of the memory to allocate
* 
* Returns: A pointer to the allocated memory
*/
static void *CacheAlloc(size_t size)
{
	assert(size > 0);
	if (!size || (size < 0))
		return(NULL);

	size_t totalsize = size + sizeof(size_t);		// include the space for storing the size itself
	totalsize = (totalsize + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);

	freeblock_t *prev = NULL;
	freeblock_t *current = freeblocks;

	while (current)
	{
		if (current->size >= size)
		{
			void *ptr = memcache + current->index;
			*(size_t *)ptr = size;			// store size at start of allocation

			ptr = (unsigned char *)ptr + sizeof(size_t);

			if (current->size > totalsize)	// shrink block if it is too big
			{
				current->index += totalsize;
				current->size -= totalsize;
			}

			else	// remove block from free list if it is the exact size
			{
				if (!prev)
					freeblocks = current->next;

				else
					prev->next = current->next;

				ReturnFreeBlock(current);
			}

			memcacheused += totalsize;

			if (memcacheused >= (lastreported + (1024 * 1024))) // report every 1MB
			{
				Log_Writef(LOG_INFO, "Memory Cache usage [bytes: %zu]", memcacheused);
				lastreported = memcacheused;
			}

			assert((unsigned char *)ptr + size <= memcache + MEM_CACHE_SIZE);

			numallocs++;

			return(ptr);
		}

		prev = current;
		current = current->next;
	}

	return(NULL);
}

/*
* Function: CacheFree
* Frees memory allocated from the cache
* 
* 	ptr: The pointer to the memory to deallocate
*/
static void CacheFree(void *ptr)
{
	assert(ptr);
	if (!ptr)
		return;

	ptr = (unsigned char *)ptr - sizeof(size_t);
	size_t size = *(size_t *)ptr;

	size_t totalsize = size + sizeof(size_t);

	memcacheused -= totalsize;

	size_t index = (unsigned char *)ptr - memcache;

	freeblock_t *newblock = GetFreeBlock();
	if (!newblock)
		return;

	newblock->index = index;
	newblock->size = totalsize;

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

	numfrees++;

#if defined(MENGINE_DEBUG)
	memset(ptr, 0, size);	// 0 out the previous allocation, no need to do this
#endif
}

/*
* Function: CacheReset
* Resets the memory cache, freeing all memory and resetting the cache back to 0
*/
static void CacheReset(void)
{
	Log_Writef(LOG_INFO, "Resetting memory cache [last allocation: %zu bytes]", memcacheused);

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

#if defined(MENGINE_DEBUG)							// dont really have to do the following resetting to 0, just making the blocks point to NULL is enough
	if (memcacheused == (MEM_CACHE_SIZE / 2))
		memset(memcache, 0, MEM_CACHE_SIZE);

	else
	{
		freeblock_t *allocated = freeblocks->next;
		while (allocated)
		{
			memset(memcache + allocated->index, 0, allocated->size);
			allocated = allocated->next;
		}
	}
#endif

	memcacheused = 0;
	lastreported = 0;

	numallocs = 0;
	numfrees = 0;
}

/*
* Function: CacheDump
* Dumps all memory information allocated by the cache allocator
*/
static void CacheDump(void)
{
	Log_Writef(LOG_INFO, "\t\tMemory Cache Dump [bytes used by the memory cache: %zu]:", memcacheused);

	freeblock_t *current = freeblocks;
	while (current)
	{
		Log_Writef(LOG_INFO, "\t\t\tindex: %zu, size: %zu", current->index, current->size);
		current = current->next;
	}

	Log_Writef(LOG_INFO, "\t\tEnd of Memory Cache Dump");
}

/*
* Function: CacheTotalMemory
* Returns the total memory remaining in the cache
* 
* Returns: The total memory remaining in the cache
*/
static size_t CacheTotalMemory(void)
{
	return(freeblocks->size);
}

/*
* Function: DumpAllocData
* Dumps all allocation data, polymorphic function to call the appropriate allocator dump function, used for debugging
*/
static void DumpAllocData(void)
{
	allocator.Dump();
}

/*
* Function: MemCache_Init
* Initializes the memory cache system and picks the appropriate allocator to use based on system memory conditions
* 
* Returns: A boolean if the initialization was successful or not
*/
bool MemCache_Init(void)
{
	if (initialized)
		return(true);

	if ((Sys_GetSystemMemory() < (4 * 1024)) || Common_UseDefaultAlloc())
	{
		allocator = (allocator_t)
		{
			.usecache = false,
			.Allocate = DefaultAlloc,
			.Deallocate = DefaultFree,
			.Reset = DefaultReset,
			.Dump = DefaultDump,
			.CacheGetTotalMemory = DefaultTotalMemory
		};

		return (true);
	}

	allocator = (allocator_t)
	{
		.usecache = true,
		.Allocate = CacheAlloc,
		.Deallocate = CacheFree,
		.Reset = CacheReset,
		.Dump = CacheDump,
		.CacheGetTotalMemory = CacheTotalMemory
	};

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

	lastreported = 0;
	memcacheused = 0;

	initialized = true;

	return(true);
}

/*
* Function: MemCache_Shutdown
* Shuts down the memory cache system
*/
void MemCache_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down memory cache");

#if defined(MENGINE_DEBUG)
	int diff = numallocs - numfrees;

	Log_Writef(LOG_INFO, "\t\tNum allocs: [%d], Num frees: [%d], Difference: [%d] - %s", numallocs, numfrees, diff,
		(diff == 0) ? "All allocations have been freed" : "Not all allocations have been freed"
	);

	DumpAllocData();
#endif
	(void)DumpAllocData;

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

	initialized = false;
}

/*
* Function: MemCache_Alloc
* Allocates memory polymorphically using the appropriate allocator
*/
void *MemCache_Alloc(size_t size)
{
	return(allocator.Allocate(size));
}

/*
* Function: MemCache_Free
* Frees memory polymorphically using the appropriate allocator
*/
void MemCache_Free(void *ptr)
{
	allocator.Deallocate(ptr);
}

/*
* Function: MemCache_Reset
* Resets the memory cache or taglist depending on the allocator used
*/
void MemCache_Reset(void)
{
	allocator.Reset();
}

/*
* Function: MemCache_GetMemUsed
* Gets the total memory used by the allocator
* 
* Returns: The total memory used by the allocator
*/
size_t MemCahce_GetMemUsed(void)
{
	return(memcacheused);
}

/*
* Function: MemCache_GetTotalMemory
* Gets the total memory remaining in the cache
* 
* Returns: The total memory remaining in the cache
*/
size_t MemCache_GetTotalMemory(void)
{
	return(freeblocks->size);
}

/*
* Function: MemCache_UseCache
* Returns if the memory cache is being used or not
* 
* Returns: A boolean if the memory cache is being used or not
*/
bool MemCache_UseCache(void)
{
	return(allocator.usecache);
}
