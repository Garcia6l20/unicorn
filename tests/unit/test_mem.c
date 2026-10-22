#include "unicorn_test.h"

static void test_map_correct(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map(uc, 0x40000, 0x1000 * 16, UC_PROT_ALL)); // [0x40000, 0x50000]
    OK(uc_mem_map(uc, 0x60000, 0x1000 * 16, UC_PROT_ALL)); // [0x60000, 0x70000]
    OK(uc_mem_map(uc, 0x20000, 0x1000 * 16, UC_PROT_ALL)); // [0x20000, 0x30000]
    uc_assert_err(UC_ERR_MAP,
                  uc_mem_map(uc, 0x10000, 0x2000 * 16, UC_PROT_ALL));
    uc_assert_err(UC_ERR_MAP,
                  uc_mem_map(uc, 0x25000, 0x1000 * 16, UC_PROT_ALL));
    uc_assert_err(UC_ERR_MAP,
                  uc_mem_map(uc, 0x35000, 0x1000 * 16, UC_PROT_ALL));
    uc_assert_err(UC_ERR_MAP,
                  uc_mem_map(uc, 0x45000, 0x1000 * 16, UC_PROT_ALL));
    uc_assert_err(UC_ERR_MAP,
                  uc_mem_map(uc, 0x55000, 0x2000 * 16, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x35000, 0x5000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x50000, 0x5000, UC_PROT_ALL));

    OK(uc_close(uc));
}

static void test_map_wrapping(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    uc_assert_err(UC_ERR_ARG, uc_mem_map(uc, (~0ll - 0x4000) & ~0xfff, 0x8000,
                                         UC_PROT_ALL));

    OK(uc_close(uc));
}

static void test_mem_protect(void)
{
    uc_engine *qc;
    int r_eax = 0x2000;
    int r_esi = 0xdeadbeef;
    uint32_t mem;
    // add [eax + 4], esi
    char code[] = {0x01, 0x70, 0x04};

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &qc));
    OK(uc_reg_write(qc, UC_X86_REG_EAX, &r_eax));
    OK(uc_reg_write(qc, UC_X86_REG_ESI, &r_esi));
    OK(uc_mem_map(qc, 0x1000, 0x1000, UC_PROT_READ | UC_PROT_EXEC));
    OK(uc_mem_map(qc, 0x2000, 0x1000, UC_PROT_READ));
    OK(uc_mem_protect(qc, 0x2000, 0x1000, UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_mem_write(qc, 0x1000, code, sizeof(code)));

    OK(uc_emu_start(qc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 1));
    OK(uc_mem_read(qc, 0x2000 + 4, &mem, 4));

    TEST_CHECK(LEINT32(mem) == 0xdeadbeef);

    OK(uc_close(qc));
}

static void test_splitting_mem_unmap(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));

    OK(uc_mem_map(uc, 0x20000, 0x1000, UC_PROT_NONE));
    OK(uc_mem_map(uc, 0x21000, 0x2000, UC_PROT_NONE));

    OK(uc_mem_unmap(uc, 0x21000, 0x1000));

    OK(uc_close(uc));
}

static uint64_t test_splitting_mmio_unmap_read_callback(uc_engine *uc,
                                                        uint64_t offset,
                                                        unsigned size,
                                                        void *user_data)
{
    TEST_CHECK(offset == 4);
    TEST_CHECK(size == 4);

    return 0x19260817;
}

static void test_splitting_mmio_unmap(void)
{
    uc_engine *uc;
    // mov ecx, [0x3004] <-- normal read
    // mov ebx, [0x4004] <-- mmio read
    char code[] = "\x8b\x0d\x04\x30\x00\x00\x8b\x1d\x04\x40\x00\x00";
    int r_ecx, r_ebx;
    int bytes = LEINT32(0xdeadbeef);

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));

    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));

    OK(uc_mmio_map(uc, 0x3000, 0x2000, test_splitting_mmio_unmap_read_callback,
                   NULL, NULL, NULL));

    // Map a ram area instead
    OK(uc_mem_unmap(uc, 0x3000, 0x1000));
    OK(uc_mem_map(uc, 0x3000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x3004, &bytes, 4));

    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_X86_REG_ECX, &r_ecx));
    OK(uc_reg_read(uc, UC_X86_REG_EBX, &r_ebx));

    TEST_CHECK(r_ecx == 0xdeadbeef);
    TEST_CHECK(r_ebx == 0x19260817);

    OK(uc_close(uc));
}

static void test_mem_protect_map_ptr(void)
{
    uc_engine *uc;
    uint64_t val = 0x114514;
    uint8_t *data1 = NULL;
    uint8_t *data2 = NULL;
    uint64_t mem;

    data1 = calloc(sizeof(*data1), 0x4000);
    data2 = calloc(sizeof(*data2), 0x2000);

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));

    OK(uc_mem_map_ptr(uc, 0x4000, 0x4000, UC_PROT_ALL, data1));
    OK(uc_mem_unmap(uc, 0x6000, 0x2000));
    OK(uc_mem_map_ptr(uc, 0x6000, 0x2000, UC_PROT_ALL, data2));

    OK(uc_mem_write(uc, 0x6004, &val, 8));
    OK(uc_mem_protect(uc, 0x6000, 0x1000, UC_PROT_READ));
    OK(uc_mem_read(uc, 0x6004, (void *)&mem, 8));

    TEST_CHECK(val == mem);

    OK(uc_close(uc));

    free(data2);
    free(data1);
}

static void test_map_at_the_end(void)
{
    uc_engine *uc;
    uint8_t mem[0x1000];

    memset(mem, 0xff, 0x100);

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));

    OK(uc_mem_map(uc, 0xfffffffffffff000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0xfffffffffffff000, mem, sizeof(mem)));

    uc_assert_err(UC_ERR_WRITE_UNMAPPED,
                  uc_mem_write(uc, 0xffffffffffffff00, mem, sizeof(mem)));
    uc_assert_err(UC_ERR_WRITE_UNMAPPED, uc_mem_write(uc, 0, mem, sizeof(mem)));

    OK(uc_close(uc));
}

static void test_map_wrap(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));

    uc_assert_err(UC_ERR_ARG,
                  uc_mem_map(uc, 0xfffffffffffff000, 0x2000, UC_PROT_ALL));

    OK(uc_close(uc));
}

static void test_map_big_memory(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    uint64_t requested_size = 0xfffffffffffff000; // assume 4K page size
#else
    long ps = sysconf(_SC_PAGESIZE);
    uint64_t requested_size = (uint64_t)(-ps);
#endif

    uc_assert_err(UC_ERR_NOMEM,
                  uc_mem_map(uc, 0x0, requested_size, UC_PROT_ALL));

    OK(uc_close(uc));
}

static void test_mem_protect_remove_exec_callback(uc_engine *uc, uint64_t addr,
                                                  size_t size, void *data)
{
    uint64_t *p = (uint64_t *)data;
    (*p)++;

    OK(uc_mem_protect(uc, 0x2000, 0x1000, UC_PROT_READ));
}

static void test_mem_protect_remove_exec(void)
{
    uc_engine *uc;
    char code[] = "\x90\xeb\x00\x90";
    uc_hook hk;
    uint64_t called_count = 0;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_ALL));

    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));
    OK(uc_hook_add(uc, &hk, UC_HOOK_BLOCK,
                   test_mem_protect_remove_exec_callback, (void *)&called_count,
                   1, 0));

    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));

    TEST_CHECK(called_count == 2);

    OK(uc_close(uc));
}

