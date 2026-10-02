#include "unicorn_test.h"

const uint64_t code_start = 0x1000;
const uint64_t code_len = 0x4000;

static void uc_common_setup(uc_engine **uc, uc_arch arch, uc_mode mode,
                            const char *code, uint64_t size, uc_cpu_arm cpu)
{
    OK(uc_open(arch, mode, uc));
    OK(uc_ctl_set_cpu_model(*uc, cpu));
    OK(uc_mem_map(*uc, code_start, code_len, UC_PROT_ALL));
    OK(uc_mem_write(*uc, code_start, code, size));
}

typedef struct _WFI_HOOK_INSN_RESULT {
    bool called;
} WFI_HOOK_INSN_RESULT;

static void test_arm_nop(void)
{
    uc_engine *uc;
    char code[] = "\x00\xf0\x20\xe3"; // nop
    int r_r0 = 0x1234;
    int r_r2 = 0x6789;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r_r2));
    TEST_CHECK(r_r0 == 0x1234);
    TEST_CHECK(r_r2 == 0x6789);

    OK(uc_close(uc));
}

static void test_arm_thumb_sub(void)
{
    uc_engine *uc;
    char code[] = "\x83\xb0"; // sub    sp, #0xc
    int r_sp = 0x1234;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));
    TEST_CHECK(r_sp == 0x1228);

    OK(uc_close(uc));
}

static void test_armeb_sub(void)
{
    uc_engine *uc;
    char code[] =
        "\xe3\xa0\x00\x37\xe0\x42\x10\x03"; // mov r0, #0x37; sub r1, r2, r3
    int r_r0 = 0x1234;
    int r_r2 = 0x6789;
    int r_r3 = 0x3333;
    int r_r1;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_BIG_ENDIAN, code,
                    sizeof(code) - 1, UC_CPU_ARM_1176);
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r_r3));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r_r1));
    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r_r3));

    TEST_CHECK(r_r0 == 0x37);
    TEST_CHECK(r_r2 == 0x6789);
    TEST_CHECK(r_r3 == 0x3333);
    TEST_CHECK(r_r1 == 0x3456);

    OK(uc_close(uc));
}

static void test_armeb_be8_sub(void)
{
    uc_engine *uc;
    char code[] =
        "\x37\x00\xa0\xe3\x03\x10\x42\xe0"; // mov r0, #0x37; sub r1, r2, r3
    int r_r0 = 0x1234;
    int r_r2 = 0x6789;
    int r_r3 = 0x3333;
    int r_r1;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_ARMBE8, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_A15);
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r_r3));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r_r1));
    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r_r3));

    TEST_CHECK(r_r0 == 0x37);
    TEST_CHECK(r_r2 == 0x6789);
    TEST_CHECK(r_r3 == 0x3333);
    TEST_CHECK(r_r1 == 0x3456);

    OK(uc_close(uc));
}

static void test_arm_thumbeb_sub(void)
{
    uc_engine *uc;
    char code[] = "\xb0\x83"; // sub    sp, #0xc
    int r_sp = 0x1234;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_BIG_ENDIAN, code,
                    sizeof(code) - 1, UC_CPU_ARM_1176);
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));
    TEST_CHECK(r_sp == 0x1228);

    OK(uc_close(uc));
}

static void test_arm_thumb_ite_count_callback(uc_engine *uc, uint64_t address,
                                              uint32_t size, void *user_data)
{
    uint64_t *count = (uint64_t *)user_data;

    (*count) += 1;
}

static void test_arm_thumb_ite(void)
{
    uc_engine *uc;
    uc_hook hook;
    char code[] =
        "\x9a\x42\x15\xbf\x00\x9a\x01\x9a\x78\x23\x15\x23"; // cmp r2, r3; itete
                                                            // ne; ldrne r2,
                                                            // [sp]; ldreq r2,
                                                            // [sp,#4]; movne
                                                            // r3, #0x78; moveq
                                                            // r3, #0x15
    int r_sp = 0x8000;
    int r_r2 = 0;
    int r_r3 = 1;
    int r_pc = 0;
    uint64_t count = 0;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r_r3));

    OK(uc_mem_map(uc, r_sp, 0x1000, UC_PROT_ALL));
    r_r2 = LEINT32(0x68);
    OK(uc_mem_write(uc, r_sp, &r_r2, 4));
    r_r2 = LEINT32(0x4d);
    OK(uc_mem_write(uc, r_sp + 4, &r_r2, 4));

    OK(uc_hook_add(uc, &hook, UC_HOOK_CODE, test_arm_thumb_ite_count_callback,
                   &count, 1, 0));

    // Execute four instructions at a time.
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r_r3));
    TEST_CHECK(r_r2 == 0x68);
    TEST_CHECK(count == 4);

    r_pc = code_start;
    r_r2 = 0;
    count = 0;
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r_r3));
    for (int i = 0; i < 6 && r_pc < code_start + sizeof(code) - 1; i++) {
        // Execute one instruction at a time.
        OK(uc_emu_start(uc, r_pc | 1, code_start + sizeof(code) - 1, 0, 1));

        OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));
    }
    OK(uc_reg_read(uc, UC_ARM_REG_R2, &r_r2));

    TEST_CHECK(r_r2 == 0x68);
    TEST_CHECK(r_r3 == 0x78);
    TEST_CHECK(count == 4);

    OK(uc_close(uc));
}

static void test_arm_m_thumb_mrs(void)
{
    uc_engine *uc;
    char code[] =
        "\xef\xf3\x14\x80\xef\xf3\x00\x81"; // mrs r0, control; mrs r1, apsr
    uint32_t r_control = 0b10;
    uint32_t r_apsr = (0b10101 << 27);
    uint32_t r_r0, r_r1;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_A15);

    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &r_control));
    OK(uc_reg_write(uc, UC_ARM_REG_APSR_NZCVQ, &r_apsr));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r_r1));

    TEST_CHECK(r_r0 == 0b10);
    TEST_CHECK(r_r1 == (0b10101 << 27));

    OK(uc_close(uc));
}

static void test_arm_m_control(void)
{
    uc_engine *uc;
    int r_control, r_msp, r_psp;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));

    r_control = 0; // Make sure we are using MSP.
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &r_control));

    r_msp = 0x1000;
    OK(uc_reg_write(uc, UC_ARM_REG_R13, &r_msp));

    r_control = 0b10; // Make the switch.
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &r_control));

    OK(uc_reg_read(uc, UC_ARM_REG_R13, &r_psp));
    TEST_CHECK(r_psp != r_msp);

    r_psp = 0x2000;
    OK(uc_reg_write(uc, UC_ARM_REG_R13, &r_psp));

    r_control = 0; // Switch again
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &r_control));

    OK(uc_reg_read(uc, UC_ARM_REG_R13, &r_msp));
    TEST_CHECK(r_psp != r_msp);
    TEST_CHECK(r_msp == 0x1000);

    OK(uc_close(uc));
}

static void test_arm_m_unprivileged_special_regs(void)
{
    uc_engine *uc;
    uint32_t value;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));

    value = 0x1000;
    OK(uc_reg_write(uc, UC_ARM_REG_MSP, &value));
    value = 0x2000;
    OK(uc_reg_write(uc, UC_ARM_REG_PSP, &value));
    value = 0b11;
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_CONTROL, &value));
    TEST_CHECK(value == 0b11);

    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0x1000);
    OK(uc_reg_read(uc, UC_ARM_REG_PSP, &value));
    TEST_CHECK(value == 0x2000);
    OK(uc_reg_read(uc, UC_ARM_REG_R13, &value));
    TEST_CHECK(value == 0x2000);

    value = 0x3000;
    OK(uc_reg_write(uc, UC_ARM_REG_MSP, &value));
    value = 0x4000;
    OK(uc_reg_write(uc, UC_ARM_REG_PSP, &value));
    value = 1;
    OK(uc_reg_write(uc, UC_ARM_REG_PRIMASK, &value));
    value = 0x40;
    OK(uc_reg_write(uc, UC_ARM_REG_BASEPRI, &value));
    value = 1;
    OK(uc_reg_write(uc, UC_ARM_REG_FAULTMASK, &value));

    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0x3000);
    OK(uc_reg_read(uc, UC_ARM_REG_PSP, &value));
    TEST_CHECK(value == 0x4000);
    OK(uc_reg_read(uc, UC_ARM_REG_R13, &value));
    TEST_CHECK(value == 0x4000);
    OK(uc_reg_read(uc, UC_ARM_REG_PRIMASK, &value));
    TEST_CHECK(value == 1);
    OK(uc_reg_read(uc, UC_ARM_REG_BASEPRI, &value));
    TEST_CHECK(value == 0x40);
    OK(uc_reg_read(uc, UC_ARM_REG_FAULTMASK, &value));
    TEST_CHECK(value == 1);
    OK(uc_reg_read(uc, UC_ARM_REG_CONTROL, &value));
    TEST_CHECK(value == 0b11);

    value = 0b10;
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_CONTROL, &value));
    TEST_CHECK(value == 0b10);

    value = 0b11;
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_CONTROL, &value));
    TEST_CHECK(value == 0b11);

    value = 0b01;
    OK(uc_reg_write(uc, UC_ARM_REG_CONTROL, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_CONTROL, &value));
    TEST_CHECK(value == 0b01);
    OK(uc_reg_read(uc, UC_ARM_REG_R13, &value));
    TEST_CHECK(value == 0x3000);

    OK(uc_close(uc));
}

