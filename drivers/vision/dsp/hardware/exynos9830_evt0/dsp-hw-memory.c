// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#include <linux/types.h>
#include <linux/ion_exynos.h>
#include <linux/exynos_iovmm.h>
#include <asm/cacheflush.h>

#include "dsp-log.h"
#include "hardware/dsp-system.h"
#include "hardware/dsp-memory.h"

int dsp_memory_map_buffer(struct dsp_memory *mem, struct dsp_buffer *buf)
{
	int ret;
	struct dma_buf *dbuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t iova;
	void *kvaddr;

	dsp_enter();
	if (buf->fd <= 0) {
		ret = -EINVAL;
		dsp_err("fd(%d) is invalid\n", buf->fd);
		goto p_err;
	}

	dbuf = dma_buf_get(buf->fd);
	if (IS_ERR(dbuf)) {
		ret = PTR_ERR(dbuf);
		dsp_err("dma_buf is invalid(%d/%d)\n", buf->fd, ret);
		goto p_err;
	}
	buf->dbuf = dbuf;

	if (buf->size + buf->offset > dbuf->size) {
		ret = -EINVAL;
		dsp_err("size is invalid(%zu/%u/%zu)\n",
				buf->size, buf->offset, dbuf->size);
		goto p_err_size;
	}
	buf->dbuf_size = dbuf->size;

	attach = dma_buf_attach(dbuf, mem->dev);
	if (IS_ERR(attach)) {
		ret = PTR_ERR(attach);
		dsp_err("Failed to attach dma_buf(%d)\n", ret);
		goto p_err_attach;
	}
	buf->attach = attach;

	sgt = dma_buf_map_attachment(attach, buf->dir);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		dsp_err("Failed to map attachment(%d)\n", ret);
		goto p_err_map_attach;
	}
	buf->sgt = sgt;

	iova = ion_iovmm_map(attach, 0, buf->dbuf_size, buf->dir, IOMMU_CACHE);
	if (IS_ERR_VALUE(iova)) {
		ret = (int)iova;
		dsp_err("Failed to map iova(%d)\n", ret);
		goto p_err_map_dva;
	}
	buf->iova = iova;

	if (buf->cached) {
		kvaddr = dma_buf_vmap(dbuf);
		if (IS_ERR(kvaddr)) {
			ret = PTR_ERR(kvaddr);
			dsp_err("Failed to map kvaddr(%d)\n", ret);
			goto p_err_kva;
		}
		buf->kvaddr = kvaddr;
	} else {
		buf->kvaddr = NULL;
	}

	dsp_leave();
	return 0;
p_err_kva:
	ion_iovmm_unmap(attach, iova);
p_err_map_dva:
	dma_buf_unmap_attachment(attach, sgt, buf->dir);
p_err_map_attach:
	dma_buf_detach(dbuf, attach);
p_err_attach:
p_err_size:
	dma_buf_put(dbuf);
p_err:
	return ret;
}

int dsp_memory_unmap_buffer(struct dsp_memory *mem, struct dsp_buffer *buf)
{
	dsp_enter();
	if (buf->kvaddr)
		dma_buf_vunmap(buf->dbuf, buf->kvaddr);

	ion_iovmm_unmap(buf->attach, buf->iova);
	dma_buf_unmap_attachment(buf->attach, buf->sgt, buf->dir);
	dma_buf_detach(buf->dbuf, buf->attach);
	dma_buf_put(buf->dbuf);
	dsp_leave();
	return 0;
}

int dsp_memory_sync_for_device(struct dsp_memory *mem, struct dsp_buffer *buf)
{
	dsp_enter();
	if (!buf->cached) {
		dsp_warn("It is not required to sync non-cacheable area(%d)\n",
				buf->fd);
		return 0;
	}

	if (!buf->kvaddr) {
		dsp_err("kvaddr is required to sync cacheable area(%d)\n",
				buf->fd);
		return -EINVAL;
	}

	__dma_map_area(buf->kvaddr + buf->offset, buf->size, buf->dir);

	dsp_leave();
	return 0;
}