static uint64_t test_mem_protect_mmio_read_cb(struct uc_struct *uc,
                                              uint64_t addr, unsigned size,
                                              void *user_data)
{
    TEST_CHECK(addr == 0x20); // note, it's not 0x1020

    *(uint64_t *)user_data += 1;
    return 0x114514;
}

static void test_mem_protect_mmio_write_cb(struct uc_struct *uc, uint64_t addr,
                                           unsigned size, uint64_t data,
                                           void *user_data)
{
    TEST_CHECK(false);
    return;
}

static void test_mem_protect_mmio(void)
{
    uc_engine *uc;
    // mov eax, [0x2020]; mov [0x2020], eax
    char code[] = "\xa1\x20\x20\x00\x00\x00\x00\x00\x00\xa3\x20\x20\x00\x00\x00"
                  "\x00\x00\x00";
    uint64_t called = 0;
    uint64_t r_eax;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map(uc, 0x8000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x8000, code, sizeof(code) - 1));

    OK(uc_mmio_map(uc, 0x1000, 0x3000, test_mem_protect_mmio_read_cb,
                   (void *)&called, test_mem_protect_mmio_write_cb,
                   (void *)&called));
    OK(uc_mem_protect(uc, 0x2000, 0x1000, UC_PROT_READ));

    uc_assert_err(UC_ERR_WRITE_PROT,
                  uc_emu_start(uc, 0x8000, 0x8000 + sizeof(code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_X86_REG_RAX, &r_eax));

    TEST_CHECK(called == 1);
    TEST_CHECK(r_eax == 0x114514);

    OK(uc_close(uc));
}

static void test_snapshot(void)
{
    uc_engine *uc;
    uc_context *c0, *c1;
    uint32_t mem;
    uint8_t code_data;
    // mov eax, [0x2020]; inc eax; mov [0x2020], eax
    char code[] = "\xa1\x20\x20\x00\x00\x00\x00\x00\x00\xff\xc0\xa3\x20\x20\x00"
                  "\x00\x00\x00\x00\x00";

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_context_alloc(uc, &c0));
    OK(uc_context_alloc(uc, &c1));
    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY));
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));

    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_ALL));
    OK(uc_context_save(uc, c0));

    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 1);
    OK(uc_context_save(uc, c1));
    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 2);
    OK(uc_context_restore(uc, c1));

    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 1);
    OK(uc_context_restore(uc, c0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 0);

    OK(uc_mem_read(uc, 0x1000, &code_data, sizeof(code_data)));
    TEST_CHECK(code_data == 0xa1);

    OK(uc_context_free(c0));
    OK(uc_context_free(c1));
    OK(uc_close(uc));
}

static bool test_snapshot_with_vtlb_callback(uc_engine *uc, uint64_t addr,
                                             uc_mem_type type,
                                             uc_tlb_entry *result,
                                             void *user_data)
{
    result->paddr = addr - 0x400000000;
    result->perms = UC_PROT_ALL;
    return true;
}

static void test_snapshot_with_vtlb(void)
{
    uc_engine *uc;
    uc_context *c0, *c1;
    uint32_t mem;
    uc_hook hook;

    // mov eax, [0x2020]; inc eax; mov [0x2020], eax
    char code[] = "\xA1\x20\x20\x00\x00\x04\x00\x00\x00\xFF\xC0\xA3\x20\x20\x00"
                  "\x00\x04\x00\x00\x00";

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));

    // Allocate contexts
    OK(uc_context_alloc(uc, &c0));
    OK(uc_context_alloc(uc, &c1));
    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY));

    OK(uc_ctl_tlb_mode(uc, UC_TLB_VIRTUAL));
    OK(uc_hook_add(uc, &hook, UC_HOOK_TLB_FILL,
                   test_snapshot_with_vtlb_callback, NULL, 1, 0));

    // Map physical memory
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_EXEC | UC_PROT_READ));
    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));
    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_ALL));

    // Initial context save
    OK(uc_context_save(uc, c0));

    OK(uc_emu_start(uc, 0x400000000 + 0x1000,
                    0x400000000 + 0x1000 + sizeof(code) - 1, 0, 0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 1);
    OK(uc_context_save(uc, c1));
    OK(uc_emu_start(uc, 0x400000000 + 0x1000,
                    0x400000000 + 0x1000 + sizeof(code) - 1, 0, 0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 2);
    OK(uc_context_restore(uc, c1));
    // TODO check mem
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 1);
    OK(uc_context_restore(uc, c0));
    OK(uc_mem_read(uc, 0x2020, &mem, sizeof(mem)));
    TEST_CHECK(LEINT32(mem) == 0);
    // TODO check mem

    OK(uc_context_free(c0));
    OK(uc_context_free(c1));
    OK(uc_close(uc));
}