//
// Some notes:
//   Qemu raise a special exception EXCP_EXCEPTION_EXIT to handle the
//   EXC_RETURN. We can't help user handle EXC_RETURN since unicorn is designed
//   not to handle any CPU exception.
//
static void test_arm_m_exc_return_hook_interrupt(uc_engine *uc, int intno,
                                                 void *data)
{
    int r_pc;

    OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));
    TEST_CHECK(intno == 8); // EXCP_EXCEPTION_EXIT: Return from v7M exception.
    TEST_CHECK((r_pc | 1) == 0xFFFFFFFD);
    OK(uc_emu_stop(uc));
}

static void test_arm_m_exc_return(void)
{
    uc_engine *uc;
    char code[] = "\x6f\xf0\x02\x00\x00\x47"; // mov r0, #0xFFFFFFFD; bx r0;
    int r_ipsr;
    int r_sp = 0x8000;
    uc_hook hook;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_M7);
    OK(uc_mem_map(uc, r_sp - 0x1000, 0x1000, UC_PROT_ALL));
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR,
                   test_arm_m_exc_return_hook_interrupt, NULL, 0, 0));

    r_sp -= 0x1c;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));

    r_ipsr = 16; // We are in whatever exception.
    OK(uc_reg_write(uc, UC_ARM_REG_IPSR, &r_ipsr));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0,
                    2)); // Just execute 2 instructions.

    OK(uc_hook_del(uc, hook));
    OK(uc_close(uc));
}

// For details, see https://github.com/unicorn-engine/unicorn/issues/1494.
static void test_arm_und32_to_svc32(void)
{
    uc_engine *uc;
    // # MVN r0, #0
    // # MOVS pc, lr
    // # MVN r0, #0
    // # MVN r0, #0
    char code[] =
        "\x00\x00\xe0\xe3\x0e\xf0\xb0\xe1\x00\x00\xe0\xe3\x00\x00\xe0\xe3";
    int r_cpsr, r_sp, r_spsr, r_lr;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_A9));

    OK(uc_mem_map(uc, code_start, code_len, UC_PROT_ALL));
    OK(uc_mem_write(uc, code_start, code, sizeof(code) - 1));

    // https://www.keil.com/pack/doc/CMSIS/Core_A/html/group__CMSIS__CPSR__M.html
    r_cpsr = 0x40000093; // SVC32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_sp = 0x12345678;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));

    r_cpsr = 0x4000009b; // UND32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_spsr = 0x40000093; // Save previous CPSR
    OK(uc_reg_write(uc, UC_ARM_REG_SPSR, &r_spsr));
    r_sp = 0xDEAD0000;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    r_lr = code_start + 8;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 3));

    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));

    TEST_CHECK(r_sp == 0x12345678);

    OK(uc_close(uc));
}

static void test_arm_usr32_to_svc32(void)
{
    uc_engine *uc;
    int r_cpsr, r_sp, r_spsr, r_lr;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_A9));

    // https://www.keil.com/pack/doc/CMSIS/Core_A/html/group__CMSIS__CPSR__M.html
    r_cpsr = 0x40000093; // SVC32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_sp = 0x12345678;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    r_lr = 0x00102220;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    r_cpsr = 0x4000009b; // UND32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_spsr = 0x40000093; // Save previous CPSR
    OK(uc_reg_write(uc, UC_ARM_REG_SPSR, &r_spsr));
    r_sp = 0xDEAD0000;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    r_lr = 0x00509998;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));
    TEST_CHECK((r_cpsr & ((1 << 4) - 1)) == 0xb); // We are in UND32

    r_cpsr = 0x40000090; // USR32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_sp = 0x0010000;
    OK(uc_reg_write(uc, UC_ARM_REG_R13, &r_sp));
    r_lr = 0x0001234;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));
    TEST_CHECK((r_cpsr & ((1 << 4) - 1)) == 0); // We are in USR32

    r_cpsr = 0x40000093; // SVC32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));

    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));
    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));
    TEST_CHECK((r_cpsr & ((1 << 4) - 1)) == 3); // We are in SVC32
    TEST_CHECK(r_sp == 0x12345678);

    OK(uc_close(uc));
}

static void test_arm_v8(void)
{
    char code[] = "\xd0\xe8\xff\x17"; // LDAEXD.W R1, [R0]
    uc_engine *uc;
    uint32_t r_r1 = LEINT32(0xdeadbeef);
    uint32_t r_r0;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M33);

    r_r0 = 0x8000;
    OK(uc_mem_map(uc, r_r0, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, r_r0, (void *)&r_r1, 4));
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r_r0));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r_r1));

    TEST_CHECK(r_r1 == 0xdeadbeef);

    OK(uc_close(uc));
}

static void test_arm_thumb_smlabb(void)
{
    char code[] = "\x13\xfb\x01\x23";
    uint32_t r_r1, r_r2, r_r3;
    uc_engine *uc;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);

    r_r3 = 5;
    r_r1 = 7;
    r_r2 = 9;
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r_r3));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r_r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r_r2));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r_r3));

    TEST_CHECK(r_r3 == 5 * 7 + 9);

    OK(uc_close(uc));
}

static void test_arm_not_allow_privilege_escalation(void)
{
    uc_engine *uc;
    int r_cpsr, r_sp, r_spsr, r_lr;
    // E3C6601F : BIC     r6, r6, #&1F
    // E3866013 : ORR     r6, r6, #&13
    // E121F006 : MSR     cpsr_c, r6 ; switch to SVC32 (should be ineffective
    // from USR32)
    // E1A00000 : MOV     r0,r0 EF000011 : SWI     OS_Exit
    char code[] = "\x1f\x60\xc6\xe3\x13\x60\x86\xe3\x06\xf0\x21\xe1\x00\x00\xa0"
                  "\xe1\x11\x00\x00\xef";

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);

    // https://www.keil.com/pack/doc/CMSIS/Core_A/html/group__CMSIS__CPSR.html
    r_cpsr = 0x40000013; // SVC32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_spsr = 0x40000013;
    OK(uc_reg_write(uc, UC_ARM_REG_SPSR, &r_spsr));
    r_sp = 0x12345678;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    r_lr = 0x00102220;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    r_cpsr = 0x40000010; // USR32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_sp = 0x0010000;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    r_lr = 0x0001234;
    OK(uc_reg_write(uc, UC_ARM_REG_LR, &r_lr));

    uc_assert_err(
        UC_ERR_EXCEPTION,
        uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));
    OK(uc_reg_read(uc, UC_ARM_REG_LR, &r_lr));
    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));

    TEST_CHECK((r_cpsr & ((1 << 4) - 1)) == 0); // Stay in USR32
    TEST_CHECK(r_lr == 0x1234);
    TEST_CHECK(r_sp == 0x10000);

    OK(uc_close(uc));
}

static void test_arm_mrc(void)
{
    uc_engine *uc;
    // mrc p15, #0, r1, c13, c0, #3
    char code[] = "\x1d\xee\x70\x1f";

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_MAX);

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_close(uc));
}