int dsp_memory_sync_for_cpu(struct dsp_memory *mem, struct dsp_buffer *buf)
{
	dsp_enter();
	if (!buf->cached) {
		dsp_warn("It is not required to sync non-cacheable area(%d)\n",
				buf->fd);
		return 0;
	}

	if (!buf->kvaddr) {
		dsp_err("kvaddr is required to sync cacheable area(%d)\n",
				buf->fd);
		return -EINVAL;
	}

	__dma_unmap_area(buf->kvaddr + buf->offset, buf->size, buf->dir);

	dsp_leave();
	return 0;
}

static int __dsp_memory_iovmm_map_sg(struct dsp_memory *mem,
		struct dsp_priv_mem *pmem)
{
	size_t size;

	dsp_enter();
	size = iommu_map_sg(mem->domain, pmem->iova, pmem->sgt->sgl,
			pmem->sgt->nents, 0);
	if (!size) {
		dsp_err("Failed to map sg[%s]\n", pmem->name);
		return -ENOMEM;
	}

	if (size != pmem->size) {
		dsp_warn("pmem size(%zd) is different from mapped size(%zd)\n",
				pmem->size, size);
		pmem->size = size;
	}

	dsp_leave();
	return 0;
}

extern void exynos_sysmmu_tlb_invalidate(struct iommu_domain *iommu_domain,
		dma_addr_t d_start, size_t size);

static int __dsp_memory_iovmm_unmap(struct dsp_memory *mem,
		struct dsp_priv_mem *pmem)
{
	size_t size;

	dsp_enter();
	size = iommu_unmap(mem->domain, pmem->iova, pmem->size);
	if (!size) {
		dsp_err("Failed to unmap iovmm(%zd)[%s]\n", size, pmem->name);
		return size;
	}
	exynos_sysmmu_tlb_invalidate(mem->domain, pmem->iova, pmem->size);

	dsp_leave();
	return 0;
}

int dsp_memory_ion_alloc(struct dsp_memory *mem, struct dsp_priv_mem *pmem)
{
	int ret;
	const char *heap_name = "ion_system_heap";
	struct dma_buf *dbuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	dma_addr_t iova;
	void *kvaddr;

	dsp_enter();
	dbuf = ion_alloc_dmabuf(heap_name, pmem->size, pmem->flags);
	if (IS_ERR(dbuf)) {
		ret = PTR_ERR(dbuf);
		dsp_err("Failed to allocate dma_buf(%d)[%s]\n",
				ret, pmem->name);
		goto p_err_alloc;
	}
	pmem->dbuf = dbuf;
	pmem->dbuf_size = dbuf->size;

	attach = dma_buf_attach(dbuf, mem->dev);
	if (IS_ERR(attach)) {
		ret = PTR_ERR(attach);
		dsp_err("Failed to attach dma_buf(%d)[%s]\n",
				ret, pmem->name);
		goto p_err_attach;
	}
	pmem->attach = attach;

	sgt = dma_buf_map_attachment(attach, pmem->dir);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		dsp_err("Failed to map attachment(%d)[%s]\n",
				ret, pmem->name);
		goto p_err_map_attach;
	}
	pmem->sgt = sgt;

	if (pmem->kmap) {
		kvaddr = dma_buf_vmap(dbuf);
		if (IS_ERR(kvaddr)) {
			ret = PTR_ERR(kvaddr);
			dsp_err("Failed to map kvaddr(%d)[%s]\n",
					ret, pmem->name);
			goto p_err_kmap;
		}
		pmem->kvaddr = kvaddr;
	} else {
		pmem->kvaddr = NULL;
	}

	if (pmem->fixed_iova) {
		ret = __dsp_memory_iovmm_map_sg(mem, pmem);
		if (ret)
			goto p_err_map_dva;
	} else {
		iova = ion_iovmm_map(attach, 0, pmem->size, pmem->dir,
				IOMMU_CACHE);
		if (IS_ERR_VALUE(iova)) {
			ret = (int)iova;
			dsp_err("Failed to map iova(%d)[%s]\n",
					ret, pmem->name);
			goto p_err_map_dva;
		}
		pmem->iova = iova;
	}

	if (pmem->kmap)
		dsp_info("[%16s] %#lx/%#x/%zuKB\n", pmem->name, (long)kvaddr,
				(int)pmem->iova, pmem->size / SZ_1K);
	else
		dsp_info("[%16s] %#x/%zuKB\n", pmem->name, (int)pmem->iova,
				pmem->size / SZ_1K);

	dsp_leave();
	return 0;