static void test_context_snapshot(void)
{
    uc_engine *uc;
    uc_context *ctx;
    uint64_t baseaddr = 0xfffff1000;
    uint64_t offset = 0x10;
    uint64_t tmp = 1;
    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY | UC_CTL_CONTEXT_CPU));
    OK(uc_mem_map(uc, baseaddr, 0x1000, UC_PROT_ALL));
    OK(uc_context_alloc(uc, &ctx));
    OK(uc_context_save(uc, ctx));

    OK(uc_mem_write(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    OK(uc_mem_read(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 1);
    OK(uc_context_restore(uc, ctx));
    OK(uc_mem_read(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 0);

    tmp = 2;
    OK(uc_mem_write(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    OK(uc_mem_read(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 2);
    OK(uc_context_restore(uc, ctx));
    OK(uc_mem_read(uc, baseaddr + offset, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 0);

    OK(uc_context_free(ctx));
    OK(uc_close(uc));
}

static void test_snapshot_unmap(void)
{
    uc_engine *uc;
    uc_context *ctx;
    uint64_t tmp;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY | UC_CTL_CONTEXT_CPU));
    OK(uc_mem_map(uc, 0x1000, 0x2000, UC_PROT_ALL));

    tmp = 1;
    OK(uc_mem_write(uc, 0x1000, &tmp, sizeof(tmp)));
    tmp = 2;
    OK(uc_mem_write(uc, 0x2000, &tmp, sizeof(tmp)));

    OK(uc_context_alloc(uc, &ctx));
    OK(uc_context_save(uc, ctx));

    uc_assert_err(UC_ERR_ARG, uc_mem_unmap(uc, 0x1000, 0x1000));
    OK(uc_mem_unmap(uc, 0x1000, 0x2000));
    uc_assert_err(UC_ERR_READ_UNMAPPED,
                  uc_mem_read(uc, 0x1000, &tmp, sizeof(tmp)));
    uc_assert_err(UC_ERR_READ_UNMAPPED,
                  uc_mem_read(uc, 0x2000, &tmp, sizeof(tmp)));

    OK(uc_context_restore(uc, ctx));
    OK(uc_mem_read(uc, 0x1000, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 1);
    OK(uc_mem_read(uc, 0x2000, &tmp, sizeof(tmp)));
    TEST_CHECK(tmp == 2);

    OK(uc_context_free(ctx));
    OK(uc_close(uc));
}

static void parts_increment(size_t idx, char parts[3])
{
    if (idx && idx % 3 == 0) {
        if (++parts[2] > '9') {
            parts[2] = '0';
            if (++parts[1] > 'z') {
                parts[1] = 'a';
                if (++parts[0] > 'Z')
                    parts[0] = 'A';
            }
        }
    }
}

// Create a pattern string. It works the same as
// https://github.com/rapid7/metasploit-framework/blob/master/tools/exploit/pattern_create.rb
static void pattern_create(char *buf, size_t len)
{
    char parts[] = {'A', 'a', '0'};
    size_t i;

    for (i = 0; i < len; i++) {
        buf[i] = parts[i % 3];
        parts_increment(i, parts);
    }
}

static bool pattern_verify(const char *buf, size_t len)
{
    char parts[] = {'A', 'a', '0'};
    size_t i;

    for (i = 0; i < len; i++) {
        if (buf[i] != parts[i % 3])
            return false;
        parts_increment(i, parts);
    }

    return true;
}

// Test for reading and writing memory block that are bigger than INT_MAX.
static void test_mem_read_and_write_large_memory_block(void)
{
    uc_engine *uc;
    uint64_t mem_addr = 0x1000000;
    uint64_t mem_size = 0x9f000000;
    char *pmem = NULL;

    if (sizeof(void *) < 8) {
        // Don't perform the test on a 32-bit platforms since we may not have
        // enough memory space.
        return;
    }
    // Android CI/CD services do not have enough memory capacity for this
    // test to work. Executing it will result in a permanent loop with the
    // low memory killer daemon. 
#ifdef __ANDROID__
    return;
#endif 

    OK(uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &uc));
    OK(uc_mem_map(uc, mem_addr, mem_size, UC_PROT_ALL));

    pmem = malloc(mem_size);
    if (TEST_CHECK(pmem != NULL)) {
        pattern_create(pmem, mem_size);

        OK(uc_mem_write(uc, mem_addr, pmem, mem_size));
        memset(pmem, 'a', mem_size);
        OK(uc_mem_read(uc, mem_addr, pmem, mem_size));
        TEST_CHECK(pattern_verify(pmem, mem_size));
        free(pmem);
    }

    OK(uc_mem_unmap(uc, mem_addr, mem_size));
    OK(uc_close(uc));
}

static bool test_v2p_tlb_fill(uc_engine *uc, uint64_t addr, uc_mem_type type,
                               uc_tlb_entry *result, void *user_data)               
{
    if (type != UC_MEM_READ)
        return false;
    result->paddr = addr;
    result->perms = UC_PROT_READ;
    return true;
}

static void test_virtual_to_physical(void)
{
    uc_engine *uc;
    uc_hook hook;
    uint64_t res;

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_ctl_tlb_mode(uc, UC_TLB_VIRTUAL));
    OK(uc_hook_add(uc, &hook, UC_HOOK_TLB_FILL, test_v2p_tlb_fill, NULL, 1, 0));

    OK(uc_vmem_translate(uc, 0x1000, UC_PROT_READ, &res));
    uc_assert_err(UC_ERR_WRITE_PROT,
                  uc_vmem_translate(uc, 0x1000, UC_PROT_WRITE, &res));
    OK(uc_close(uc));
}

static bool test_virtual_write_tlb_fill(uc_engine *uc, uint64_t addr, uc_mem_type type,
                                        uc_tlb_entry *result, void *user_data)
{
    if (addr < 0x1000)
        return false;
    result->paddr = addr - 0x1000;
    result->perms = UC_PROT_ALL;
    return true;
}

static void test_virtual_write(void)
{
    uc_engine *uc;
    uc_hook hook;
    uint64_t rax = 21;
    uint64_t res = 0;
    /*
     * mov rax, [0x2000]
     */
    char code[] = { 0x48, 0x8B, 0x04, 0x25, 0x00, 0x20, 0x00, 0x00 };

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_ctl_tlb_mode(uc, UC_TLB_VIRTUAL));
    OK(uc_hook_add(uc, &hook, UC_HOOK_TLB_FILL, test_virtual_write_tlb_fill, NULL, 1, 0));
    OK(uc_mem_map(uc, 0x0, 0x2000, UC_PROT_ALL));

    OK(uc_vmem_write(uc, 0x1000, UC_PROT_EXEC, code, sizeof(code)));
    OK(uc_vmem_write(uc, 0x2000, UC_PROT_READ, &rax, sizeof(rax)));

    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code), 0, 1));
    OK(uc_reg_read(uc, UC_X86_REG_RAX, &res));
    TEST_CHECK(rax == res);

    OK(uc_close(uc));
}

// Regression test for issue #2321. With two legal mappings (top page and
// page 0), an address+size that wraps used to let check_mem_area() walk
// from the top of the address space back to 0 and treat the wrapping
// range as fully mapped. uc_mem_read then memcpy'd 0x2000 bytes into a
// 0x1000 buffer; uc_mem_unmap silently dropped both regions. After the
// fix, check_mem_area() rejects the wrap up front so the four entry
// points fall back to their existing not-mapped error codes.
static void test_mem_addr_size_wraparound(void)
{
    uc_engine *uc;
    uint8_t buf[0x1000];

    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map(uc, 0,                     0x1000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0xFFFFFFFFFFFFF000ULL, 0x1000, UC_PROT_ALL));

    uc_assert_err(UC_ERR_READ_UNMAPPED,
        uc_mem_read(uc, 0xFFFFFFFFFFFFF000ULL, buf, 0x2000));
    uc_assert_err(UC_ERR_WRITE_UNMAPPED,
        uc_mem_write(uc, 0xFFFFFFFFFFFFF000ULL, buf, 0x2000));
    uc_assert_err(UC_ERR_NOMEM,
        uc_mem_unmap(uc, 0xFFFFFFFFFFFFF000ULL, 0x2000));
    uc_assert_err(UC_ERR_NOMEM,
        uc_mem_protect(uc, 0xFFFFFFFFFFFFF000ULL, 0x2000, UC_PROT_READ));

    // The non-wrapping single-page case must still work for both regions.
    OK(uc_mem_read(uc, 0xFFFFFFFFFFFFF000ULL, buf, 0x1000));
    OK(uc_mem_read(uc, 0,                     buf, 0x1000));

    OK(uc_close(uc));
}

/* SMC test: verifies that a store which overwrites code on
 * the same page invalidates any cached TBs translated from that
 * page, so subsequent execution sees the new instructions.
 */
static void test_smc(void)
{
    uc_engine *uc;
    uint64_t r_rax;
    uint64_t r_rsp;

    char code[] = (                    // 00: do_inc_dec:
        "\x48\xff\xc0"                 // 00:    inc %rax
        "\xc3"                         // 03:    ret
        "\xe8\xf7\xff\xff\xff"         // 04: call do_inc_dec
        "\xc6\x05\xf2\xff\xff\xff\xc8" // 09: movb $0xc8, -0xe(%rip)
        "\xe8\xeb\xff\xff\xff"         // 16: call do_inc_dec
    );

    r_rax = 0x1234;
    r_rsp = 0x5000;
    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map  (uc, 0x0,    0x1000, UC_PROT_ALL));                // text
    OK(uc_mem_map  (uc, 0x4000, 0x1000, UC_PROT_READ|UC_PROT_WRITE)); // stack
    OK(uc_mem_write(uc, 0x0,    code,   sizeof(code)-1));
    OK(uc_reg_write(uc, UC_X86_REG_RAX, &r_rax));
    OK(uc_reg_write(uc, UC_X86_REG_RSP, &r_rsp));
    OK(uc_emu_start(uc, 0x4, sizeof(code)-1, 0, 0));

    OK(uc_reg_read(uc, UC_X86_REG_RAX, &r_rax));
    OK(uc_mem_read(uc, 0x0, code, sizeof(code)-1));
    TEST_CHECK(r_rax == 0x1234);
    TEST_CHECK((code[2] & 0xFF) == 0xC8);

    OK(uc_close(uc));
}

