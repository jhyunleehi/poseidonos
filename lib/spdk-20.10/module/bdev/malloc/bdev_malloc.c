/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "spdk/stdinc.h"

#include "bdev_malloc.h"
#include "spdk/bdev.h"
#include "spdk/endian.h"
#include "spdk/env.h"
#include "spdk/accel_engine.h"
#include "spdk/json.h"
#include "spdk/thread.h"
#include "spdk/queue.h"
#include "spdk/string.h"
#include "spdk/ioat.h"

#include "spdk/bdev_module.h"
#include "spdk/log.h"

struct malloc_disk {
	struct spdk_bdev		disk;
	void				*malloc_buf;
	TAILQ_ENTRY(malloc_disk)	link;
};

struct malloc_task {
	int				num_outstanding;
	enum spdk_bdev_io_status	status;
};

struct ioat_io_channel {
	struct spdk_ioat_chan	*ioat_ch;
	struct ioat_device	*ioat_dev;
	struct spdk_poller	*poller;
};

static void
malloc_done(void *ref, int status)
{
	struct malloc_task *task = (struct malloc_task *)ref;

	if (status != 0) {
		if (status == -ENOMEM) {
			task->status = SPDK_BDEV_IO_STATUS_NOMEM;
		} else {
			task->status = SPDK_BDEV_IO_STATUS_FAILED;
		}
	}
	if (--task->num_outstanding == 0) {
		spdk_bdev_io_complete(spdk_bdev_io_from_ctx(task), task->status);
	}
}

static void
malloc_done_ioat(void *ref)
{
	malloc_done(ref, 0);
}

static TAILQ_HEAD(, malloc_disk) g_malloc_disks = TAILQ_HEAD_INITIALIZER(g_malloc_disks);

int malloc_disk_count = 0;

static int bdev_malloc_initialize(void);

static int
bdev_malloc_get_ctx_size(void)
{
	return sizeof(struct malloc_task);
}

static struct spdk_bdev_module malloc_if = {
	.name = "malloc",
	.module_init = bdev_malloc_initialize,
	.get_ctx_size = bdev_malloc_get_ctx_size,

};

SPDK_BDEV_MODULE_REGISTER(malloc, &malloc_if)

static void
malloc_disk_free(struct malloc_disk *malloc_disk)
{
	if (!malloc_disk) {
		return;
	}

	free(malloc_disk->disk.name);
	spdk_free(malloc_disk->malloc_buf);
	free(malloc_disk);
}

static int
bdev_malloc_destruct(void *ctx)
{
	struct malloc_disk *malloc_disk = ctx;

	TAILQ_REMOVE(&g_malloc_disks, malloc_disk, link);
	malloc_disk_free(malloc_disk);
	return 0;
}

static int
bdev_malloc_check_iov_len(struct iovec *iovs, int iovcnt, size_t nbytes)
{
	int i;

	for (i = 0; i < iovcnt; i++) {
		if (nbytes < iovs[i].iov_len) {
			return 0;
		}

		nbytes -= iovs[i].iov_len;
	}

	return nbytes != 0;
}

static void
bdev_malloc_readv(struct malloc_disk *mdisk, struct spdk_io_channel *ch,
		  struct malloc_task *task,
		  struct iovec *iov, int iovcnt, size_t len, uint64_t offset)
{
	int64_t res = 0;
	void *src = mdisk->malloc_buf + offset;
	int i;

	if (bdev_malloc_check_iov_len(iov, iovcnt, len)) {
		spdk_bdev_io_complete(spdk_bdev_io_from_ctx(task),
				      SPDK_BDEV_IO_STATUS_FAILED);
		return;
	}

	SPDK_DEBUGLOG(bdev_malloc, "read %lu bytes from offset %#lx\n",
		      len, offset);

	task->status = SPDK_BDEV_IO_STATUS_SUCCESS;
	task->num_outstanding = iovcnt;

	for (i = 0; i < iovcnt; i++) {
		res = spdk_accel_submit_copy(ch, iov[i].iov_base,
					     src, iov[i].iov_len, malloc_done, task);

		if (res != 0) {
			malloc_done(task, res);
		}

		src += iov[i].iov_len;
		len -= iov[i].iov_len;
	}
}

static void
bdev_malloc_writev(struct malloc_disk *mdisk, struct spdk_io_channel *ch,
		   struct malloc_task *task,
		   struct iovec *iov, int iovcnt, size_t len, uint64_t offset)
{
	int64_t res = 0;
	void *dst = mdisk->malloc_buf + offset;
	int i;