static void test_arm_hflags_rebuilt(void)
{
    // MRS     r6, apsr
    // BIC     r6, r6, #&1F
    // ORR     r6, r6, #&10
    // MSR     cpsr_c, r6
    // SWI     OS_EnterOS
    // MSR     cpsr_c, r6
    char code[] = "\x00\x60\x0f\xe1\x1f\x60\xc6\xe3\x10\x60\x86\xe3\x06\xf0\x21"
                  "\xe1\x16\x00\x02\xef\x06\xf0\x21\xe1";
    uc_engine *uc;
    uint32_t r_cpsr, r_spsr, r_r13, r_r14, r_pc;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A9);

    r_cpsr = 0x40000013; // SVC32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_spsr = 0x40000013;
    OK(uc_reg_write(uc, UC_ARM_REG_SPSR, &r_spsr));
    r_r13 = 0x12345678; // SP
    OK(uc_reg_write(uc, UC_ARM_REG_R13, &r_r13));
    r_r14 = 0x00102220; // LR
    OK(uc_reg_write(uc, UC_ARM_REG_R14, &r_r14));

    r_cpsr = 0x40000010; // USR32
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_r13 = 0x0010000; // SP
    OK(uc_reg_write(uc, UC_ARM_REG_R13, &r_r13));
    r_r14 = 0x0001234; // LR
    OK(uc_reg_write(uc, UC_ARM_REG_R14, &r_r14));

    uc_assert_err(
        UC_ERR_EXCEPTION,
        uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    r_cpsr = 0x60000013;
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_cpsr = 0x60000010;
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_cpsr = 0x60000013;
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));

    OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));

    OK(uc_emu_start(uc, r_pc, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));
    OK(uc_reg_read(uc, UC_ARM_REG_R13, &r_r13));
    OK(uc_reg_read(uc, UC_ARM_REG_R14, &r_r14));

    TEST_CHECK(r_cpsr == 0x60000010);
    TEST_CHECK(r_r13 == 0x00010000);
    TEST_CHECK(r_r14 == 0x00001234);

    OK(uc_close(uc));
}

static bool test_arm_mem_access_abort_hook_mem(uc_engine *uc, uc_mem_type type,
                                               uint64_t addr, int size,
                                               int64_t val, void *data)
{
    OK(uc_reg_read(uc, UC_ARM_REG_PC, data));
    return false;
}

static bool test_arm_mem_access_abort_hook_insn_invalid(uc_engine *uc,
                                                        void *data)
{
    OK(uc_reg_read(uc, UC_ARM_REG_PC, data));
    return false;
}

static void test_arm_mem_access_abort(void)
{
    // LDR     r0, [r0]
    // Undefined instruction
    char code[] = "\x00\x00\x90\xe5\x00\xa0\xf0\xf7";
    uc_engine *uc;
    uint32_t r_pc, r_r0, r_pc_in_hook;
    uc_hook hk, hkk;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A9);

    r_r0 = 0x990000;
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r_r0));

    OK(uc_hook_add(uc, &hk, UC_HOOK_MEM_UNMAPPED,
                   test_arm_mem_access_abort_hook_mem, (void *)&r_pc_in_hook, 1,
                   0));
    OK(uc_hook_add(uc, &hkk, UC_HOOK_INSN_INVALID,
                   test_arm_mem_access_abort_hook_insn_invalid,
                   (void *)&r_pc_in_hook, 1, 0));

    uc_assert_err(UC_ERR_READ_UNMAPPED,
                  uc_emu_start(uc, code_start, code_start + 4, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));

    TEST_CHECK(r_pc == r_pc_in_hook);

    uc_assert_err(UC_ERR_INSN_INVALID,
                  uc_emu_start(uc, code_start + 4, code_start + 8, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));

    TEST_CHECK(r_pc == r_pc_in_hook);

    uc_assert_err(UC_ERR_FETCH_UNMAPPED,
                  uc_emu_start(uc, 0x900000, 0x900000 + 8, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_PC, &r_pc));

    TEST_CHECK(r_pc == r_pc_in_hook);

    OK(uc_close(uc));
}

static void test_arm_read_sctlr(void)
{
    uc_engine *uc;
    uc_arm_cp_reg reg;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc));

    // SCTLR. See arm reference.
    reg.cp = 15;
    reg.is64 = 0;
    reg.sec = 0;
    reg.crn = 1;
    reg.crm = 0;
    reg.opc1 = 0;
    reg.opc2 = 0;

    OK(uc_reg_read(uc, UC_ARM_REG_CP_REG, &reg));

    TEST_CHECK((uint32_t)((reg.val >> 31) & 1) == 0);

    OK(uc_close(uc));
}

static void test_arm_be_cpsr_sctlr(void)
{
    uc_engine *uc;
    uc_arm_cp_reg reg;
    uint32_t cpsr;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_BIG_ENDIAN, &uc));
    OK(uc_ctl_set_cpu_model(
        uc, UC_CPU_ARM_1176)); // big endian code, big endian data

    // SCTLR. See arm reference.
    reg.cp = 15;
    reg.is64 = 0;
    reg.sec = 0;
    reg.crn = 1;
    reg.crm = 0;
    reg.opc1 = 0;
    reg.opc2 = 0;

    OK(uc_reg_read(uc, UC_ARM_REG_CP_REG, &reg));
    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &cpsr));

    TEST_CHECK((reg.val & (1 << 7)) != 0);
    TEST_CHECK((cpsr & (1 << 9)) != 0);

    OK(uc_close(uc));

    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARMBE8, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_A15));

    // SCTLR. See arm reference.
    reg.cp = 15;
    reg.is64 = 0;
    reg.sec = 0;
    reg.crn = 1;
    reg.crm = 0;
    reg.opc1 = 0;
    reg.opc2 = 0;

    OK(uc_reg_read(uc, UC_ARM_REG_CP_REG, &reg));
    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &cpsr));

    // SCTLR.B == 0
    TEST_CHECK((reg.val & (1 << 7)) == 0);
    TEST_CHECK((cpsr & (1 << 9)) != 0);

    OK(uc_close(uc));
}

static void test_arm_switch_endian(void)
{
    uc_engine *uc;
    char code[] = "\x00\x00\x91\xe5"; // ldr r0, [r1]
    uint32_t r_r1 = (uint32_t)code_start;
    uint32_t r_r0, r_cpsr;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r_r1));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));

    // Little endian
    TEST_CHECK(r_r0 == 0xe5910000);

    OK(uc_reg_read(uc, UC_ARM_REG_CPSR, &r_cpsr));
    r_cpsr |= (1 << 9);
    OK(uc_reg_write(uc, UC_ARM_REG_CPSR, &r_cpsr));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));

    // Big endian
    TEST_CHECK(r_r0 == 0x000091e5);

    OK(uc_close(uc));
}

static void test_armeb_ldrb(void)
{
    uc_engine *uc;
    const char test_code[] = "\xe5\xd2\x10\x00"; // ldrb r1, [r2]
    uint64_t data_address = 0x800000;
    int r1 = 0x1234;
    int r2 = data_address;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM | UC_MODE_BIG_ENDIAN,
                    test_code, sizeof(test_code) - 1, UC_CPU_ARM_1176);

    OK(uc_mem_map(uc, data_address, 1024 * 1024, UC_PROT_ALL));
    OK(uc_mem_write(uc, data_address, "\x66\x67\x68\x69", 4));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(test_code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r1));

    TEST_CHECK(r1 == 0x66);

    OK(uc_close(uc));
}

static void test_arm_context_save(void)
{
    uc_engine *uc;
    uc_engine *uc2;
    char code[] = "\x83\xb0"; // sub    sp, #0xc
    uc_context *ctx;
    uint32_t pc;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_R5);

    OK(uc_context_alloc(uc, &ctx));
    OK(uc_context_save(uc, ctx));
    OK(uc_context_reg_read(ctx, UC_ARM_REG_PC, (void *)&pc));
    OK(uc_context_reg_write(ctx, UC_ARM_REG_PC, (void *)&pc));
    OK(uc_context_restore(uc, ctx));

    uc_common_setup(&uc2, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A7); // Note the different CPU model

    OK(uc_context_restore(uc2, ctx));

    OK(uc_context_free(ctx));
    OK(uc_close(uc));
    OK(uc_close(uc2));
}

static void test_arm_thumb2(void)
{
    uc_engine *uc;
    // MOVS  R0, #0x24
    // AND.W R0, R0, #4
    char code[] = "\x24\x20\x00\xF0\x04\x00";
    uint32_t r_r0;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_LITTLE_ENDIAN,
                    code, sizeof(code) - 1, UC_CPU_ARM_CORTEX_R5);

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));

    TEST_CHECK(r_r0 == 0x4);

    OK(uc_close(uc));
}

static void test_armeb_be32_thumb2(void)
{
    uc_engine *uc;
    // MOVS  R0, #0x24
    // AND.W R0, R0, #4
    char code[] = "\x20\x24\xF0\x00\x00\x04";
    uint32_t r_r0;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_BIG_ENDIAN, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_R5);

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r_r0));

    TEST_CHECK(r_r0 == 0x4);

    OK(uc_close(uc));
}