/*
 * Ensures that a code section initially as RW, if marked later as RX
 * still works as expected
 */
static void test_tlbdirty_exec(void)
{
    uc_engine *uc;
    uint32_t eax;

    char code[] = ("\x40"); // inc eax
    eax = 41;
    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));
    OK(uc_reg_write  (uc, UC_X86_REG_EAX, &eax));
    OK(uc_mem_map    (uc, 0x0, 0x1000, UC_PROT_READ|UC_PROT_WRITE));
    OK(uc_mem_write  (uc, 0x0, code,   sizeof(code)-1));
    OK(uc_mem_protect(uc, 0x0, 0x1000, UC_PROT_READ|UC_PROT_EXEC));
    OK(uc_emu_start  (uc, 0x0, sizeof(code)-1, 0, 0));

    OK(uc_reg_read(uc, UC_X86_REG_EAX, &eax));
    TEST_CHECK(eax == 42);

    OK(uc_close(uc));
}

typedef struct {
    int calls;
    bool drop;
    bool handled;
} write_prot_hook_state;

static bool test_write_prot_hook(uc_engine *uc, uc_mem_type type,
                                 uint64_t address, int size, int64_t value,
                                 void *user_data)
{
    write_prot_hook_state *state = (write_prot_hook_state *)user_data;

    TEST_CHECK(type == UC_MEM_WRITE_PROT);
    TEST_CHECK(address == 0x1800 + 4 * state->calls);
    TEST_CHECK(size == 4);
    TEST_CHECK(value == 0x11223344);
    state->calls++;
    if (state->drop) {
        OK(uc_ctl_drop_store(uc));
    }
    return state->handled;
}

static const char write_prot_code[] =
    "\xb8\x44\x33\x22\x11"
    "\xa3\x00\x18\x00\x00\x00\x00\x00\x00"
    "\xa3\x04\x18\x00\x00\x00\x00\x00\x00"
    "\xbb\x01\x00\x00\x00";

static const uint8_t write_prot_fill[8] = {0xaa, 0xaa, 0xaa, 0xaa,
                                           0xaa, 0xaa, 0xaa, 0xaa};

static const uint8_t write_prot_stored[8] = {0x44, 0x33, 0x22, 0x11,
                                             0x44, 0x33, 0x22, 0x11};

static uc_engine *open_write_prot_engine(uint8_t *backing)
{
    uc_engine *uc;

    memcpy(backing, write_prot_code, sizeof(write_prot_code) - 1);
    memcpy(backing + 0x800, write_prot_fill, sizeof(write_prot_fill));
    OK(uc_open(UC_ARCH_X86, UC_MODE_64, &uc));
    OK(uc_mem_map_ptr(uc, 0x1000, 0x1000, UC_PROT_READ | UC_PROT_EXEC,
                      backing));
    return uc;
}

static void run_write_prot_engine(uc_engine *uc, uc_err expected_err,
                                  uint8_t mem[8], uint64_t *r_rbx)
{
    *r_rbx = 0;
    OK(uc_reg_write(uc, UC_X86_REG_RBX, r_rbx));
    uc_assert_err(expected_err,
                  uc_emu_start(uc, 0x1000,
                               0x1000 + sizeof(write_prot_code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_X86_REG_RBX, r_rbx));
    OK(uc_mem_read(uc, 0x1800, mem, 8));
}

static void run_write_prot_store(write_prot_hook_state *state,
                                 uc_err expected_err, uint8_t mem[8],
                                 uint64_t *r_rbx)
{
    uint8_t *backing = calloc(1, 0x1000);
    uc_engine *uc = open_write_prot_engine(backing);
    uc_hook hook;

    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE_PROT, test_write_prot_hook,
                   state, 1, 0));
    run_write_prot_engine(uc, expected_err, mem, r_rbx);
    OK(uc_close(uc));
    free(backing);
}

static void test_mem_write_prot_drop_store(void)
{
    write_prot_hook_state state = {0, true, true};
    uint8_t mem[8];
    uint64_t r_rbx;

    run_write_prot_store(&state, UC_ERR_OK, mem, &r_rbx);
    TEST_CHECK(state.calls == 2);
    TEST_CHECK(memcmp(mem, write_prot_fill, sizeof(mem)) == 0);
    TEST_CHECK(r_rbx == 1);

    state = (write_prot_hook_state){0, true, false};
    run_write_prot_store(&state, UC_ERR_WRITE_PROT, mem, &r_rbx);
    TEST_CHECK(state.calls == 1);
    TEST_CHECK(memcmp(mem, write_prot_fill, sizeof(mem)) == 0);
    TEST_CHECK(r_rbx == 0);
}

static void test_mem_write_prot_handled_commits(void)
{
    write_prot_hook_state state = {0, false, true};
    uint8_t mem[8];
    uint64_t r_rbx;

    run_write_prot_store(&state, UC_ERR_OK, mem, &r_rbx);
    TEST_CHECK(state.calls == 2);
    TEST_CHECK(memcmp(mem, write_prot_stored, sizeof(mem)) == 0);
    TEST_CHECK(r_rbx == 1);
}

static void test_mem_write_prot_drop_store_scope(void)
{
    write_prot_hook_state state = {0, true, false};
    write_prot_hook_state second = {0, false, true};
    uint8_t *backing = calloc(1, 0x1000);
    uc_engine *uc = open_write_prot_engine(backing);
    uc_hook hook;
    uc_hook second_hook;
    uint8_t mem[8];
    uint64_t r_rbx;

    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE_PROT, test_write_prot_hook,
                   &state, 1, 0));
    run_write_prot_engine(uc, UC_ERR_WRITE_PROT, mem, &r_rbx);
    TEST_CHECK(state.calls == 1);

    state = (write_prot_hook_state){0, false, true};
    OK(uc_ctl_drop_store(uc));
    run_write_prot_engine(uc, UC_ERR_OK, mem, &r_rbx);
    TEST_CHECK(state.calls == 2);
    TEST_CHECK(memcmp(mem, write_prot_stored, sizeof(mem)) == 0);
    TEST_CHECK(r_rbx == 1);
    OK(uc_close(uc));

    uc = open_write_prot_engine(backing);
    state = (write_prot_hook_state){0, true, false};
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE_PROT, test_write_prot_hook,
                   &state, 1, 0));
    OK(uc_hook_add(uc, &second_hook, UC_HOOK_MEM_WRITE_PROT,
                   test_write_prot_hook, &second, 1, 0));
    run_write_prot_engine(uc, UC_ERR_OK, mem, &r_rbx);
    TEST_CHECK(state.calls == 2);
    TEST_CHECK(second.calls == 2);
    TEST_CHECK(memcmp(mem, write_prot_stored, sizeof(mem)) == 0);
    TEST_CHECK(r_rbx == 1);
    OK(uc_close(uc));
    free(backing);
}

