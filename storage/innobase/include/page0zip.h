/*****************************************************************************

Copyright (c) 2005, 2013, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2012, Facebook Inc.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/page0zip.h
Compressed page interface

Created June 2005 by Marko Makela
*******************************************************/

#ifndef page0zip_h
#define page0zip_h

#ifdef UNIV_MATERIALIZE
# undef UNIV_INLINE
# define UNIV_INLINE
#endif

#include "mtr0types.h"
#include "page0types.h"
#include "buf0types.h"
#include "dict0types.h"
#include "srv0srv.h"
#include "trx0types.h"
#include "mem0mem.h"

/* Compression level to be used by zlib. Settable by user. */
extern uint	page_zip_level;

/* Default compression level. */
#define DEFAULT_COMPRESSION_LEVEL	6

/* Whether or not to log compressed page images to avoid possible
compression algorithm changes in zlib. */
extern my_bool	page_zip_log_pages;

/**********************************************************************//**
Determine the size of a compressed page in bytes.
@return	size in bytes */
UNIV_INLINE
ulint
page_zip_get_size(
/*==============*/
	const page_zip_des_t*	page_zip)	/*!< in: compressed page */
	__attribute__((nonnull, pure));
/**********************************************************************//**
Set the size of a compressed page in bytes. */
UNIV_INLINE
void
page_zip_set_size(
/*==============*/
	page_zip_des_t*	page_zip,	/*!< in/out: compressed page */
	ulint		size);		/*!< in: size in bytes */

#ifndef UNIV_HOTBACKUP
/**********************************************************************//**
Determine if a record is so big that it needs to be stored externally.
@return	FALSE if the entire record can be stored locally on the page */
UNIV_INLINE
ibool
page_zip_rec_needs_ext(
/*===================*/
	ulint	rec_size,	/*!< in: length of the record in bytes */
	ulint	comp,		/*!< in: nonzero=compact format */
	ulint	n_fields,	/*!< in: number of fields in the record;
				ignored if zip_size == 0 */
	ulint	zip_size)	/*!< in: compressed page size in bytes, or 0 */
	__attribute__((const));

/**********************************************************************//**
Determine the guaranteed free space on an empty page.
@return	minimum payload size on the page */
UNIV_INTERN
ulint
page_zip_empty_size(
/*================*/
	ulint	n_fields,	/*!< in: number of columns in the index */
	ulint	zip_size)	/*!< in: compressed page size in bytes */
	__attribute__((const));
#endif /* !UNIV_HOTBACKUP */

/**********************************************************************//**
Initialize a compressed page descriptor. */
UNIV_INLINE
void
page_zip_des_init(
/*==============*/
	page_zip_des_t*	page_zip);	/*!< in/out: compressed page
					descriptor */

/**********************************************************************//**
Configure the zlib allocator to use the given memory heap. */
UNIV_INTERN
void
page_zip_set_alloc(
/*===============*/
	void*		stream,		/*!< in/out: zlib stream */
	mem_heap_t*	heap);		/*!< in: memory heap to use */

/** Compress a page.
@param[in/out]	page_zip	compressed page; in: size;
out: data, n_blobs,m_start, m_end, m_nonempty
@param[in]	page		uncompressed page
@param[in]	index		B-tree index
@param[in]	level		compression level
@param[in/out]	mtr		mini-transaction; NULL=no logging
@return true on success, false on failure; page_zip will be left
intact on failure. */
UNIV_INTERN
bool
page_zip_compress(
	page_zip_des_t*		page_zip,
	const page_t*		page,
	const dict_index_t*	index,
	ulint			level,
	mtr_t*			mtr)
	__attribute__((nonnull(1,2,3)));

/**********************************************************************//**
Decompress a page.  This function should tolerate errors on the compressed
page.  Instead of letting assertions fail, it will return FALSE if an
inconsistency is detected.
@return	TRUE on success, FALSE on failure */
UNIV_INTERN
ibool
page_zip_decompress(
/*================*/
	page_zip_des_t*	page_zip,/*!< in: data, ssize;
				out: m_start, m_end, m_nonempty, n_blobs */
	page_t*		page,	/*!< out: uncompressed page, may be trashed */
	ibool		all)	/*!< in: TRUE=decompress the whole page;
				FALSE=verify but do not copy some
				page header fields that should not change
				after page creation */
	__attribute__((nonnull(1,2)));