static bool test_arm_mem_read_write_cb(uc_engine *uc, int type,
                                       uint64_t address, int size,
                                       int64_t value, void *user_data)
{
    uint64_t *count = (uint64_t *)user_data;
    switch (type) {
    case UC_MEM_READ:
        count[0]++;
        break;
    case UC_MEM_WRITE:
        count[1]++;
        break;
    }

    return 0;
}
static void test_arm_mem_hook_read_write(void)
{
    uc_engine *uc;
    // ldr r1, [sp]
    // str r1, [sp, #4]
    // ldr r2, [sp, #4]
    // str r2, [sp]
    const char code[] =
        "\x00\x10\x9d\xe5\x04\x10\x8d\xe5\x04\x20\x9d\xe5\x00\x20\x8d\xe5";
    uint32_t r_sp;
    r_sp = 0x9000;
    uc_hook hk;
    uint64_t counter[2] = {0, 0};

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);

    uc_reg_write(uc, UC_ARM_REG_SP, &r_sp);
    uc_mem_map(uc, 0x8000, 1024 * 16, UC_PROT_ALL);

    OK(uc_hook_add(uc, &hk, UC_HOOK_MEM_READ, test_arm_mem_read_write_cb,
                   counter, 1, 0));
    OK(uc_hook_add(uc, &hk, UC_HOOK_MEM_WRITE, test_arm_mem_read_write_cb,
                   counter, 1, 0));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));

    TEST_CHECK(counter[0] == 2 && counter[1] == 2);
    OK(uc_close(uc));
}

static void test_arm_thumb_it_mem_read_cb(uc_engine *uc, uc_mem_type type,
                                          uint64_t address, int size,
                                          int64_t value, void *user_data)
{
    uint64_t *count = (uint64_t *)user_data;

    (void)uc;
    (void)type;
    (void)address;
    (void)size;
    (void)value;

    (*count)++;
}

static void test_arm_thumb_it_mem_hook(void)
{
    uc_engine *uc;
    uc_hook hk;
    uint8_t code[] = {
        0x00, 0x28, /* cmp r0, #0 */
        0x1c, 0xbf, /* itt ne */
        0x11, 0x68, /* ldrne r1, [r2] */
        0x00, 0x29, /* cmpne r1, #0 */
        0x00, 0xe0, /* b.n #0x100c */
        0x00, 0xbf, /* nop */
        0x03, 0x2c, /* cmp r4, #3 */
        0x00, 0xd3, /* bcc #0x1012 */
        0x01, 0x23, /* movs r3, #1 */
        0x02, 0x23, /* movs r3, #2 */
    };
    uint32_t r0 = 1;
    uint32_t r1 = 0;
    uint32_t r2 = code_start + 0x200;
    uint32_t r3 = 0;
    uint32_t r4 = 1;
    uint32_t data = LEINT32(1);
    uint64_t count = 0;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS,
                    (char *)code, sizeof(code), UC_CPU_ARM_CORTEX_M7);

    OK(uc_mem_write(uc, r2, &data, sizeof(data)));
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));
    OK(uc_reg_write(uc, UC_ARM_REG_R3, &r3));
    OK(uc_reg_write(uc, UC_ARM_REG_R4, &r4));

    OK(uc_hook_add(uc, &hk, UC_HOOK_MEM_READ,
                   test_arm_thumb_it_mem_read_cb, &count, 1, 0));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code), 0, 0));

    OK(uc_reg_read(uc, UC_ARM_REG_R3, &r3));
    TEST_CHECK(r3 == 2);
    TEST_CHECK(count == 1);

    OK(uc_close(uc));
}

typedef struct {
    uint64_t v0;
    uint64_t v1;
    uint64_t size;
    uint64_t pc;
} _last_cmp_info;

static void _uc_hook_sub_cmp(uc_engine *uc, uint64_t address, uint64_t arg1,
                             uint64_t arg2, uint32_t size,
                             _last_cmp_info *user_data)
{
    user_data->pc = address;
    user_data->size = size;
    user_data->v0 = arg1;
    user_data->v1 = arg2;
}

static void test_arm_tcg_opcode_cmp(void)
{
    uc_engine *uc;
    const char code[] = "\x04\x00\x9f\xe5" // ldr   r0, [pc, #4]
                        "\x04\x10\x9f\xe5" // ldr   r1, [pc, #4]
                        "\x01\x00\x50\xe1" // cmp   r0, r1
                        "\x05\x00\x00\x00" // (5)
                        "\x03\x00\x00\x00" // (3)
        ;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);

    uc_hook hook;
    _last_cmp_info cmp_info = {0};

    OK(uc_hook_add(uc, &hook, UC_HOOK_TCG_OPCODE, (void *)_uc_hook_sub_cmp,
                   (void *)&cmp_info, 1, 0, UC_TCG_OP_SUB, UC_TCG_OP_FLAG_CMP));

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 3));
    TEST_CHECK(cmp_info.v0 == 5 && cmp_info.v1 == 3);
    TEST_CHECK(cmp_info.pc == 0x1008);
    TEST_CHECK(cmp_info.size == 32);
    OK(uc_close(uc));
}

static void test_arm_thumb_tcg_opcode_cmn(void)
{
    uc_engine *uc;
    const char code[] = "\x01\x48"         // ldr  r0, [pc, #4]
                        "\x02\x49"         // ldr  r1, [pc, #8]
                        "\x00\xbf"         // nop
                        "\xc8\x42"         // cmn  r0, r1
                        "\x05\x00\x00\x00" // (5)
                        "\x03\x00\x00\x00" // (3)
        ;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);

    uc_hook hook;
    _last_cmp_info cmp_info = {0};

    OK(uc_hook_add(uc, &hook, UC_HOOK_TCG_OPCODE, (void *)_uc_hook_sub_cmp,
                   (void *)&cmp_info, 1, 0, UC_TCG_OP_SUB, UC_TCG_OP_FLAG_CMP));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 4));
    TEST_CHECK(cmp_info.v0 == 5 && cmp_info.v1 == 3);
    TEST_CHECK(cmp_info.pc == 0x1006);
    TEST_CHECK(cmp_info.size == 32);
    OK(uc_close(uc));
}

static void test_arm_cp15_c1_c0_2(void)
{
    uc_engine *uc;
    uint32_t val = 0x12345678;
    uint32_t read_val;

    // Initialize emulator in ARM mode
    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_A15));

    // Write to CP15 C1_C0_2
    OK(uc_reg_write(uc, UC_ARM_REG_C1_C0_2, &val));

    // Read from CP15 C1_C0_2
    OK(uc_reg_read(uc, UC_ARM_REG_C1_C0_2, &read_val));

    TEST_CHECK(read_val == val);

    OK(uc_close(uc));
}

static void test_arm_mrrc_cp15_c15_1_cpu(uc_cpu_arm cpu)
{
    uc_engine *uc;
    uc_arm_cp_reg reg = {
        .cp = 15,
        .is64 = 1,
        .sec = 0,
        .crm = 15,
        .opc1 = 1,
        .val = 0x0123456789abcdefULL,
    };
    const char code[] = "\x1f\x1f\x40\xec"
                        "\x1f\x1f\x50\xec";
    uint32_t r0 = 0x76543210;
    uint32_t r1 = 0x89abcdef;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    cpu);

    OK(uc_reg_write(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_reg_write(uc, UC_ARM_REG_CP_REG, &reg));
    OK(uc_reg_read(uc, UC_ARM_REG_CP_REG, &reg));
    TEST_CHECK(reg.val == 0);

    OK(uc_emu_start(uc, code_start, code_start + sizeof(code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r0));
    OK(uc_reg_read(uc, UC_ARM_REG_R1, &r1));

    TEST_CHECK(r0 == 0);
    TEST_CHECK(r1 == 0);

    OK(uc_close(uc));
}

static void test_arm_mrrc_cp15_c15_1(void)
{
    test_arm_mrrc_cp15_c15_1_cpu(UC_CPU_ARM_CORTEX_A9);
    test_arm_mrrc_cp15_c15_1_cpu(UC_CPU_ARM_CORTEX_A15);
    test_arm_mrrc_cp15_c15_1_cpu(UC_CPU_ARM_MAX);
}

static bool test_arm_v7_lpae_hook_tlb(uc_engine *uc, uint64_t addr,
                                      uc_mem_type type, uc_tlb_entry *result,
                                      void *user_data)
{
    result->paddr = addr + 0x100000000;
    result->perms = UC_PROT_ALL;
    return 1;
}

static void test_arm_v7_lpae_hook_read(uc_engine *uc, uc_mem_type type,
                                       uint64_t address, int size,
                                       uint64_t value, void *user_data)
{
    TEST_CHECK(address == 0x100001000);
}

static void test_arm_v7_lpae(void)
{
    uc_engine *uc;
    uc_hook hook_read, hook_tlb;
    uint32_t reg;
    char code[] = "\x00\x10\x90\xe5"; // ldr r1, [r0]
    OK(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_A7));

    OK(uc_ctl_tlb_mode(uc, UC_TLB_VIRTUAL));
    OK(uc_hook_add(uc, &hook_tlb, UC_HOOK_TLB_FILL, test_arm_v7_lpae_hook_tlb,
                   NULL, 1, 0));
    OK(uc_hook_add(uc, &hook_read, UC_HOOK_MEM_READ, test_arm_v7_lpae_hook_read,
                   NULL, 1, 0));

    reg = 0x1000;
    OK(uc_reg_write(uc, UC_ARM_REG_R0, &reg));
    OK(uc_mem_map(uc, 0x100001000, 0x1000, UC_PROT_ALL));
    OK(uc_mem_write(uc, 0x100001000, code, sizeof(code)));
    OK(uc_emu_start(uc, 0x1000, 0x1000 + sizeof(code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_ARM_REG_R1, &reg));
    TEST_CHECK(reg == 0xe5901000);

    OK(uc_close(uc));
}

static void test_arm_svc_interrupt(uc_engine *uc, int intno, void *user_data)
{
    uint32_t esr;
    OK(uc_reg_read(uc, UC_ARM_REG_ESR, &esr));
    switch (intno) {
    // SVC
    case 2:
        TEST_CHECK((esr & 0xff) == 0x42);
        break;
    // HVC
    case 3:
        TEST_CHECK((esr & 0xff) == 0x33);
        break;
    }
}

static void test_arm_svc_hvc_syndrome(void)
{
    uc_engine *uc;
    uint8_t code[] = {
        0x42, 0x00, 0x00, 0xef, // svc #0x42
        0x73, 0x03, 0x40, 0xe1, // hvc #0x33
    };

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, (char *)code, sizeof(code),
                    UC_CPU_ARM_CORTEX_A15);

    uc_hook hook;
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_svc_interrupt, NULL, 1,
                   0));

    OK(uc_emu_start(uc, code_start, code_start + 4, 0, 0));

    OK(uc_close(uc));
}

