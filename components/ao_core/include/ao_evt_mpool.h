#ifndef MPOOL_H
#define MPOOL_H

/**
 * @file mpool.h
 * @brief Header file for the memory pool (mpool) module.
 * @details This file contains the declarations and definitions for the memory pool module,
 *          which is responsible for managing memory pools in the system.
 * @author Roberto Axt
 * @date 2025-10-13
 * @version 0.0
 * 
 * @par License
 * This file is part of the memory pool module and is licensed under the MIT License.
 */

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initializes the memory pool module.
 * @details This function sets up the memory pool for use. It must be called
 * before any other mpool functions are used.
 * 
 * @return true if initialization is successful, false otherwise.
 * @note This function must be called before any other mpool functions.
 */
bool mpool_start(void);

/**
 * @brief Allocates a block of memory from the memory pool.
 * @details This function allocates a block of memory of the specified size
 * from the memory pool. If the requested size exceeds the block size, it
 * returns nullptr.
 * 
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or nullptr if allocation fails.
 * 
 * @note The requested size must not exceed the block size returned by mpool_block_size().
 */
void* mpool_alloc(size_t size);

/**
 * @brief Frees a previously allocated block of memory back to the memory pool.
 * @details This function returns a block of memory previously allocated
 * with mpool_alloc() back to the memory pool for reuse.
 * 
 * @param ptr A pointer to the memory block to free.
 * 
 * @note The pointer must have been returned by a previous call to mpool_alloc().
 */
void mpool_free(void* ptr);

/**
 * @brief Retrieves information about the memory pool.
 * @details This function provides information about the memory pool,
 * including the block size, total capacity, and number of free blocks.
 * @return The block size, total capacity, and number of free blocks.
 */
size_t  mpool_block_size(void);

/**
 * @brief Retrieves the total capacity and number of free blocks in the memory pool.
 * @details This function returns the total number of blocks in the memory pool
 * and the number of currently free blocks.
 * @return The total capacity and number of free blocks in the memory pool.
 */
size_t  mpool_capacity(void);

/**
 * @brief Retrieves the number of free blocks in the memory pool.
 * @details This function returns the number of currently free blocks
 * in the memory pool.
 * @return The number of free blocks in the memory pool.
 */
size_t  mpool_free_count(void);


#endif // MPOOL_H