static void test_mem_write_hook_flush_tlb_cb(uc_engine *uc, uc_mem_type type,
                                             uint64_t address, int size,
                                             int64_t value, void *user_data)
{
    int *calls = (int *)user_data;
    (*calls)++;
    OK(uc_ctl_flush_tlb(uc));
}

static void test_mem_write_hook_flush_tlb(void)
{
    uc_engine *uc;
    uc_hook hook;
    int calls = 0;
    uint32_t mem[2] = {0, 0};
    char code[] = "\xc7\x05\x00\x20\x00\x00\x78\x56\x34\x12"
                  "\xc7\x05\x04\x20\x00\x00\xf0\xde\xbc\x9a";

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE,
                   test_mem_write_hook_flush_tlb_cb, &calls, 0x2000, 0x2007));
    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));

    OK(uc_mem_read(uc, 0x2000, mem, sizeof(mem)));
    TEST_CHECK(calls == 2);
    TEST_CHECK(mem[0] == 0x12345678);
    TEST_CHECK(mem[1] == 0x9abcdef0);

    OK(uc_close(uc));
}

typedef struct {
    int reads;
    int writes;
    int outside;
    int dec_calls;
    uc_hook write_hook;
} page_hook_state;

static const char page_hook_loop[] =
    "\xa1\x00\x20\x00\x00"
    "\xa3\x04\x20\x00\x00"
    "\xa3\x00\x30\x00\x00"
    "\x49"
    "\x75\xee"
    "\x90\x90";

static void page_hook_read_cb(uc_engine *uc, uc_mem_type type,
                              uint64_t address, int size, int64_t value,
                              void *user_data)
{
    page_hook_state *state = (page_hook_state *)user_data;
    state->reads++;
    if (address < 0x2000 || address > 0x2fff) {
        state->outside++;
    }
}

static void page_hook_write_cb(uc_engine *uc, uc_mem_type type,
                               uint64_t address, int size, int64_t value,
                               void *user_data)
{
    page_hook_state *state = (page_hook_state *)user_data;
    state->writes++;
    if (address < 0x2000 || address > 0x2fff) {
        state->outside++;
    }
}

static uc_engine *open_page_hook_engine(void)
{
    uc_engine *uc;
    uint32_t value = 0x11223344;

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_mem_map(uc, 0x3000, 0x1000, UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_mem_write(uc, 0x1000, page_hook_loop, sizeof(page_hook_loop) - 1));
    OK(uc_mem_write(uc, 0x2000, &value, sizeof(value)));
    return uc;
}

static void run_page_hook_loop(uc_engine *uc, uint32_t iterations)
{
    OK(uc_reg_write(uc, UC_X86_REG_ECX, &iterations));
    OK(uc_emu_start(uc, 0x1000, 0x1013, 0, 0));
}

static void test_mem_hook_bounded_to_page(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook read_hook, write_hook;
    page_hook_state state = {0};
    uint32_t outside = 0;

    OK(uc_hook_add(uc, &read_hook, UC_HOOK_MEM_READ, page_hook_read_cb,
                   &state, 0x2000, 0x2fff));
    OK(uc_hook_add(uc, &write_hook, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 0x2000, 0x2fff));
    run_page_hook_loop(uc, 5);

    OK(uc_mem_read(uc, 0x3000, &outside, sizeof(outside)));
    TEST_CHECK(state.reads == 5);
    TEST_CHECK(state.writes == 5);
    TEST_CHECK(state.outside == 0);
    TEST_CHECK(outside == 0x11223344);

    OK(uc_close(uc));
}

typedef struct {
    int calls;
    int wrong_value;
    uint64_t addresses[4];
} read_after_state;

static void page_hook_read_after_cb(uc_engine *uc, uc_mem_type type,
                                    uint64_t address, int size, int64_t value,
                                    void *user_data)
{
    read_after_state *state = (read_after_state *)user_data;
    if (value != 0x11223344 || address != 0x2000) {
        state->wrong_value++;
    }
    state->calls++;
}

static void test_mem_hook_read_after_bounded(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook hook;
    read_after_state state = {0};

    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_READ_AFTER, page_hook_read_after_cb,
                   &state, 0x2000, 0x2fff));
    run_page_hook_loop(uc, 5);
    TEST_CHECK(state.calls == 5);
    TEST_CHECK(state.wrong_value == 0);

    OK(uc_close(uc));
}

static void page_hook_span_cb(uc_engine *uc, uc_mem_type type,
                              uint64_t address, int size, int64_t value,
                              void *user_data)
{
    read_after_state *state = (read_after_state *)user_data;
    if (state->calls < 4) {
        state->addresses[state->calls] = address;
    }
    state->calls++;
}

static void test_mem_hook_range_spanning_pages(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook hook;
    read_after_state state = {0};
    const char code[] = "\xb8\x44\x33\x22\x11"
                        "\xa3\x00\x20\x00\x00"
                        "\xa3\x08\x30\x00\x00"
                        "\xa3\xfc\x2f\x00\x00"
                        "\xa3\x00\x30\x00\x00"
                        "\x90\x90";

    OK(uc_mem_write(uc, 0x1100, code, sizeof(code) - 1));
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, page_hook_span_cb, &state,
                   0x2ffc, 0x3003));
    OK(uc_emu_start(uc, 0x1100, 0x1100 + sizeof(code) - 2, 0, 0));
    TEST_CHECK(state.calls == 2);
    TEST_CHECK(state.addresses[0] == 0x2ffc);
    TEST_CHECK(state.addresses[1] == 0x3000);

    OK(uc_close(uc));
}

static void test_mem_hook_added_after_translation(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook read_hook, write_hook;
    page_hook_state state = {0};

    run_page_hook_loop(uc, 5);
    OK(uc_hook_add(uc, &read_hook, UC_HOOK_MEM_READ, page_hook_read_cb,
                   &state, 0x2000, 0x2fff));
    OK(uc_hook_add(uc, &write_hook, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 0x2000, 0x2fff));
    run_page_hook_loop(uc, 5);
    TEST_CHECK(state.reads == 5);
    TEST_CHECK(state.writes == 5);

    OK(uc_hook_del(uc, read_hook));
    OK(uc_hook_del(uc, write_hook));
    run_page_hook_loop(uc, 5);
    TEST_CHECK(state.reads == 5);
    TEST_CHECK(state.writes == 5);

    OK(uc_close(uc));
}

static void page_hook_dec_cb(uc_engine *uc, uint64_t address, uint32_t size,
                             void *user_data)
{
    page_hook_state *state = (page_hook_state *)user_data;
    state->dec_calls++;
    if (state->dec_calls == 2) {
        OK(uc_hook_add(uc, &state->write_hook, UC_HOOK_MEM_WRITE,
                       page_hook_write_cb, state, 0x2000, 0x2fff));
    } else if (state->dec_calls == 4) {
        OK(uc_hook_del(uc, state->write_hook));
    }
}

static void test_mem_hook_added_mid_run(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook code_hook;
    page_hook_state state = {0};

    OK(uc_hook_add(uc, &code_hook, UC_HOOK_CODE, page_hook_dec_cb, &state,
                   0x100f, 0x100f));
    run_page_hook_loop(uc, 6);
    TEST_CHECK(state.dec_calls == 6);
    TEST_CHECK(state.writes == 2);

    OK(uc_close(uc));
}