static int test_arm_hook_insn_wfi_callback(uc_engine *uc, void *user_data)
{
    WFI_HOOK_INSN_RESULT *result = (WFI_HOOK_INSN_RESULT *)user_data;
    result->called = true;
    return 0;
}

static void test_arm_hook_insn_wfi(void)
{
    uc_engine *uc;
    uc_hook hook;
    char code[] = "\x30\xbf";
    WFI_HOOK_INSN_RESULT result = {false};

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    OK(uc_hook_add(uc, &hook, UC_HOOK_INSN, test_arm_hook_insn_wfi_callback, &result, 1, 0,
                   UC_ARM_INS_WFI));

    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    TEST_CHECK(result.called == true);

    OK(uc_hook_del(uc, hook));
    OK(uc_close(uc));
}

typedef struct _STKOF_HOOK_RESULT {
    int intno;
    uint32_t sp;
} STKOF_HOOK_RESULT;

static void test_arm_v8m_stack_limit_intr(uc_engine *uc, int intno,
                                          void *user_data)
{
    STKOF_HOOK_RESULT *result = user_data;

    result->intno = intno;
    OK(uc_reg_read(uc, UC_ARM_REG_SP, &result->sp));
    OK(uc_emu_stop(uc));
}

static void test_arm_v8m_stack_limit_regs(void)
{
    uc_engine *uc;
    uc_hook hook;
    char code[] = "\x82\xb0"; // sub sp, #8
    const int excp_stkof = 19;
    uint32_t r_msplim, r_psplim, r_sp;
    STKOF_HOOK_RESULT result = {-1, 0};

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M33);

    r_msplim = 0x7007;
    OK(uc_reg_write(uc, UC_ARM_REG_MSPLIM, &r_msplim));
    r_psplim = 0x5fff;
    OK(uc_reg_write(uc, UC_ARM_REG_PSPLIM, &r_psplim));

    OK(uc_reg_read(uc, UC_ARM_REG_MSPLIM, &r_msplim));
    OK(uc_reg_read(uc, UC_ARM_REG_PSPLIM, &r_psplim));
    TEST_CHECK(r_msplim == 0x7000);
    TEST_CHECK(r_psplim == 0x5ff8);

    r_sp = 0x7010;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_stack_limit_intr,
                   &result, 1, 0));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_ARM_REG_SP, &r_sp));
    TEST_CHECK(result.intno == -1);
    TEST_CHECK(r_sp == 0x7008);

    r_sp = 0x7004;
    OK(uc_reg_write(uc, UC_ARM_REG_SP, &r_sp));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    TEST_CHECK(result.intno == excp_stkof);
    TEST_CHECK(result.sp == 0x7004);

    OK(uc_hook_del(uc, hook));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_MSPLIM, &r_msplim));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_PSPLIM, &r_psplim));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_MSPLIM, &r_msplim));
    OK(uc_close(uc));
}

#define EXIT_AFTER_INSN_MMIO 0x40000000
#define EXIT_AFTER_INSN_LOG 8

typedef struct _EXIT_AFTER_INSN_STATE {
    uint32_t read16;
    int request_on;
    uc_err request_err;
    int count;
    uint64_t log[EXIT_AFTER_INSN_LOG];
} EXIT_AFTER_INSN_STATE;

typedef struct _EXIT_AFTER_INSN_REGS {
    uint32_t r[5];
    uint32_t pc;
} EXIT_AFTER_INSN_REGS;

static void exit_after_insn_access(uc_engine *uc, EXIT_AFTER_INSN_STATE *s,
                                   bool write, uint64_t offset, uint64_t value)
{
    if (s->count < EXIT_AFTER_INSN_LOG) {
        s->log[s->count] =
            ((uint64_t)write << 63) | (offset << 32) | (uint32_t)value;
    }
    s->count++;
    if (s->count == s->request_on) {
        s->request_err = uc_ctl_exit_after_insn(uc);
    }
}

static uint64_t exit_after_insn_read(uc_engine *uc, uint64_t offset,
                                     unsigned size, void *user_data)
{
    EXIT_AFTER_INSN_STATE *s = user_data;
    uint64_t value = offset == 16 ? s->read16 : 0xabcd0000 + offset;

    exit_after_insn_access(uc, s, false, offset, value);
    return value;
}

static void exit_after_insn_write(uc_engine *uc, uint64_t offset,
                                  unsigned size, uint64_t value,
                                  void *user_data)
{
    exit_after_insn_access(uc, user_data, true, offset, value);
}

static uc_engine *exit_after_insn_open(uc_mode mode, uc_cpu_arm cpu,
                                       uint64_t address, const char *code,
                                       uint64_t size, EXIT_AFTER_INSN_STATE *s)
{
    uc_engine *uc;
    uint32_t r[5] = {0x11, EXIT_AFTER_INSN_MMIO, 0, 0x33, 0};
    int i;

    OK(uc_open(UC_ARCH_ARM, mode, &uc));
    OK(uc_ctl_set_cpu_model(uc, cpu));
    OK(uc_mem_map(uc, code_start, code_len, UC_PROT_ALL));
    OK(uc_mem_write(uc, address, code, size));
    OK(uc_mmio_map(uc, EXIT_AFTER_INSN_MMIO, 0x1000, exit_after_insn_read, s,
                   exit_after_insn_write, s));
    for (i = 0; i < 5; i++) {
        OK(uc_reg_write(uc, UC_ARM_REG_R0 + i, &r[i]));
    }
    return uc;
}

static EXIT_AFTER_INSN_REGS exit_after_insn_regs(uc_engine *uc)
{
    EXIT_AFTER_INSN_REGS regs;
    int i;

    for (i = 0; i < 5; i++) {
        OK(uc_reg_read(uc, UC_ARM_REG_R0 + i, &regs.r[i]));
    }
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &regs.pc));
    return regs;
}