	if (bdev_malloc_check_iov_len(iov, iovcnt, len)) {
		spdk_bdev_io_complete(spdk_bdev_io_from_ctx(task),
				      SPDK_BDEV_IO_STATUS_FAILED);
		return;
	}

	SPDK_DEBUGLOG(bdev_malloc, "wrote %lu bytes to offset %#lx\n",
		      len, offset);

	task->status = SPDK_BDEV_IO_STATUS_SUCCESS;
	task->num_outstanding = iovcnt;

	for (i = 0; i < iovcnt; i++) {
		res = spdk_accel_submit_copy(ch, dst, iov[i].iov_base,
					     iov[i].iov_len, malloc_done, task);

		if (res != 0) {
			malloc_done(task, res);
		}

		dst += iov[i].iov_len;
	}
}

static int
bdev_malloc_unmap(struct malloc_disk *mdisk,
		  struct spdk_io_channel *ch,
		  struct malloc_task *task,
		  uint64_t offset,
		  uint64_t byte_count)
{
	task->status = SPDK_BDEV_IO_STATUS_SUCCESS;
	task->num_outstanding = 1;

	return spdk_accel_submit_fill(ch, mdisk->malloc_buf + offset, 0,
				      byte_count, malloc_done, task);
}

static int64_t
bdev_malloc_flush(struct malloc_disk *mdisk, struct malloc_task *task,
		  uint64_t offset, uint64_t nbytes)
{
	spdk_bdev_io_complete(spdk_bdev_io_from_ctx(task), SPDK_BDEV_IO_STATUS_SUCCESS);

	return 0;
}

static int
bdev_malloc_reset(struct malloc_disk *mdisk, struct malloc_task *task)
{
	spdk_bdev_io_complete(spdk_bdev_io_from_ctx(task), SPDK_BDEV_IO_STATUS_SUCCESS);

	return 0;
}

static int _bdev_malloc_submit_request(struct spdk_io_channel *ch, struct spdk_bdev_io *bdev_io)
{
	uint32_t block_size = bdev_io->bdev->blocklen;

	switch (bdev_io->type) {
	case SPDK_BDEV_IO_TYPE_READ:
		if (bdev_io->u.bdev.iovs[0].iov_base == NULL) {
			assert(bdev_io->u.bdev.iovcnt == 1);
			bdev_io->u.bdev.iovs[0].iov_base =
				((struct malloc_disk *)bdev_io->bdev->ctxt)->malloc_buf +
				bdev_io->u.bdev.offset_blocks * block_size;
			bdev_io->u.bdev.iovs[0].iov_len = bdev_io->u.bdev.num_blocks * block_size;
			spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_SUCCESS);
			return 0;
		}

		bdev_malloc_readv((struct malloc_disk *)bdev_io->bdev->ctxt,
				  ch,
				  (struct malloc_task *)bdev_io->driver_ctx,
				  bdev_io->u.bdev.iovs,
				  bdev_io->u.bdev.iovcnt,
				  bdev_io->u.bdev.num_blocks * block_size,
				  bdev_io->u.bdev.offset_blocks * block_size);
		return 0;

	case SPDK_BDEV_IO_TYPE_WRITE:
		bdev_malloc_writev((struct malloc_disk *)bdev_io->bdev->ctxt,
				   ch,
				   (struct malloc_task *)bdev_io->driver_ctx,
				   bdev_io->u.bdev.iovs,
				   bdev_io->u.bdev.iovcnt,
				   bdev_io->u.bdev.num_blocks * block_size,
				   bdev_io->u.bdev.offset_blocks * block_size);
		return 0;

	case SPDK_BDEV_IO_TYPE_RESET:
		return bdev_malloc_reset((struct malloc_disk *)bdev_io->bdev->ctxt,
					 (struct malloc_task *)bdev_io->driver_ctx);

	case SPDK_BDEV_IO_TYPE_FLUSH:
		return bdev_malloc_flush((struct malloc_disk *)bdev_io->bdev->ctxt,
					 (struct malloc_task *)bdev_io->driver_ctx,
					 bdev_io->u.bdev.offset_blocks * block_size,
					 bdev_io->u.bdev.num_blocks * block_size);

	case SPDK_BDEV_IO_TYPE_UNMAP:
		return bdev_malloc_unmap((struct malloc_disk *)bdev_io->bdev->ctxt,
					 ch,
					 (struct malloc_task *)bdev_io->driver_ctx,
					 bdev_io->u.bdev.offset_blocks * block_size,
					 bdev_io->u.bdev.num_blocks * block_size);

	case SPDK_BDEV_IO_TYPE_WRITE_ZEROES:
		/* bdev_malloc_unmap is implemented with a call to mem_cpy_fill which zeroes out all of the requested bytes. */
		return bdev_malloc_unmap((struct malloc_disk *)bdev_io->bdev->ctxt,
					 ch,
					 (struct malloc_task *)bdev_io->driver_ctx,
					 bdev_io->u.bdev.offset_blocks * block_size,
					 bdev_io->u.bdev.num_blocks * block_size);

	case SPDK_BDEV_IO_TYPE_ZCOPY:
		if (bdev_io->u.bdev.zcopy.start) {
			void *buf;
			size_t len;

			buf = ((struct malloc_disk *)bdev_io->bdev->ctxt)->malloc_buf +
			      bdev_io->u.bdev.offset_blocks * block_size;
			len = bdev_io->u.bdev.num_blocks * block_size;
			spdk_bdev_io_set_buf(bdev_io, buf, len);

		}
		spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_SUCCESS);
		return 0;
	case SPDK_BDEV_IO_TYPE_ABORT:
		spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
		return 0;
	default:
		return -1;
	}
	return 0;
}