static void page_hook_self_delete_cb(uc_engine *uc, uc_mem_type type,
                                     uint64_t address, int size,
                                     int64_t value, void *user_data)
{
    page_hook_state *state = (page_hook_state *)user_data;
    state->writes++;
    OK(uc_hook_del(uc, state->write_hook));
}

static void test_mem_hook_deleted_in_its_callback(void)
{
    uc_engine *uc = open_page_hook_engine();
    page_hook_state state = {0};
    uint32_t stored = 0;

    OK(uc_hook_add(uc, &state.write_hook, UC_HOOK_MEM_WRITE,
                   page_hook_self_delete_cb, &state, 0x2000, 0x2fff));
    run_page_hook_loop(uc, 3);

    OK(uc_mem_read(uc, 0x2004, &stored, sizeof(stored)));
    TEST_CHECK(state.writes == 1);
    TEST_CHECK(stored == 0x11223344);

    OK(uc_close(uc));
}

static void test_mem_hook_arm_exclusive_store(void)
{
    uc_engine *uc;
    uc_hook read_hook, write_hook;
    page_hook_state state = {0};
    const char code[] = "\x50\xe8\x00\x2f"
                        "\x40\xe8\x00\x13"
                        "\x00\xbf\x00\xbf";
    uint32_t r0 = 0x2000, r1 = 0x55, r2 = 0, r3 = 1, stored = 0;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_map(uc, 0x2000, 0x1000, UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_mem_write(uc, 0x1000, code, sizeof(code) - 1));
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r3));
    OK(uc_hook_add(uc, &read_hook, UC_HOOK_MEM_READ, page_hook_read_cb,
                   &state, 0x2000, 0x2fff));
    OK(uc_hook_add(uc, &write_hook, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 0x2000, 0x2fff));
    OK(uc_emu_start(uc, 0x1000 | 1, 0x100a, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r2));
    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r3));
    OK(uc_mem_read(uc, 0x2000, &stored, sizeof(stored)));
    TEST_CHECK(state.reads == 2);
    TEST_CHECK(state.writes == 1);
    TEST_CHECK(r3 == 0);
    TEST_CHECK(stored == 0x55);

    OK(uc_close(uc));
}

static uint8_t alias_backing[0x1000];

static uc_engine *open_alias_engine(uint32_t target_perms, uint32_t alias_perms)
{
    uc_engine *uc;

    memset(alias_backing, 0, sizeof(alias_backing));
    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    OK(uc_mem_map_ptr(uc, 0x20000000, 0x1000, target_perms, alias_backing));
    OK(uc_mem_map_alias(uc, 0x0a000000, 0x1000, alias_perms, 0x20000000));
    return uc;
}

static void test_mem_alias_shares_storage(void)
{
    uc_engine *uc = open_alias_engine(UC_PROT_ALL, UC_PROT_ALL);
    uint32_t value = 0x12345678, read = 0;

    OK(uc_mem_write(uc, 0x0a000010, &value, sizeof(value)));
    OK(uc_mem_read(uc, 0x20000010, &read, sizeof(read)));
    TEST_CHECK(read == 0x12345678);
    TEST_CHECK(memcmp(alias_backing + 0x10, &value, sizeof(value)) == 0);

    value = 0x9abcdef0;
    OK(uc_mem_write(uc, 0x20000ffc, &value, sizeof(value)));
    OK(uc_mem_read(uc, 0x0a000ffc, &read, sizeof(read)));
    TEST_CHECK(read == 0x9abcdef0);

    OK(uc_close(uc));
}

static void test_mem_alias_perms_per_window(void)
{
    uc_engine *uc =
        open_alias_engine(UC_PROT_READ | UC_PROT_WRITE, UC_PROT_READ | UC_PROT_EXEC);
    const char code[] = "\x08\x60\x00\xbf";
    uint32_t r0 = 0x55, r1, stored = 0;

    OK(uc_mem_write(uc, 0x20000000, code, sizeof(code) - 1));
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));

    r1 = 0x20000100;
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_emu_start(uc, 0x0a000000 | 1, 0x0a000002, 0, 0));
    OK(uc_mem_read(uc, 0x20000100, &stored, sizeof(stored)));
    TEST_CHECK(stored == 0x55);

    r1 = 0x0a000200;
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    uc_assert_err(UC_ERR_WRITE_PROT,
                  uc_emu_start(uc, 0x0a000000 | 1, 0x0a000002, 0, 0));
    stored = 0;
    OK(uc_mem_read(uc, 0x20000200, &stored, sizeof(stored)));
    TEST_CHECK(stored == 0);

    uc_assert_err(UC_ERR_FETCH_PROT,
                  uc_emu_start(uc, 0x20000000 | 1, 0x20000002, 0, 0));

    OK(uc_close(uc));
}

static void test_mem_alias_store_invalidates_other_window(void)
{
    uc_engine *uc =
        open_alias_engine(UC_PROT_READ | UC_PROT_WRITE, UC_PROT_READ | UC_PROT_EXEC);
    const char code[] = "\x01\x33\x00\xe0\x00\xbf\x01\x20\x02\x2b\x02\xd0"
                        "\xff\xe7\x0a\x80\xf6\xe7\xfe\xe7";
    uint32_t r0 = 0, r1 = 0x20000006, r2 = 0x2002, r3 = 0;

    OK(uc_mem_write(uc, 0x20000000, code, sizeof(code) - 1));
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r3));
    OK(uc_emu_start(uc, 0x0a000000 | 1, 0x0a000012, 0, 100));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r3));
    TEST_CHECK(r3 == 2);
    TEST_CHECK(r0 == 2);

    OK(uc_close(uc));
}

static void run_patched_twice_through_execute_never(uint64_t target,
                                                    uint32_t target_perms,
                                                    uint64_t alias,
                                                    uint32_t alias_perms)
{
    uc_engine *uc;
    const char code[] = "\x00\xf0\x0a\xf8\x05\x46\x0a\x80\x00\xf0\x06\xf8"
                        "\x06\x46\x0b\x80\x00\xf0\x02\xf8\x07\x46\xfe\xe7"
                        "\x01\x20\x70\x47";
    uint32_t r1 = 0xa0000018, r2 = 0x2002, r3 = 0x2003, r5 = 0, r6 = 0, r7 = 0;

    memset(alias_backing, 0, sizeof(alias_backing));
    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    OK(uc_mem_map_ptr(uc, target, 0x1000, target_perms, alias_backing));
    OK(uc_mem_map_alias(uc, alias, 0x1000, alias_perms, target));
    OK(uc_mem_write(uc, 0x20000000, code, sizeof(code) - 1));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r3));
    OK(uc_emu_start(uc, 0x20000000 | 1, 0x20000016, 0, 100));

    OK(uc_reg_read(uc, UC_ARM_REG_R5, &r5));
    OK(uc_reg_read(uc, UC_ARM_REG_R6, &r6));
    OK(uc_reg_read(uc, UC_ARM_REG_R7, &r7));
    TEST_CHECK(r5 == 1);
    TEST_CHECK(r6 == 2);
    TEST_CHECK(r7 == 3);
    TEST_MSG("r5 %u r6 %u r7 %u", r5, r6, r7);

    OK(uc_close(uc));
}

static void test_mem_alias_execute_never_window_keeps_invalidating(void)
{
    run_patched_twice_through_execute_never(0x20000000, UC_PROT_ALL, 0xa0000000,
                                            UC_PROT_READ | UC_PROT_WRITE);
    run_patched_twice_through_execute_never(
        0xa0000000, UC_PROT_READ | UC_PROT_WRITE, 0x20000000, UC_PROT_ALL);
}

