/*
 * Summary: Chained hash tables
 * Description: This module implements the hash table support used in 
 * 		various places in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Bjorn Reese <bjorn.reese@systematic.dk>
 */

#ifndef __VIR_HASH_H__
#define __VIR_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The hash table.
 */
typedef struct _virHashTable virHashTable;
typedef virHashTable *virHashTablePtr;

/*
 * function types:
 */
/**
 * virHashDeallocator:
 * @payload:  the data in the hash
 * @name:  the name associated
 *
 * Callback to free data from a hash.
 */
typedef void (*virHashDeallocator)(void *payload, char *name);

/*
 * Constructor and destructor.
 */
virHashTablePtr		virHashCreate	(int size);
void			
			virHashFree	(virHashTablePtr table,
					 virHashDeallocator f);
int			virHashSize	(virHashTablePtr table);

/*
 * Add a new entry to the hash table.
 */
int			virHashAddEntry	(virHashTablePtr table,
		                         const char *name,
		                         void *userdata);
int			virHashUpdateEntry(virHashTablePtr table,
		                         const char *name,
		                         void *userdata,
					 virHashDeallocator f);

/*
 * Remove an entry from the hash table.
 */
int    			virHashRemoveEntry(virHashTablePtr table,
					 const char *name,
					 virHashDeallocator f);
/*
 * Retrieve the userdata.
 */
void *			virHashLookup	(virHashTablePtr table,
					 const char *name);

#ifdef __cplusplus
}
#endif
#endif /* ! __VIR_HASH_H__ */