static EXIT_AFTER_INSN_REGS
exit_after_insn_check(uc_mode mode, uc_cpu_arm cpu, uint64_t address,
                      const char *code, uint64_t size, uint32_t read16,
                      uint32_t stop_pc, int stop_accesses, uint64_t taken)
{
    uint64_t taken_read;
    uint64_t thumb = (mode & UC_MODE_THUMB) ? 1 : 0;
    uint64_t end = address + size;
    EXIT_AFTER_INSN_STATE ref = {read16};
    EXIT_AFTER_INSN_STATE run = {read16, 1};
    EXIT_AFTER_INSN_REGS ref_regs, stop_regs, end_regs;
    uc_engine *uc;

    uc = exit_after_insn_open(mode, cpu, address, code, size, &ref);
    OK(uc_emu_start(uc, address | thumb, end, 0, 0));
    ref_regs = exit_after_insn_regs(uc);
    OK(uc_close(uc));
    TEST_CHECK(ref_regs.pc == end);
    TEST_CHECK(ref.count >= stop_accesses);

    uc = exit_after_insn_open(mode, cpu, address, code, size, &run);
    OK(uc_ctl_exit_after_insn_enable(uc));
    OK(uc_emu_start(uc, address | thumb, end, 0, 0));
    OK(run.request_err);
    stop_regs = exit_after_insn_regs(uc);
    TEST_CHECK(stop_regs.pc == stop_pc);
    TEST_MSG("pc 0x%x, expected 0x%x", stop_regs.pc, stop_pc);
    TEST_CHECK(run.count == stop_accesses);
    TEST_MSG("%d accesses, expected %d", run.count, stop_accesses);
    OK(uc_ctl_exit_after_insn_taken(uc, &taken_read));
    TEST_CHECK(taken_read == taken);
    TEST_MSG("taken 0x%" PRIx64 ", expected 0x%" PRIx64, taken_read, taken);

    run.request_on = 0;
    OK(uc_emu_start(uc, stop_regs.pc | thumb, end, 0, 0));
    end_regs = exit_after_insn_regs(uc);
    OK(uc_ctl_exit_after_insn_taken(uc, &taken_read));
    TEST_CHECK(taken_read == 0);
    OK(uc_close(uc));
    TEST_CHECK(memcmp(&end_regs, &ref_regs, sizeof(ref_regs)) == 0);
    TEST_CHECK(run.count == ref.count);
    TEST_CHECK(memcmp(run.log, ref.log, sizeof(ref.log)) == 0);
    return stop_regs;
}

static EXIT_AFTER_INSN_REGS exit_after_insn_check_m(const char *code,
                                                    uint64_t size,
                                                    uint32_t stop_offset,
                                                    int stop_accesses,
                                                    uint32_t taken_offset)
{
    return exit_after_insn_check(UC_MODE_THUMB | UC_MODE_MCLASS,
                                 UC_CPU_ARM_CORTEX_M33, code_start, code, size,
                                 0, code_start + stop_offset, stop_accesses,
                                 code_start + taken_offset);
}

/** str r0, [r1]; adds r2, #1; str r3, [r1, #4]; adds r2, #1 */
static void test_arm_exit_after_insn_str(void)
{
    char code[] = "\x08\x60\x01\x32\x4b\x60\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 2, 1, 0);

    TEST_CHECK(regs.r[2] == 0);
}

/** str r0, [r1], #4; adds r2, #1; str r3, [r1]; adds r2, #1 */
static void test_arm_exit_after_insn_str_post_index(void)
{
    char code[] = "\x41\xf8\x04\x0b\x01\x32\x0b\x60\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 4, 1, 0);

    TEST_CHECK(regs.r[1] == EXIT_AFTER_INSN_MMIO + 4);
    TEST_CHECK(regs.r[2] == 0);
}

/** strd r0, r3, [r1]; adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_strd(void)
{
    char code[] = "\xc1\xe9\x00\x03\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 4, 2, 0);

    TEST_CHECK(regs.r[2] == 0);
}

/** stmia r1!, {r0, r2, r3}; adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_stm(void)
{
    char code[] = "\x0d\xc1\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 2, 3, 0);

    TEST_CHECK(regs.r[1] == EXIT_AFTER_INSN_MMIO + 12);
    TEST_CHECK(regs.r[2] == 0);
}

/** ldr r4, [r1]; adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_ldr(void)
{
    char code[] = "\x0c\x68\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 2, 1, 0);

    TEST_CHECK(regs.r[4] == 0xabcd0000);
    TEST_CHECK(regs.r[2] == 0);
}

/** cmp r0, r0; itt eq; streq r0, [r1]; addeq r2, #1; adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_it(void)
{
    char code[] = "\x80\x42\x04\xbf\x08\x60\x01\x32\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 8, 1, 6);

    TEST_CHECK(regs.r[2] == 1);
}

/**
 * cmp r0, r0; itttt eq; streq r0, [r1]; addeq r2, #1; streq r3, [r1, #4];
 * addeq r2, #1; adds r2, #1
 */
static void test_arm_exit_after_insn_it_long(void)
{
    char code[] = "\x80\x42\x01\xbf\x08\x60\x01\x32\x4b\x60\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 12, 2, 10);

    TEST_CHECK(regs.r[2] == 2);
}

/**
 * cmp r0, r0; it eq; streq r0, [r1]; adds r2, #1; adds r2, #1;
 * str r3, [r1, #8]; adds r2, #1
 */
static void test_arm_exit_after_insn_it_tail(void)
{
    char code[] = "\x80\x42\x08\xbf\x08\x60\x01\x32\x01\x32\x8b\x60\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 6, 1, 4);

    TEST_CHECK(regs.r[2] == 0);
}

static void exit_after_insn_block_cb(uc_engine *uc, uint64_t address,
                                     uint32_t size, void *user_data)
{
    if (address == 0x2000) {
        *(bool *)user_data = true;
    }
}

/**
 * At 0x1ffa, so that the IT block spans two TBs:
 * cmp r0, r0; itt eq; streq r0, [r1]; addeq r2, #1 (at 0x2000);
 * adds r2, #1; str r3, [r1, #4]; adds r2, #1
 */
static void test_arm_exit_after_insn_it_across_tbs(void)
{
    char code[] = "\x80\x42\x04\xbf\x08\x60\x01\x32\x01\x32\x4b\x60\x01\x32";
    uint64_t address = 0x1ffa;
    EXIT_AFTER_INSN_STATE s = {0};
    EXIT_AFTER_INSN_REGS regs;
    bool split = false;
    uc_hook hook;
    uc_engine *uc;

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, address, code,
                              sizeof(code) - 1, &s);
    OK(uc_hook_add(uc, &hook, UC_HOOK_BLOCK, exit_after_insn_block_cb, &split,
                   1, 0));
    OK(uc_emu_start(uc, address | 1, address + sizeof(code) - 1, 0, 0));
    OK(uc_close(uc));
    TEST_CHECK(split);

    regs = exit_after_insn_check(UC_MODE_THUMB | UC_MODE_MCLASS,
                                 UC_CPU_ARM_CORTEX_M33, address, code,
                                 sizeof(code) - 1, 0, 0x2002, 1, 0x2000);
    TEST_CHECK(regs.r[2] == 1);
}

/** ldr pc, [r1, #16]; adds r2, #1; target: adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_ldr_pc(void)
{
    char code[] = "\xd1\xf8\x10\xf0\x01\x32\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs = exit_after_insn_check(
        UC_MODE_THUMB | UC_MODE_MCLASS, UC_CPU_ARM_CORTEX_M33, code_start,
        code, sizeof(code) - 1, (code_start + 6) | 1, code_start + 6, 1,
        code_start);

    TEST_CHECK(regs.r[2] == 0);
}

/** ldr pc, [r1, #16]; adds r2, #1; target: adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_ldr_pc_a_profile(void)
{
    char code[] = "\xd1\xf8\x10\xf0\x01\x32\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs = exit_after_insn_check(
        UC_MODE_THUMB, UC_CPU_ARM_CORTEX_A15, code_start, code,
        sizeof(code) - 1, (code_start + 6) | 1, code_start + 6, 1,
        code_start);

    TEST_CHECK(regs.r[2] == 0);
}

/** str r0, [r1]; b 1f; adds r2, #1; 1: adds r2, #1 */
static void test_arm_exit_after_insn_str_b(void)
{
    char code[] = "\x08\x60\x00\xe0\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 2, 1, 0);

    TEST_CHECK(regs.r[2] == 0);
}

/**
 * A32: str r0, [r1]; add r2, r2, #1; str r3, [r1, #4]; add r2, r2, #1
 */
static void test_arm_exit_after_insn_a32_str(void)
{
    char code[] = "\x00\x00\x81\xe5\x01\x20\x82\xe2\x04\x30\x81\xe5\x01\x20"
                  "\x82\xe2";
    EXIT_AFTER_INSN_REGS regs = exit_after_insn_check(
        UC_MODE_ARM, UC_CPU_ARM_CORTEX_A15, code_start, code, sizeof(code) - 1,
        0, code_start + 4, 1, code_start);

    TEST_CHECK(regs.r[2] == 0);
}

/**
 * A32: ldr pc, [r1, #16]; add r2, r2, #1; target: add r2, r2, #1;
 * add r2, r2, #1
 */