#ifdef UNIV_DEBUG
/**********************************************************************//**
Validate a compressed page descriptor.
@return	TRUE if ok */
UNIV_INLINE
ibool
page_zip_simple_validate(
/*=====================*/
	const page_zip_des_t*	page_zip);	/*!< in: compressed page
						descriptor */
#endif /* UNIV_DEBUG */

#ifdef UNIV_ZIP_DEBUG
/** Check that the compressed and decompressed pages match.
@param[in] page_zip	compressed page
@param[in] page		uncompressed page
@param[in] index	index of the page, or NULL if not known
@param[in] sloppy	false=strict, true=ignore REC_INFO_MIN_REC_FLAG
@return	true if valid, false if not */
UNIV_INTERN
bool
page_zip_validate_low(
	const page_zip_des_t*	page_zip,
	const page_t*		page,
	const dict_index_t*	index,
	bool			sloppy);
# define page_zip_validate_if_zip(page_zip, page, index)		\
	ut_a(!(page_zip) || page_zip_validate(page_zip, page, index))
# define page_zip_validate_sloppy_if_zip(page_zip, page, index)		\
	ut_a(!(page_zip) || page_zip_validate_low(page_zip, page, index, true))
#else
# define page_zip_validate_low(page_zip, page, index, sloppy) /* empty */
# define page_zip_validate_if_zip(page_zip, page, index) /* empty */
# define page_zip_validate_sloppy_if_zip(page_zip, page, index) /* empty */
#endif /* UNIV_ZIP_DEBUG */

/**********************************************************************//**
Check that the compressed and decompressed pages match.
@param[in] page_zip	compressed page
@param[in] page		uncompressed page
@param[in] index	index of the page, or NULL if not known
@return	true if valid, false if not */
#define page_zip_validate(page_zip, page, index)			\
	page_zip_validate_low(page_zip, page, index, recv_recovery_is_on())

/**********************************************************************//**
Determine how big record can be inserted without recompressing the page.
@return a positive number indicating the maximum size of a record
whose insertion is guaranteed to succeed, or zero or negative */
UNIV_INLINE
lint
page_zip_max_ins_size(
/*==================*/
	const page_zip_des_t*	page_zip,/*!< in: compressed page */
	ibool			is_clust)/*!< in: TRUE if clustered index */
	__attribute__((nonnull, pure));

/**********************************************************************//**
Determine if enough space is available in the modification log.
@return	TRUE if page_zip_write_rec() will succeed */
UNIV_INLINE
ibool
page_zip_available(
/*===============*/
	const page_zip_des_t*	page_zip,/*!< in: compressed page */
	ibool			is_clust,/*!< in: TRUE if clustered index */
	ulint			length,	/*!< in: combined size of the record */
	ulint			create)	/*!< in: nonzero=add the record to
					the heap */
	__attribute__((nonnull, pure));

/**********************************************************************//**
Write data to the uncompressed header portion of a page.  The data must
already have been written to the uncompressed page. */
UNIV_INLINE
void
page_zip_write_header(
/*==================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	const byte*	str,	/*!< in: address on the uncompressed page */
	ulint		length,	/*!< in: length of the data */
	mtr_t*		mtr)	/*!< in: mini-transaction, or NULL */
	__attribute__((nonnull(1,2)));

/** Write an entire record on the compressed page.
The data must already have been written to the uncompressed page.
@param[in/out]	page_zip	compressed page
@param[in]	rec		the record in the uncompressed page
@param[in]	index		B-tree index
@param[in]	offsets		rec_get_offsets(rec, index)
@param[in]	create		nonzero=insert, 0=update */
UNIV_INTERN
void
page_zip_write_rec(
	page_zip_des_t*		page_zip,
	const byte*		rec,
	const dict_index_t*	index,
	const ulint*		offsets,
	ulint			create)
	__attribute__((nonnull));