static void test_mem_alias_unmap_order(void)
{
    uc_engine *uc = open_alias_engine(UC_PROT_ALL, UC_PROT_ALL);

    uc_assert_err(UC_ERR_ARG, uc_mem_unmap(uc, 0x20000000, 0x1000));
    uc_assert_err(UC_ERR_ARG, uc_mem_unmap(uc, 0x0a000000, 0x400));
    OK(uc_mem_unmap(uc, 0x0a000000, 0x1000));
    OK(uc_mem_unmap(uc, 0x20000000, 0x1000));

    OK(uc_close(uc));
}

static void test_mem_alias_protect_and_snapshot_refused(void)
{
    uc_engine *uc = open_alias_engine(UC_PROT_ALL, UC_PROT_ALL);
    uc_context *context;

    uc_assert_err(UC_ERR_ARG, uc_mem_protect(uc, 0x0a000000, 0x1000,
                                             UC_PROT_READ));
    uc_assert_err(UC_ERR_ARG, uc_mem_protect(uc, 0x20000000, 0x1000,
                                             UC_PROT_READ));

    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY));
    OK(uc_context_alloc(uc, &context));
    uc_assert_err(UC_ERR_ARG, uc_context_save(uc, context));
    OK(uc_context_free(context));

    OK(uc_close(uc));
}

static void test_mem_alias_listed_by_regions(void)
{
    uc_engine *uc = open_alias_engine(UC_PROT_READ | UC_PROT_WRITE,
                                      UC_PROT_READ | UC_PROT_EXEC);
    uc_mem_region *regions;
    uint32_t count = 0;

    OK(uc_mem_regions(uc, &regions, &count));
    TEST_CHECK(count == 2);
    TEST_CHECK(regions[0].begin == 0x0a000000);
    TEST_CHECK(regions[0].end == 0x0a000fff);
    TEST_CHECK(regions[0].perms == (UC_PROT_READ | UC_PROT_EXEC));
    TEST_CHECK(regions[1].begin == 0x20000000);
    TEST_CHECK(regions[1].perms == (UC_PROT_READ | UC_PROT_WRITE));
    OK(uc_free(regions));

    OK(uc_close(uc));
}

static void test_mem_alias_refuses_partial(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    OK(uc_mem_map_ptr(uc, 0x20000000, 0x2000, UC_PROT_ALL, alias_backing));
    uc_assert_err(UC_ERR_ARG, uc_mem_map_alias(uc, 0x0a000000, 0x1000,
                                               UC_PROT_ALL, 0x20000000));
    uc_assert_err(UC_ERR_ARG, uc_mem_map_alias(uc, 0x0a000000, 0x1000,
                                               UC_PROT_ALL, 0x20001000));
    uc_assert_err(UC_ERR_ARG, uc_mem_map_alias(uc, 0x0a000000, 0x2000,
                                               UC_PROT_ALL, 0x30000000));
    OK(uc_mem_map_alias(uc, 0x0a000000, 0x2000, UC_PROT_ALL, 0x20000000));
    uc_assert_err(UC_ERR_ARG, uc_mem_map_alias(uc, 0x0e000000, 0x2000,
                                               UC_PROT_ALL, 0x0a000000));

    OK(uc_close(uc));
}

static uint64_t slow_path_accesses_of_loop(uc_engine *uc, uint32_t iterations)
{
    uint64_t count = 0;

    run_page_hook_loop(uc, iterations);
    OK(uc_ctl_slow_path_count_reset(uc));
    run_page_hook_loop(uc, iterations);
    OK(uc_ctl_slow_path_count(uc, &count));
    return count;
}

static void test_mem_slow_path_only_on_hooked_pages(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook read_hook, write_hook, unbounded;
    page_hook_state state = {0};

    TEST_CHECK(slow_path_accesses_of_loop(uc, 5) == 0);

    OK(uc_hook_add(uc, &read_hook, UC_HOOK_MEM_READ, page_hook_read_cb,
                   &state, 0x2000, 0x2fff));
    OK(uc_hook_add(uc, &write_hook, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 0x2000, 0x2fff));
    TEST_CHECK(slow_path_accesses_of_loop(uc, 5) == 10);

    OK(uc_hook_del(uc, read_hook));
    OK(uc_hook_del(uc, write_hook));
    TEST_CHECK(slow_path_accesses_of_loop(uc, 5) == 0);

    OK(uc_hook_add(uc, &unbounded, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 1, 0));
    TEST_CHECK(slow_path_accesses_of_loop(uc, 5) == 10);

    OK(uc_close(uc));
}

static void test_mem_slow_path_deleted_mid_run(void)
{
    uc_engine *uc = open_page_hook_engine();
    uc_hook code_hook;
    page_hook_state state = {0};
    uint64_t count = 0;

    OK(uc_hook_add(uc, &code_hook, UC_HOOK_CODE, page_hook_dec_cb, &state,
                   0x100f, 0x100f));
    run_page_hook_loop(uc, 6);
    TEST_CHECK(state.writes == 2);
    OK(uc_hook_del(uc, code_hook));
    OK(uc_ctl_slow_path_count_reset(uc));
    run_page_hook_loop(uc, 6);
    OK(uc_ctl_slow_path_count(uc, &count));
    TEST_CHECK(state.writes == 2);
    TEST_CHECK(count == 0);

    OK(uc_close(uc));
}

static const char store_routines[] = "\x08\x60\x01\x3a\xfc\xd1\x00\xbf"
                                     "\x08\x60\x00\xbf"
                                     "\x08\x80\x00\xbf"
                                     "\x88\x47\x00\xbf";

static const char return_one[] = "\x01\x20\x70\x47";

static void map_store_routines(uc_engine *uc)
{
    OK(uc_mem_map(uc, 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x1000, store_routines, sizeof(store_routines) - 1));
}

static uc_engine *open_store_engine(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    map_store_routines(uc);
    OK(uc_mem_map(uc, 0x20000000, 0x1000, UC_PROT_ALL));
    return uc;
}

static void guest_store(uc_engine *uc, uint64_t address, uint32_t value)
{
    uint32_t r0 = value, r1 = (uint32_t)address;

    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_emu_start(uc, 0x1008 | 1, 0x100a, 0, 0));
}

static void guest_store_half(uc_engine *uc, uint64_t address, uint32_t value)
{
    uint32_t r0 = value, r1 = (uint32_t)address;

    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_emu_start(uc, 0x100c | 1, 0x100e, 0, 0));
}

static uint64_t slow_path_stores_of_loop(uc_engine *uc, uint64_t address,
                                         uint32_t iterations)
{
    uint32_t r0 = 0x55, r1 = (uint32_t)address, r2 = iterations;
    uint64_t count = 0;

    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));
    OK(uc_ctl_slow_path_count_reset(uc));
    OK(uc_emu_start(uc, 0x1000 | 1, 0x1006, 0, 0));
    OK(uc_ctl_slow_path_count(uc, &count));
    return count;
}

static uint32_t call_function(uc_engine *uc, uint64_t address)
{
    uint32_t r0 = 0, r1 = (uint32_t)address | 1;

    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_emu_start(uc, 0x1010 | 1, 0x1012, 0, 0));
    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r0));
    return r0;
}