static void bdev_malloc_submit_request(struct spdk_io_channel *ch, struct spdk_bdev_io *bdev_io)
{
	if (_bdev_malloc_submit_request(ch, bdev_io) != 0) {
		spdk_bdev_io_complete(bdev_io, SPDK_BDEV_IO_STATUS_FAILED);
	}
}

static bool
bdev_malloc_io_type_supported(void *ctx, enum spdk_bdev_io_type io_type)
{
	switch (io_type) {
	case SPDK_BDEV_IO_TYPE_READ:
	case SPDK_BDEV_IO_TYPE_WRITE:
	case SPDK_BDEV_IO_TYPE_FLUSH:
	case SPDK_BDEV_IO_TYPE_RESET:
	case SPDK_BDEV_IO_TYPE_UNMAP:
	case SPDK_BDEV_IO_TYPE_WRITE_ZEROES:
	case SPDK_BDEV_IO_TYPE_ZCOPY:
	case SPDK_BDEV_IO_TYPE_ABORT:
		return true;

	default:
		return false;
	}
}

static struct spdk_io_channel *
bdev_malloc_get_io_channel(void *ctx)
{
	return spdk_accel_engine_get_io_channel();
}

static void
bdev_malloc_write_json_config(struct spdk_bdev *bdev, struct spdk_json_write_ctx *w)
{
	char uuid_str[SPDK_UUID_STRING_LEN];

	spdk_json_write_object_begin(w);

	spdk_json_write_named_string(w, "method", "bdev_malloc_create");

	spdk_json_write_named_object_begin(w, "params");
	spdk_json_write_named_string(w, "name", bdev->name);
	spdk_json_write_named_uint64(w, "num_blocks", bdev->blockcnt);
	spdk_json_write_named_uint32(w, "block_size", bdev->blocklen);
	spdk_uuid_fmt_lower(uuid_str, sizeof(uuid_str), &bdev->uuid);
	spdk_json_write_named_string(w, "uuid", uuid_str);

	spdk_json_write_object_end(w);

	spdk_json_write_object_end(w);
}

static const struct spdk_bdev_fn_table malloc_fn_table = {
	.destruct		= bdev_malloc_destruct,
	.submit_request		= bdev_malloc_submit_request,
	.io_type_supported	= bdev_malloc_io_type_supported,
	.get_io_channel		= bdev_malloc_get_io_channel,
	.write_config_json	= bdev_malloc_write_json_config,
};

void *malloc_disk_base = NULL;

__attribute__((used))
void *
bdev_malloc_get_buf(void)
{
    return malloc_disk_base;
}