/***********************************************************//**
Parses a log record of writing a BLOB pointer of a record.
@return	end of log record or NULL */
UNIV_INTERN
byte*
page_zip_parse_write_blob_ptr(
/*==========================*/
	byte*		ptr,	/*!< in: redo log buffer */
	byte*		end_ptr,/*!< in: redo log buffer end */
	page_t*		page,	/*!< in/out: uncompressed page */
	page_zip_des_t*	page_zip);/*!< in/out: compressed page */

/** Write a BLOB pointer of a record on the leaf page of a clustered index.
The information must already have been updated on the uncompressed page.
@param[in/out]	page_zip	compressed page
@param[in]	rec		the record in the uncompressed page
@param[in]	index		B-tree index
@param[in]	offsets		rec_get_offsets(rec, index)
@param[in]	n		column index
@param[in/out]	mtr		mini-transaction; NULL=no logging */
UNIV_INTERN
void
page_zip_write_blob_ptr(
	page_zip_des_t*	page_zip,
	const byte*	rec,
	dict_index_t*	index,
	const ulint*	offsets,
	ulint		n,
	mtr_t*		mtr)
	__attribute__((nonnull(1,2,3,4)));

/***********************************************************//**
Parses a log record of writing the node pointer of a record.
@return	end of log record or NULL */
UNIV_INTERN
byte*
page_zip_parse_write_node_ptr(
/*==========================*/
	byte*		ptr,	/*!< in: redo log buffer */
	byte*		end_ptr,/*!< in: redo log buffer end */
	page_t*		page,	/*!< in/out: uncompressed page */
	page_zip_des_t*	page_zip);/*!< in/out: compressed page */

/**********************************************************************//**
Write the node pointer of a record on a non-leaf compressed page. */
UNIV_INTERN
void
page_zip_write_node_ptr(
/*====================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	byte*		rec,	/*!< in/out: record */
	ulint		size,	/*!< in: data size of rec */
	ulint		ptr,	/*!< in: node pointer */
	mtr_t*		mtr)	/*!< in: mini-transaction, or NULL */
	__attribute__((nonnull(1,2)));

/**********************************************************************//**
Write the trx_id and roll_ptr of a record on a B-tree leaf node page. */
UNIV_INTERN
void
page_zip_write_trx_id_and_roll_ptr(
/*===============================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	byte*		rec,	/*!< in/out: record */
	const ulint*	offsets,/*!< in: rec_get_offsets(rec, index) */
	ulint		trx_id_col,/*!< in: column number of TRX_ID in rec */
	trx_id_t	trx_id,	/*!< in: transaction identifier */
	roll_ptr_t	roll_ptr)/*!< in: roll_ptr */
	__attribute__((nonnull));

/**********************************************************************//**
Write the "deleted" flag of a record on a compressed page.  The flag must
already have been written on the uncompressed page. */
UNIV_INTERN
void
page_zip_rec_set_deleted(
/*=====================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	const byte*	rec,	/*!< in: record on the uncompressed page */
	ulint		flag)	/*!< in: the deleted flag (nonzero=TRUE) */
	__attribute__((nonnull));

/**********************************************************************//**
Write the "owned" flag of a record on a compressed page.  The n_owned field
must already have been written on the uncompressed page. */
UNIV_INTERN
void
page_zip_rec_set_owned(
/*===================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	const byte*	rec,	/*!< in: record on the uncompressed page */
	ulint		flag)	/*!< in: the owned flag (nonzero=TRUE) */
	__attribute__((nonnull));

/**********************************************************************//**
Insert a record to the dense page directory. */
UNIV_INTERN
void
page_zip_dir_insert(
/*================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	const byte*	prev_rec,/*!< in: record after which to insert */
	const byte*	free_rec,/*!< in: record from which rec was
				allocated, or NULL */
	byte*		rec);	/*!< in: record to insert */