static void test_arm_exit_after_insn_a32_ldr_pc(void)
{
    char code[] = "\x10\xf0\x91\xe5\x01\x20\x82\xe2\x01\x20\x82\xe2\x01\x20"
                  "\x82\xe2";
    EXIT_AFTER_INSN_REGS regs = exit_after_insn_check(
        UC_MODE_ARM, UC_CPU_ARM_CORTEX_A15, code_start, code, sizeof(code) - 1,
        code_start + 8, code_start + 8, 1, code_start);

    TEST_CHECK(regs.r[2] == 0);
}

/**
 * cmp r0, r0; itt eq; streq r0, [r1]; beq.w 1f; adds r2, #1; adds r2, #1;
 * 1: adds r2, #1; adds r2, #1
 */
static void test_arm_exit_after_insn_it_b_taken(void)
{
    char code[] = "\x80\x42\x04\xbf\x08\x60\x00\xf0\x02\xb8\x01\x32\x01\x32"
                  "\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 0xe, 1, 6);

    TEST_CHECK(regs.r[2] == 0);
}

/**
 * cmp r0, r0; ite eq; streq r0, [r1]; bne.w 1f; adds r2, #1;
 * 1: adds r2, #1
 */
static void test_arm_exit_after_insn_it_b_not_taken(void)
{
    char code[] = "\x80\x42\x0c\xbf\x08\x60\x00\xf0\x01\xb8\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 0xa, 1, 6);

    TEST_CHECK(regs.r[2] == 0);
}

/** cmp r0, r0; ite eq; streq r0, [r1]; popne {pc}; adds r2, #1; adds r2, #1 */
static void test_arm_exit_after_insn_it_pop_skipped(void)
{
    char code[] = "\x80\x42\x0c\xbf\x08\x60\x00\xbd\x01\x32\x01\x32";
    EXIT_AFTER_INSN_REGS regs =
        exit_after_insn_check_m(code, sizeof(code) - 1, 8, 1, 6);

    TEST_CHECK(regs.r[2] == 0);
}

typedef struct _EXIT_AFTER_INSN_NESTED {
    bool outer_request;
    bool nested_request;
    int count;
    uc_err outer_err;
    uc_err nested_err;
    uint32_t nested_pc;
} EXIT_AFTER_INSN_NESTED;

static void exit_after_insn_nested_write(uc_engine *uc, uint64_t offset,
                                         unsigned size, uint64_t value,
                                         void *user_data)
{
    EXIT_AFTER_INSN_NESTED *n = user_data;

    if (offset == 8) {
        if (n->nested_request) {
            n->nested_err = uc_ctl_exit_after_insn(uc);
        }
        return;
    }
    n->count++;
    if (n->count == 1) {
        if (n->outer_request) {
            n->outer_err = uc_ctl_exit_after_insn(uc);
        }
        OK(uc_emu_start(uc, (code_start + 0x100) | 1, code_start + 0x104, 0,
                        0));
        OK(uc_reg_read(uc, UC_ARM_REG_PC, &n->nested_pc));
    }
}

/**
 * str r0, [r1]; adds r2, #1; str r3, [r1, #4]; adds r2, #1, and
 * str r3, [r1, #8]; nop at code_start + 0x100, which the first outer write
 * callback runs with a nested uc_emu_start.
 */
static void exit_after_insn_nested(bool outer_request, bool nested_request)
{
    char code[] = "\x08\x60\x01\x32\x4b\x60\x01\x32";
    EXIT_AFTER_INSN_NESTED n = {outer_request, nested_request};
    uint64_t end = code_start + sizeof(code) - 1;
    uint64_t taken;
    uint32_t pc;
    uc_engine *uc;

    OK(uc_open(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, &uc));
    OK(uc_ctl_set_cpu_model(uc, UC_CPU_ARM_CORTEX_M33));
    OK(uc_mem_map(uc, code_start, code_len, UC_PROT_ALL));
    OK(uc_mem_write(uc, code_start, code, sizeof(code) - 1));
    OK(uc_mem_write(uc, code_start + 0x100, "\x8b\x60\x00\xbf", 4));
    OK(uc_mmio_map(uc, EXIT_AFTER_INSN_MMIO, 0x1000, NULL, NULL,
                   exit_after_insn_nested_write, &n));
    pc = EXIT_AFTER_INSN_MMIO;
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &pc));
    OK(uc_ctl_exit_after_insn_enable(uc));

    OK(uc_emu_start(uc, code_start | 1, end, 0, 0));
    OK(n.outer_err);
    OK(n.nested_err);
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &pc));
    OK(uc_ctl_exit_after_insn_taken(uc, &taken));
    TEST_CHECK(n.nested_pc == code_start + (nested_request ? 0x102 : 0x104));
    if (outer_request) {
        TEST_CHECK(pc == code_start + 2);
        TEST_CHECK(n.count == 1);
        TEST_CHECK(taken == code_start);
    } else {
        TEST_CHECK(pc == end);
        TEST_CHECK(n.count == 2);
        TEST_CHECK(taken == 0);
    }
    TEST_MSG("pc 0x%x, %d outer writes, taken 0x%" PRIx64, pc, n.count,
             taken);
    OK(uc_close(uc));
}

static void test_arm_exit_after_insn_nested_emu_start(void)
{
    exit_after_insn_nested(true, false);
}

static void test_arm_exit_after_insn_nested_takes(void)
{
    exit_after_insn_nested(false, true);
}

static void test_arm_exit_after_insn_nested_and_outer_take(void)
{
    exit_after_insn_nested(true, true);
}

static void exit_after_insn_enable_write(uc_engine *uc, uint64_t offset,
                                         unsigned size, uint64_t value,
                                         void *user_data)
{
    uc_err *err = user_data;

    *err = uc_ctl_exit_after_insn_disable(uc);
}

static void test_arm_exit_after_insn_enable_while_running(void)
{
    char code[] = "\x08\x60\x01\x32";
    uc_err err = UC_ERR_OK;
    uint32_t r1 = EXIT_AFTER_INSN_MMIO;
    uc_engine *uc;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_M33);
    OK(uc_mmio_map(uc, EXIT_AFTER_INSN_MMIO, 0x1000, NULL, NULL,
                   exit_after_insn_enable_write, &err));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    OK(uc_ctl_exit_after_insn_enable(uc));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    uc_assert_err(UC_ERR_ARG, err);
    OK(uc_close(uc));
}

static void test_arm_exit_after_insn_not_enabled(void)
{
    char code[] = "\x08\x60\x01\x32\x4b\x60\x01\x32";
    EXIT_AFTER_INSN_STATE s = {0, 1};
    uint64_t taken = 1;
    uint32_t pc;
    uc_engine *uc;

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, code_start, code,
                              sizeof(code) - 1, &s);
    uc_assert_err(UC_ERR_ARG, uc_ctl_exit_after_insn(uc));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    uc_assert_err(UC_ERR_ARG, s.request_err);
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &pc));
    TEST_CHECK(pc == code_start + sizeof(code) - 1);
    TEST_CHECK(s.count == 2);

    OK(uc_ctl_exit_after_insn_taken(uc, &taken));
    TEST_CHECK(taken == 0);
    OK(uc_ctl_exit_after_insn_enable(uc));
    OK(uc_ctl_exit_after_insn_disable(uc));
    uc_assert_err(UC_ERR_ARG, uc_ctl_exit_after_insn(uc));
    OK(uc_close(uc));
}

/**
 * stmia r1!, {r0, r2, r3}; cmp r0, r0; itt eq; streq r0, [r1];
 * addeq r2, #1; ldr r4, [r1]; adds r2, #1
 */
static void test_arm_exit_after_insn_never_requested(void)
{
    char code[] = "\x0d\xc1\x80\x42\x04\xbf\x08\x60\x01\x32\x0c\x68\x01\x32";
    EXIT_AFTER_INSN_STATE ref = {0};
    EXIT_AFTER_INSN_STATE run = {0};
    EXIT_AFTER_INSN_REGS ref_regs, run_regs;
    uint64_t taken = 1;
    uint64_t end = code_start + sizeof(code) - 1;
    uc_engine *uc;

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, code_start, code,
                              sizeof(code) - 1, &ref);
    OK(uc_emu_start(uc, code_start | 1, end, 0, 0));
    ref_regs = exit_after_insn_regs(uc);
    OK(uc_close(uc));

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, code_start, code,
                              sizeof(code) - 1, &run);
    OK(uc_ctl_exit_after_insn_enable(uc));
    OK(uc_emu_start(uc, code_start | 1, end, 0, 0));
    run_regs = exit_after_insn_regs(uc);
    OK(uc_ctl_exit_after_insn_taken(uc, &taken));
    TEST_CHECK(taken == 0);
    OK(uc_close(uc));

    TEST_CHECK(ref_regs.pc == end);
    TEST_CHECK(memcmp(&run_regs, &ref_regs, sizeof(ref_regs)) == 0);
    TEST_CHECK(run.count == 5);
    TEST_CHECK(run.count == ref.count);
    TEST_CHECK(memcmp(run.log, ref.log, sizeof(ref.log)) == 0);
}