int
backup_disk_info_for_restore(struct malloc_disk* mdisk, const char* name,
    uint64_t num_blocks, uint64_t block_size)
{
    int rc = 0;
    char* file_name;
    const char* backup_dir = "/tmp/";
    const char* backup_file_postfix = ".uram.info";
    int fd = -1;
    int pid = getpid();
    const uint64_t baseAddr = 0x200000000000ULL;
    const uint32_t bytesPerHugepage = 2 * 1024 * 1024;
    uint64_t bufAddr = (uint64_t)mdisk->malloc_buf;
    uint64_t startPage = (bufAddr - baseAddr) / bytesPerHugepage - 1;
    uint64_t pageCount = (num_blocks * block_size) / bytesPerHugepage;
    char writeBuf[256] = {
        0,
    };
    uint32_t strLength;

    if (!name)
    {
        return -EINVAL;
    }

    file_name = calloc(strlen(backup_dir) + strlen(name) + strlen(backup_file_postfix) + 1,
        sizeof(char));
    if (!file_name)
    {
        SPDK_ERRLOG("mdisk calloc() failed\n");
        return -ENOMEM;
    }
    strncat(file_name, backup_dir, strlen(backup_dir));
    strncat(file_name, name, strlen(name));
    strncat(file_name, backup_file_postfix, strlen(backup_file_postfix));

    SPDK_INFOLOG(bdev_malloc,
        "Current PID: %d, buffer v address: %p, size: %lu\n",
        pid, mdisk->malloc_buf, num_blocks * block_size);

    fd = open(file_name, O_CREAT | O_TRUNC | O_RDWR,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (0 > fd)
    {
        SPDK_ERRLOG("Could not open %s.\n", file_name);
        malloc_disk_free(mdisk);
        return -EINVAL;
    }

    sprintf(writeBuf, "%d %lu %lu %lu", pid, bufAddr, startPage, pageCount);
    strLength = strlen(writeBuf);

    rc = write(fd, writeBuf, strLength);
    if (0 > rc)
    {
        SPDK_ERRLOG("Could not write on %s: %s\n", file_name, strerror(errno));
        close(fd);
        malloc_disk_free(mdisk);
        return -EINVAL;
    }

    close(fd);
    SPDK_INFOLOG(bdev_malloc,
        "Successfully written to \"%s\" about information of \"%s\"\n",
        file_name, name);
    return 0;
}

int
create_malloc_disk(struct spdk_bdev **bdev, const char *name, const struct spdk_uuid *uuid,
		   uint64_t num_blocks, uint32_t block_size)
{
	struct malloc_disk	*mdisk;
	int rc;

	if (num_blocks == 0) {
		SPDK_ERRLOG("Disk num_blocks must be greater than 0");
		return -EINVAL;
	}

	mdisk = calloc(1, sizeof(*mdisk));
	if (!mdisk) {
		SPDK_ERRLOG("mdisk calloc() failed\n");
		return -ENOMEM;
	}

	/*
	 * Allocate the large backend memory buffer from pinned memory.
	 *
	 * TODO: need to pass a hint so we know which socket to allocate
	 *  from on multi-socket systems.
	 */
	mdisk->malloc_buf = spdk_zmalloc(num_blocks * block_size, 2 * 1024 * 1024, NULL,
					 SPDK_ENV_LCORE_ID_ANY, SPDK_MALLOC_DMA);
	// TODO: Multi Array Optimization is necessary.
	// malloc_disk_base = mdisk->malloc_buf;	
        malloc_disk_base = NULL;
	if (!mdisk->malloc_buf) {
		SPDK_ERRLOG("malloc_buf spdk_zmalloc() failed\n");
		malloc_disk_free(mdisk);
		return -ENOMEM;
	}

	rc = backup_disk_info_for_restore(mdisk, name, num_blocks, block_size);
	if(rc != 0)
	{
		return rc;
	}

	if (name) {
		mdisk->disk.name = strdup(name);
	} else {
		/* Auto-generate a name */
		mdisk->disk.name = spdk_sprintf_alloc("Malloc%d", malloc_disk_count);
		malloc_disk_count++;
	}
	if (!mdisk->disk.name) {
		malloc_disk_free(mdisk);
		return -ENOMEM;
	}
	mdisk->disk.product_name = "Malloc disk";

	mdisk->disk.write_cache = 1;
	mdisk->disk.blocklen = block_size;
	mdisk->disk.blockcnt = num_blocks;
	if (uuid) {
		mdisk->disk.uuid = *uuid;
	} else {
		spdk_uuid_generate(&mdisk->disk.uuid);
	}

	mdisk->disk.ctxt = mdisk;
	mdisk->disk.fn_table = &malloc_fn_table;
	mdisk->disk.module = &malloc_if;

	rc = spdk_bdev_register(&mdisk->disk);
	if (rc) {
		malloc_disk_free(mdisk);
		return rc;
	}

	*bdev = &(mdisk->disk);

	TAILQ_INSERT_TAIL(&g_malloc_disks, mdisk, link);

	return rc;
}

void
delete_malloc_disk(struct spdk_bdev *bdev, spdk_delete_malloc_complete cb_fn, void *cb_arg)
{
	if (!bdev || bdev->module != &malloc_if) {
		cb_fn(cb_arg, -ENODEV);
		return;
	}

	spdk_bdev_unregister(bdev, cb_fn, cb_arg);
}

static int bdev_malloc_initialize(void)
{
	/* This needs to be reset for each reinitialization of submodules.
	 * Otherwise after enough devices or reinitializations the value gets too high.
	 * TODO: Make malloc bdev name mandatory and remove this counter. */
	malloc_disk_count = 0;
	return 0;
}

SPDK_LOG_REGISTER_COMPONENT(bdev_malloc)
