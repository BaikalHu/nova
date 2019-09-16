src-prefix              := src/
src-y                   += heap.c mem.c memtry.c
src-$(CONFIG_MMU)       += mmu.c
src-$(CONFIG_PAGE_POOL) += page.c
src-$(CONFIG_MEMPOOL)   += mempool.c
inc-g                   += h/