static void test_arm_exit_after_insn_cleared_by_emu_start(void)
{
    char code[] = "\x08\x60\x01\x32\x4b\x60\x01\x32";
    EXIT_AFTER_INSN_STATE s = {0};
    uint32_t pc;
    uc_engine *uc;

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, code_start, code,
                              sizeof(code) - 1, &s);
    OK(uc_ctl_exit_after_insn_enable(uc));
    OK(uc_ctl_exit_after_insn(uc));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &pc));
    TEST_CHECK(pc == code_start + sizeof(code) - 1);
    TEST_CHECK(s.count == 2);
    OK(uc_close(uc));
}

/**
 * str r0, [r1]; adds r2, #1; b 1f; 1: str r3, [r1, #4]; adds r2, #1
 * The branch keeps the first TB clear of the one around the until address,
 * which uc_emu_start invalidates when it returns.
 */
static void test_arm_exit_after_insn_enable_flushes(void)
{
    char code[] = "\x08\x60\x01\x32\xff\xe7\x4b\x60\x01\x32";
    EXIT_AFTER_INSN_STATE s = {0};
    uint32_t pc;
    uint32_t r2 = 0;
    uc_engine *uc;

    uc = exit_after_insn_open(UC_MODE_THUMB | UC_MODE_MCLASS,
                              UC_CPU_ARM_CORTEX_M33, code_start, code,
                              sizeof(code) - 1, &s);
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    TEST_CHECK(s.count == 2);

    OK(uc_ctl_exit_after_insn_enable(uc));
    s.count = 0;
    s.request_on = 1;
    OK(uc_reg_write(uc, UC_ARM_REG_R2, &r2));
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code) - 1, 0, 0));
    OK(s.request_err);
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &pc));
    TEST_CHECK(pc == code_start + 2);
    TEST_CHECK(s.count == 1);
    OK(uc_close(uc));
}

static void test_arm_exit_after_insn_other_arch(void)
{
    uc_engine *uc;

    OK(uc_open(UC_ARCH_X86, UC_MODE_32, &uc));
    uc_assert_err(UC_ERR_ARG, uc_ctl_exit_after_insn_enable(uc));
    uc_assert_err(UC_ERR_ARG, uc_ctl_exit_after_insn(uc));
    OK(uc_close(uc));

    OK(uc_open(UC_ARCH_ARM64, UC_MODE_ARM, &uc));
    uc_assert_err(UC_ERR_ARG, uc_ctl_exit_after_insn_enable(uc));
    OK(uc_close(uc));
}

TEST_LIST = {{"test_arm_nop", test_arm_nop},
             {"test_arm_thumb_sub", test_arm_thumb_sub},
             {"test_armeb_sub", test_armeb_sub},
             {"test_armeb_be8_sub", test_armeb_be8_sub},
             {"test_arm_thumbeb_sub", test_arm_thumbeb_sub},
             {"test_arm_thumb_ite", test_arm_thumb_ite},
             {"test_arm_m_thumb_mrs", test_arm_m_thumb_mrs},
             {"test_arm_m_control", test_arm_m_control},
             {"test_arm_m_unprivileged_special_regs",
              test_arm_m_unprivileged_special_regs},
             {"test_arm_m_exc_return", test_arm_m_exc_return},
             {"test_arm_und32_to_svc32", test_arm_und32_to_svc32},
             {"test_arm_usr32_to_svc32", test_arm_usr32_to_svc32},
             {"test_arm_v8", test_arm_v8},
             {"test_arm_thumb_smlabb", test_arm_thumb_smlabb},
             {"test_arm_not_allow_privilege_escalation",
              test_arm_not_allow_privilege_escalation},
             {"test_arm_mrc", test_arm_mrc},
             {"test_arm_hflags_rebuilt", test_arm_hflags_rebuilt},
             {"test_arm_mem_access_abort", test_arm_mem_access_abort},
             {"test_arm_read_sctlr", test_arm_read_sctlr},
             {"test_arm_be_cpsr_sctlr", test_arm_be_cpsr_sctlr},
             {"test_arm_switch_endian", test_arm_switch_endian},
             {"test_armeb_ldrb", test_armeb_ldrb},
             {"test_arm_context_save", test_arm_context_save},
             {"test_arm_thumb2", test_arm_thumb2},
             {"test_armeb_be32_thumb2", test_armeb_be32_thumb2},
             {"test_arm_mem_hook_read_write", test_arm_mem_hook_read_write},
             {"test_arm_thumb_it_mem_hook", test_arm_thumb_it_mem_hook},
             {"test_arm_tcg_opcode_cmp", test_arm_tcg_opcode_cmp},
             {"test_arm_thumb_tcg_opcode_cmn", test_arm_thumb_tcg_opcode_cmn},
             {"test_arm_cp15_c1_c0_2", test_arm_cp15_c1_c0_2},
             {"test_arm_mrrc_cp15_c15_1", test_arm_mrrc_cp15_c15_1},
             {"test_arm_v7_lpae", test_arm_v7_lpae},
             {"test_arm_svc_hvc_syndrome", test_arm_svc_hvc_syndrome},
             {"test_arm_hook_insn_wfi", test_arm_hook_insn_wfi},
             {"test_arm_v8m_stack_limit_regs", test_arm_v8m_stack_limit_regs},
             {"test_arm_exit_after_insn_str", test_arm_exit_after_insn_str},
             {"test_arm_exit_after_insn_str_post_index",
              test_arm_exit_after_insn_str_post_index},
             {"test_arm_exit_after_insn_strd", test_arm_exit_after_insn_strd},
             {"test_arm_exit_after_insn_stm", test_arm_exit_after_insn_stm},
             {"test_arm_exit_after_insn_ldr", test_arm_exit_after_insn_ldr},
             {"test_arm_exit_after_insn_it", test_arm_exit_after_insn_it},
             {"test_arm_exit_after_insn_it_long",
              test_arm_exit_after_insn_it_long},
             {"test_arm_exit_after_insn_it_tail",
              test_arm_exit_after_insn_it_tail},
             {"test_arm_exit_after_insn_it_across_tbs",
              test_arm_exit_after_insn_it_across_tbs},
             {"test_arm_exit_after_insn_ldr_pc",
              test_arm_exit_after_insn_ldr_pc},
             {"test_arm_exit_after_insn_ldr_pc_a_profile",
              test_arm_exit_after_insn_ldr_pc_a_profile},
             {"test_arm_exit_after_insn_str_b", test_arm_exit_after_insn_str_b},
             {"test_arm_exit_after_insn_a32_str",
              test_arm_exit_after_insn_a32_str},
             {"test_arm_exit_after_insn_a32_ldr_pc",
              test_arm_exit_after_insn_a32_ldr_pc},
             {"test_arm_exit_after_insn_it_b_taken",
              test_arm_exit_after_insn_it_b_taken},
             {"test_arm_exit_after_insn_it_b_not_taken",
              test_arm_exit_after_insn_it_b_not_taken},
             {"test_arm_exit_after_insn_it_pop_skipped",
              test_arm_exit_after_insn_it_pop_skipped},
             {"test_arm_exit_after_insn_nested_emu_start",
              test_arm_exit_after_insn_nested_emu_start},
             {"test_arm_exit_after_insn_nested_takes",
              test_arm_exit_after_insn_nested_takes},
             {"test_arm_exit_after_insn_nested_and_outer_take",
              test_arm_exit_after_insn_nested_and_outer_take},
             {"test_arm_exit_after_insn_enable_while_running",
              test_arm_exit_after_insn_enable_while_running},
             {"test_arm_exit_after_insn_not_enabled",
              test_arm_exit_after_insn_not_enabled},
             {"test_arm_exit_after_insn_never_requested",
              test_arm_exit_after_insn_never_requested},
             {"test_arm_exit_after_insn_cleared_by_emu_start",
              test_arm_exit_after_insn_cleared_by_emu_start},
             {"test_arm_exit_after_insn_enable_flushes",
              test_arm_exit_after_insn_enable_flushes},
             {"test_arm_exit_after_insn_other_arch",
              test_arm_exit_after_insn_other_arch},
             {NULL, NULL}};