/** Shift the dense page directory when a record is deleted.
@param[in/out]	page_zip	compressed page
@param[in]	free		previous start of the PAGE_FREE list
@param[in/out]	rec		deleted record (new head of PAGE_FREE)
@param[in]	index		index B-tree
@param[in]	data_size	user data size of rec, in bytes
@param[in]	extra_size	header size of rec, in bytes
@param[in]	n_ext		number of externally stored fields */
UNIV_INTERN
void
page_zip_dir_delete(
	page_zip_des_t*		page_zip,
	const byte*		free,
	byte*			rec,
	const dict_index_t*	index,
	ulint			data_size,
	ulint			extra_size,
	ulint			n_ext);

/**********************************************************************//**
Add a slot to the dense page directory. */
UNIV_INTERN
void
page_zip_dir_add_slot(
/*==================*/
	page_zip_des_t*	page_zip,	/*!< in/out: compressed page */
	ulint		is_clustered)	/*!< in: nonzero for clustered index,
					zero for others */
	__attribute__((nonnull));

/***********************************************************//**
Parses a log record of writing to the header of a page.
@return	end of log record or NULL */
UNIV_INTERN
byte*
page_zip_parse_write_header(
/*========================*/
	byte*		ptr,	/*!< in: redo log buffer */
	byte*		end_ptr,/*!< in: redo log buffer end */
	page_t*		page,	/*!< in/out: uncompressed page */
	page_zip_des_t*	page_zip);/*!< in/out: compressed page */

/**********************************************************************//**
Write data to the uncompressed header portion of a page.  The data must
already have been written to the uncompressed page.
However, the data portion of the uncompressed page may differ from
the compressed page when a record is being inserted in
page_cur_insert_rec_low(). */
UNIV_INLINE
void
page_zip_write_header(
/*==================*/
	page_zip_des_t*	page_zip,/*!< in/out: compressed page */
	const byte*	str,	/*!< in: address on the uncompressed page */
	ulint		length,	/*!< in: length of the data */
	mtr_t*		mtr)	/*!< in: mini-transaction, or NULL */
	__attribute__((nonnull(1,2)));

/** Reorganize and compress a page.  This is a low-level operation for
compressed pages, to be used when page_zip_compress() fails.
On success, a redo log entry MLOG_ZIP_PAGE_COMPRESS will be written.
The function btr_page_reorganize() should be preferred whenever possible.
IMPORTANT: if page_zip_reorganize() is invoked on a leaf page of a
non-clustered index, the caller must update the insert buffer free
bits in the same mini-transaction in such a way that the modification
will be redo-logged.
@param[in/out]	block	uncompressed and compressed page;
on the compressed page, in: size;
out: data, n_blobs, m_start, m_end,m_nonempty
@param[in]	index	B-tree index
@param[in/out]	mtr	mini-transaction
@retval true on success
@retval false on failure; page_zip will be left intact, but block->frame
will be overwritten. */
UNIV_INTERN
bool
page_zip_reorganize(
	buf_block_t*		block,
	const dict_index_t*	index,
	mtr_t*			mtr)
	__attribute__((nonnull));
#ifndef UNIV_HOTBACKUP
/**********************************************************************//**
Copy the records of a page byte for byte.  Do not copy the page header
or trailer, except those B-tree header fields that are directly
related to the storage of records.  Also copy PAGE_MAX_TRX_ID.
NOTE: The caller must update the lock table and the adaptive hash index. */
UNIV_INTERN
void
page_zip_copy_recs(
/*===============*/
	page_zip_des_t*		page_zip,	/*!< out: copy of src_zip
						(n_blobs, m_start, m_end,
						m_nonempty, data[0..size-1]) */
	page_t*			page,		/*!< out: copy of src */
	const page_zip_des_t*	src_zip,	/*!< in: compressed page */
	const page_t*		src,		/*!< in: page */
	dict_index_t*		index,		/*!< in: index of the B-tree */
	mtr_t*			mtr)		/*!< in: mini-transaction */
	__attribute__((nonnull));