p_err_map_dva:
	if (pmem->kmap)
		dma_buf_vunmap(dbuf, kvaddr);
p_err_kmap:
	dma_buf_unmap_attachment(attach, sgt, pmem->dir);
p_err_map_attach:
	dma_buf_detach(dbuf, attach);
p_err_attach:
	dma_buf_put(dbuf);
p_err_alloc:
	return ret;
}

void dsp_memory_ion_free(struct dsp_memory *mem,
		struct dsp_priv_mem *pmem)
{
	dsp_enter();
	if (pmem->fixed_iova)
		__dsp_memory_iovmm_unmap(mem, pmem);
	else
		ion_iovmm_unmap(pmem->attach, pmem->iova);

	if (pmem->kmap)
		dma_buf_vunmap(pmem->dbuf, pmem->kvaddr);

	dma_buf_unmap_attachment(pmem->attach, pmem->sgt, pmem->dir);
	dma_buf_detach(pmem->dbuf, pmem->attach);
	dma_buf_put(pmem->dbuf);
	dsp_leave();
}

int dsp_memory_open(struct dsp_memory *mem)
{
	int ret;

	dsp_enter();
	ret = dsp_memory_ion_alloc(mem, &mem->priv_mem[DSP_PRIV_MEM_FW]);
	if (ret)
		goto p_err_alloc_fw;

	dsp_leave();
	return 0;
p_err_alloc_fw:
	return ret;
}

int dsp_memory_close(struct dsp_memory *mem)
{
	dsp_enter();
	dsp_memory_ion_free(mem, &mem->priv_mem[DSP_PRIV_MEM_FW]);
	dsp_leave();
	return 0;
}

int dsp_memory_probe(struct dsp_system *sys)
{
	int ret;
	struct dsp_memory *mem;
	struct dsp_priv_mem *pmem_list, *pmem;

	dsp_enter();
	mem = &sys->memory;
	mem->dev = sys->dev;

	dma_set_mask(mem->dev, DMA_BIT_MASK(36));
	mem->domain = get_domain_from_dev(mem->dev);

	pmem_list = kcalloc(DSP_PRIV_MEM_COUNT, sizeof(*pmem_list), GFP_KERNEL);
	if (!pmem_list) {
		ret = -ENOMEM;
		dsp_err("Failed to alloc dsp_priv_mem\n");
		goto p_err;
	}

	pmem = &pmem_list[DSP_PRIV_MEM_FW];
	snprintf(pmem->name, DSP_PRIV_MEM_NAME_LEN, "ca5_bin");
	pmem->size = PAGE_ALIGN(DSP_CA5_BIN_SIZE);
	pmem->min_size = PAGE_ALIGN(DSP_CA5_BIN_SIZE);
	pmem->max_size = PAGE_ALIGN(DSP_MEMORY_MAX_SIZE);
	pmem->flags = 0;
	pmem->dir = DMA_TO_DEVICE;
	pmem->kmap = true;
	pmem->fixed_iova = true;
	pmem->iova = DSP_CA5_BIN_IOVA;

	mem->priv_mem = pmem_list;

	dsp_leave();
	return 0;
p_err:
	return ret;
}

void dsp_memory_remove(struct dsp_memory *mem)
{
	dsp_enter();
	kfree(mem->priv_mem);
	dsp_leave();
}