static bool refuse_bad_access(uc_engine *uc, uc_mem_type type,
                              uint64_t address, int size, int64_t value,
                              void *user_data)
{
    return false;
}

static void test_mem_store_fast_path_on_executable_ram(void)
{
    uc_engine *uc = open_store_engine();
    uc_hook bad_access;

    OK(uc_hook_add(uc, &bad_access, UC_HOOK_MEM_UNMAPPED | UC_HOOK_MEM_PROT,
                   refuse_bad_access, NULL, 1, 0));
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 1);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 0);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000204, 5) == 0);

    OK(uc_close(uc));
}

static void test_mem_store_patches_code_of_its_own_window(void)
{
    uc_engine *uc = open_store_engine();

    OK(uc_mem_write(uc, 0x20000100, return_one, sizeof(return_one) - 1));
    TEST_CHECK(call_function(uc, 0x20000100) == 1);
    guest_store(uc, 0x20000200, 0x55);
    guest_store_half(uc, 0x20000100, 0x2002);
    TEST_CHECK(call_function(uc, 0x20000100) == 2);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 5);

    OK(uc_close(uc));
}

static void test_mem_alias_cleaned_window_rearmed_by_new_code(void)
{
    uc_engine *uc = open_alias_engine(UC_PROT_ALL, UC_PROT_ALL);

    map_store_routines(uc);
    OK(uc_mem_write(uc, 0x20000100, return_one, sizeof(return_one) - 1));
    guest_store(uc, 0x0a000200, 0x55);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x0a000200, 5) == 0);
    TEST_CHECK(call_function(uc, 0x20000100) == 1);
    guest_store_half(uc, 0x0a000100, 0x2002);
    TEST_CHECK(call_function(uc, 0x20000100) == 2);

    OK(uc_close(uc));
}

static void test_mem_write_hook_added_after_clean(void)
{
    uc_engine *uc = open_store_engine();
    uc_hook write_hook;
    page_hook_state state = {0};

    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 1);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 0);
    OK(uc_hook_add(uc, &write_hook, UC_HOOK_MEM_WRITE, page_hook_write_cb,
                   &state, 0x20000000, 0x20000fff));
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 5);
    TEST_CHECK(state.writes == 5);
    OK(uc_hook_del(uc, write_hook));
    slow_path_stores_of_loop(uc, 0x20000200, 5);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000200, 5) == 0);
    TEST_CHECK(state.writes == 5);

    OK(uc_close(uc));
}

static void test_mem_snapshot_after_clean_copies_on_write(void)
{
    uc_engine *uc = open_store_engine();
    uc_context *before;
    uint32_t value = 0;

    OK(uc_ctl_context_mode(uc, UC_CTL_CONTEXT_MEMORY));
    OK(uc_context_alloc(uc, &before));
    guest_store(uc, 0x20000200, 1);
    TEST_CHECK(slow_path_stores_of_loop(uc, 0x20000204, 5) == 0);
    OK(uc_context_save(uc, before));
    guest_store(uc, 0x20000200, 2);
    OK(uc_mem_read(uc, 0x20000200, &value, sizeof(value)));
    TEST_CHECK(value == 2);
    OK(uc_context_restore(uc, before));
    OK(uc_mem_read(uc, 0x20000200, &value, sizeof(value)));
    TEST_CHECK(value == 1);
    TEST_MSG("value %u", value);

    OK(uc_context_free(before));
    OK(uc_close(uc));
}

TEST_LIST = {{"test_map_correct", test_map_correct},
             {"test_map_wrapping", test_map_wrapping},
             {"test_mem_protect", test_mem_protect},
             {"test_splitting_mem_unmap", test_splitting_mem_unmap},
             {"test_splitting_mmio_unmap", test_splitting_mmio_unmap},
             {"test_mem_protect_map_ptr", test_mem_protect_map_ptr},
             {"test_map_at_the_end", test_map_at_the_end},
             {"test_map_wrap", test_map_wrap},
             {"test_map_big_memory", test_map_big_memory},
             {"test_mem_protect_remove_exec", test_mem_protect_remove_exec},
             {"test_mem_protect_mmio", test_mem_protect_mmio},
             {"test_snapshot", test_snapshot},
             {"test_snapshot_with_vtlb", test_snapshot_with_vtlb},
             {"test_context_snapshot", test_context_snapshot},
             {"test_snapshot_unmap", test_snapshot_unmap},
             {"test_mem_read_and_write_large_memory_block",
              test_mem_read_and_write_large_memory_block},
             {"test_virtual_to_physical", test_virtual_to_physical},
             {"test_virtual_write", test_virtual_write},
             {"test_mem_addr_size_wraparound", test_mem_addr_size_wraparound},
             {"test_smc", test_smc},
             {"test_tlbdirty_exec", test_tlbdirty_exec},
             {"test_mem_write_prot_drop_store", test_mem_write_prot_drop_store},
             {"test_mem_write_prot_handled_commits",
              test_mem_write_prot_handled_commits},
             {"test_mem_write_prot_drop_store_scope",
              test_mem_write_prot_drop_store_scope},
             {"test_mem_write_hook_flush_tlb", test_mem_write_hook_flush_tlb},
             {"test_mem_hook_bounded_to_page", test_mem_hook_bounded_to_page},
             {"test_mem_hook_read_after_bounded",
              test_mem_hook_read_after_bounded},
             {"test_mem_hook_range_spanning_pages",
              test_mem_hook_range_spanning_pages},
             {"test_mem_hook_added_after_translation",
              test_mem_hook_added_after_translation},
             {"test_mem_hook_added_mid_run", test_mem_hook_added_mid_run},
             {"test_mem_hook_deleted_in_its_callback",
              test_mem_hook_deleted_in_its_callback},
             {"test_mem_hook_arm_exclusive_store",
              test_mem_hook_arm_exclusive_store},
             {"test_mem_alias_shares_storage", test_mem_alias_shares_storage},
             {"test_mem_alias_perms_per_window",
              test_mem_alias_perms_per_window},
             {"test_mem_alias_store_invalidates_other_window",
              test_mem_alias_store_invalidates_other_window},
             {"test_mem_alias_execute_never_window_keeps_invalidating",
              test_mem_alias_execute_never_window_keeps_invalidating},
             {"test_mem_alias_unmap_order", test_mem_alias_unmap_order},
             {"test_mem_alias_protect_and_snapshot_refused",
              test_mem_alias_protect_and_snapshot_refused},
             {"test_mem_alias_listed_by_regions",
              test_mem_alias_listed_by_regions},
             {"test_mem_alias_refuses_partial", test_mem_alias_refuses_partial},
             {"test_mem_slow_path_only_on_hooked_pages",
              test_mem_slow_path_only_on_hooked_pages},
             {"test_mem_slow_path_deleted_mid_run",
              test_mem_slow_path_deleted_mid_run},
             {"test_mem_store_fast_path_on_executable_ram",
              test_mem_store_fast_path_on_executable_ram},
             {"test_mem_store_patches_code_of_its_own_window",
              test_mem_store_patches_code_of_its_own_window},
             {"test_mem_alias_cleaned_window_rearmed_by_new_code",
              test_mem_alias_cleaned_window_rearmed_by_new_code},
             {"test_mem_write_hook_added_after_clean",
              test_mem_write_hook_added_after_clean},
             {"test_mem_snapshot_after_clean_copies_on_write",
              test_mem_snapshot_after_clean_copies_on_write},
             {NULL, NULL}};