#endif /* !UNIV_HOTBACKUP */

/**********************************************************************//**
Parses a log record of compressing an index page.
@return	end of log record or NULL */
UNIV_INTERN
byte*
page_zip_parse_compress(
/*====================*/
	byte*		ptr,	/*!< in: buffer */
	byte*		end_ptr,/*!< in: buffer end */
	page_t*		page,	/*!< out: uncompressed page */
	page_zip_des_t*	page_zip)/*!< out: compressed page */
	__attribute__((nonnull(1,2)));

/**********************************************************************//**
Calculate the compressed page checksum.
@return	page checksum */
UNIV_INTERN
ulint
page_zip_calc_checksum(
/*===================*/
        const void*     data,   /*!< in: compressed page */
        ulint           size,   /*!< in: size of compressed page */
	srv_checksum_algorithm_t algo) /*!< in: algorithm to use */
	__attribute__((nonnull));

/**********************************************************************//**
Verify a compressed page's checksum.
@return	TRUE if the stored checksum is valid according to the value of
innodb_checksum_algorithm */
UNIV_INTERN
ibool
page_zip_verify_checksum(
/*=====================*/
	const void*	data,	/*!< in: compressed page */
	ulint		size);	/*!< in: size of compressed page */
/** Write a redo log record of compressing an index page
without the data on the page.
@param[in]	level		compression level
@param[in]	page		uncompressed page that was compressed
@param[in]	index		B-tree index
@param[in/out]	mtr		mini-transaction */
UNIV_INLINE
void
page_zip_compress_write_log_no_data(
	ulint			level,
	const page_t*		page,
	const dict_index_t*	index,
	mtr_t*			mtr)
	__attribute__((nonnull));
/** Parses a log record of compressing an index page without the data.
@param[in]	ptr		redo log record
@param[in]	end_ptr		end of redo log record area
@param[in]	page		uncompressed page that was compressed
@param[in/out]	page_zip	compressed page
@param[in]	index		B-tree index
@return	end of log record or NULL */
UNIV_INLINE
byte*
page_zip_parse_compress_no_data(
	byte*			ptr,		/*!< in: buffer */
	byte*			end_ptr,	/*!< in: buffer end */
	page_t*			page,		/*!< in: uncompressed page */
	page_zip_des_t*		page_zip,	/*!< out: compressed page */
	const dict_index_t*	index)		/*!< in: index */
	__attribute__((nonnull(1,2)));

/**********************************************************************//**
Reset the counters used for filling
INFORMATION_SCHEMA.innodb_cmp_per_index. */
UNIV_INLINE
void
page_zip_reset_stat_per_index();
/*===========================*/

#ifndef UNIV_HOTBACKUP
/** Check if a pointer to an uncompressed page matches a compressed page.
When we IMPORT a tablespace the blocks and accompanying frames are allocted
from outside the buffer pool.
@param ptr	pointer to an uncompressed page frame
@param page_zip	compressed page descriptor
@return		TRUE if ptr and page_zip refer to the same block */
# define PAGE_ZIP_MATCH(ptr, page_zip)					\
	(((page_zip)->m_external					\
	  && (page_align(ptr) + UNIV_PAGE_SIZE == (page_zip)->data))	\
	  || buf_frame_get_page_zip(ptr) == (page_zip))
#else /* !UNIV_HOTBACKUP */
/** Check if a pointer to an uncompressed page matches a compressed page.
@param ptr	pointer to an uncompressed page frame
@param page_zip	compressed page descriptor
@return		TRUE if ptr and page_zip refer to the same block */
# define PAGE_ZIP_MATCH(ptr, page_zip)				\
	(page_align(ptr) + UNIV_PAGE_SIZE == (page_zip)->data)
#endif /* !UNIV_HOTBACKUP */

#ifdef UNIV_MATERIALIZE
# undef UNIV_INLINE
# define UNIV_INLINE	UNIV_INLINE_ORIGINAL
#endif

#ifndef UNIV_NONINL
# include "page0zip.ic"
#endif

#endif /* page0zip_h */
