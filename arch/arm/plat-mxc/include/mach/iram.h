/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <linux/errno.h>
#include <linux/genalloc.h>

#ifdef CONFIG_IRAM_ALLOC

int __init iram_init(phys_addr_t base, size_t size);

extern struct gen_pool *mxc_iram_pool;

static inline void *iram_alloc(size_t size, phys_addr_t *phys)
{
	unsigned long addr = gen_pool_alloc(iram_pool, size);

	*phys = gen_pool_virt_to_phys(iram_pool, addr);

	return (void *)addr;
}

static inline void iram_free(void *addr, size_t size)
{
	gen_pool_free(iram_pool, (unsigned long)addr, size);
}

#else

static inline int __init iram_init(phys_addr_t base, size_t size)
{
	return -ENOMEM;
}

static inline void *iram_alloc(size_t size, phys_addr_t *phys)
{
	*phys = (phys_addr_t)-1ULL;
	return NULL;
}

static inline void iram_free(void *addr, size_t size) {}

#endif
