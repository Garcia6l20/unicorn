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

#define V8M_EXCP_PREFETCH_ABORT 3
#define V8M_STEP_BOUND 16

typedef struct _V8M_RUN {
    int intno;
    uint32_t pc;
} V8M_RUN;

typedef struct _V8M_IDAU {
    uint32_t exempt_base;
    uint32_t exempt_limit;
    int calls;
} V8M_IDAU;

static void test_arm_v8m_intr(uc_engine *uc, int intno, void *user_data)
{
    V8M_RUN *run = user_data;

    if (run->intno == -1) {
        run->intno = intno;
        OK(uc_reg_read(uc, UC_ARM_REG_PC, &run->pc));
    }
    OK(uc_emu_stop(uc));
}

static void test_arm_v8m_idau(void *user, uint32_t address, int *region,
                              bool *exempt, bool *ns, bool *nsc)
{
    V8M_IDAU *idau = user;

    idau->calls++;
    if (address >= idau->exempt_base && address <= idau->exempt_limit) {
        *exempt = true;
    }
}

static uc_engine *test_arm_v8m_open(const char *code, uint64_t size,
                                    V8M_RUN *run)
{
    uc_engine *uc;
    uc_hook hook;

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, code,
                    size, UC_CPU_ARM_CORTEX_M33);
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_intr, run, 1, 0));
    return uc;
}

static void test_arm_v8m_run(uc_engine *uc, V8M_RUN *run, uint64_t begin,
                             uint64_t until)
{
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, begin | 1, until, 0, V8M_STEP_BOUND));
}

static uc_engine *test_arm_v8m_bxns_setup(V8M_RUN *run, V8M_IDAU *idau)
{
    char code[] = "\x0c\x47";
    char target[] = "\x5a\x20";
    uint32_t r1 = 0x3000;
    uc_engine *uc = test_arm_v8m_open(code, sizeof(code) - 1, run);

    OK(uc_mem_write(uc, 0x3000, target, sizeof(target) - 1));
    OK(uc_ctl_arm_idau(uc, test_arm_v8m_idau, idau));
    OK(uc_reg_write(uc, UC_ARM_REG_R1, &r1));
    return uc;
}

static void test_arm_v8m_lookup_on(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_IDAU idau = {0x3000, 0x33ff, 0};
    uint32_t r0 = 0;
    int ignored = 0;

    uc = test_arm_v8m_bxns_setup(&run, &idau);
    test_arm_v8m_run(uc, &run, code_start, 0x3002);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == 0x3000);
    OK(uc_close(uc));

    uc = test_arm_v8m_bxns_setup(&run, &idau);
    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_run(uc, &run, code_start, 0x3002);
    OK(uc_reg_read(uc, UC_ARM_REG_R0, &r0));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(r0 == 0x5a);

    OK(uc_ctl_arm_security(uc, 0));
    test_arm_v8m_run(uc, &run, 0x3000, 0x3002);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == 0x3000);

    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_run(uc, &run, 0x3000, 0x3002);
    TEST_CHECK(run.intno == -1);

    OK(uc_ctl_arm_idau(uc, NULL, NULL));
    test_arm_v8m_run(uc, &run, 0x3000, 0x3002);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == 0x3000);

    uc_assert_err(UC_ERR_ARG,
                  uc_ctl(uc, UC_CTL_READ(UC_CTL_ARM_SECURITY, 1), &ignored));
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl(uc, UC_CTL_READ(UC_CTL_ARM_IDAU, 1), &ignored));
    OK(uc_close(uc));
}

static void test_arm_v8m_security_off(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_IDAU idau = {0x3000, 0x33ff, 0};

    uc = test_arm_v8m_bxns_setup(&run, &idau);
    test_arm_v8m_run(uc, &run, code_start, 0x3002);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(idau.calls == 0);

    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_run(uc, &run, 0x3000, 0x3002);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(idau.calls > 0);
    OK(uc_close(uc));
}

static void test_arm_v8m_security_state(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_IDAU idau = {0x3000, 0x33ff, 0};
    uint32_t value;
    char code[] = "\x00\xbf";

    uc = test_arm_v8m_bxns_setup(&run, &idau);
    OK(uc_reg_read(uc, UC_ARM_REG_SECURE, &value));
    TEST_CHECK(value == 1);

    value = 0x20001000;
    OK(uc_reg_write(uc, UC_ARM_REG_MSP, &value));
    value = 0;
    OK(uc_reg_write(uc, UC_ARM_REG_SECURE, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_SECURE, &value));
    TEST_CHECK(value == 0);
    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0);

    value = 0x20002000;
    OK(uc_reg_write(uc, UC_ARM_REG_MSP, &value));
    value = 0;
    OK(uc_reg_write(uc, UC_ARM_REG_SECURE, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0x20002000);

    value = 1;
    OK(uc_reg_write(uc, UC_ARM_REG_SECURE, &value));
    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0x20001000);

    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_run(uc, &run, code_start, 0x3002);
    TEST_CHECK(run.intno == -1);
    OK(uc_reg_read(uc, UC_ARM_REG_SECURE, &value));
    TEST_CHECK(value == 0);
    OK(uc_reg_read(uc, UC_ARM_REG_MSP, &value));
    TEST_CHECK(value == 0x20002000);
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_SECURE, &value));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_SECURE, &value));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_SECURE, &value));
    OK(uc_close(uc));
}

#define V8M_EXCP_DATA_ABORT 4
#define V8M_SAU_ENABLE 1
#define V8M_TT_SRVALID (1u << 17)
#define V8M_TT_S (1u << 22)
#define V8M_TT_IRVALID (1u << 23)
#define V8M_LOOP_COUNT 1000

static void test_arm_v8m_reg(uc_engine *uc, int regid, uint32_t value)
{
    OK(uc_reg_write(uc, regid, &value));
}

static uint32_t test_arm_v8m_get(uc_engine *uc, int regid)
{
    uint32_t value = 0;

    OK(uc_reg_read(uc, regid, &value));
    return value;
}

static void test_arm_v8m_sau_region(uc_engine *uc, uint32_t region,
                                    uint32_t base, uint32_t rlar)
{
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, region);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RBAR, base);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RLAR, rlar);
}

static uc_engine *test_arm_v8m_ns_open(const char *code, uint64_t size,
                                       V8M_RUN *run)
{
    uc_engine *uc = test_arm_v8m_open(code, size, run);

    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_sau_region(uc, 7, 0x1000, 0x13e0 | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 0);
    return uc;
}

static void test_arm_v8m_load(uc_engine *uc, V8M_RUN *run, uint32_t address)
{
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, address);
    test_arm_v8m_run(uc, run, code_start, code_start + 2);
}

static void test_arm_v8m_sau_attribution(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x08\x68";
    uint32_t ignored = 0;

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    test_arm_v8m_sau_region(uc, 0, 0x4000, 0x43e0 | V8M_SAU_ENABLE);

    test_arm_v8m_load(uc, &run, 0x4100);
    TEST_CHECK(run.intno == -1);
    test_arm_v8m_load(uc, &run, 0x4400);
    TEST_CHECK(run.intno == V8M_EXCP_DATA_ABORT);

    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RLAR, 0x4000 | V8M_SAU_ENABLE);
    test_arm_v8m_load(uc, &run, 0x4100);
    TEST_CHECK(run.intno == V8M_EXCP_DATA_ABORT);

    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, 3);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RBAR, 0x5013);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RLAR, 0x53ff);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, 0xff);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SAU_RBAR) == 0x5000);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SAU_RLAR) == 0x53e3);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SAU_CTRL) == 3);
    uc_assert_err(UC_ERR_ARG,
                  uc_reg_write(uc, UC_ARM_REG_SAU_RNR, &(uint32_t){8}));
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SAU_RNR) == 3);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SAU_RLAR) ==
               (0x4000 | V8M_SAU_ENABLE));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_SAU_CTRL, &ignored));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_SAU_RLAR, &ignored));
    OK(uc_close(uc));
}

static void test_arm_v8m_region_idau(void *user, uint32_t address, int *region,
                                     bool *exempt, bool *ns, bool *nsc)
{
    if (address >= 0x4000 && address <= 0x43ff) {
        *region = 7;
    }
}

static void test_arm_v8m_tt(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x41\xe8\x00\xf0";
    uint32_t r0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_ctl_arm_security(uc, 1));
    OK(uc_ctl_arm_idau(uc, test_arm_v8m_region_idau, NULL));
    test_arm_v8m_sau_region(uc, 2, 0x4000, 0x43e0 | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, 5);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x4000);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    r0 = test_arm_v8m_get(uc, UC_ARM_REG_R0);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK((r0 & V8M_TT_SRVALID) != 0);
    TEST_CHECK(((r0 >> 8) & 0xff) == 2);
    TEST_CHECK((r0 & V8M_TT_S) == 0);
    TEST_CHECK((r0 & V8M_TT_IRVALID) != 0);
    TEST_CHECK((r0 >> 24) == 7);
    TEST_MSG("TT 0x%08x", r0);
    OK(uc_close(uc));
}

typedef struct _V8M_SAU_HOOK {
    int calls;
    int at;
    bool stop;
} V8M_SAU_HOOK;

static void test_arm_v8m_sau_store(uc_engine *uc, uc_mem_type type,
                                   uint64_t address, int size, int64_t value,
                                   void *user_data)
{
    V8M_SAU_HOOK *hook = user_data;

    hook->calls++;
    if (hook->calls == hook->at) {
        test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RNR, 7);
        test_arm_v8m_reg(uc, UC_ARM_REG_SAU_RLAR, 0);
        if (hook->stop) {
            OK(uc_emu_stop(uc));
        }
    }
}

static uc_engine *test_arm_v8m_loop_open(const char *code, uint64_t size,
                                         V8M_RUN *run, V8M_SAU_HOOK *hook)
{
    uc_hook handle;
    uc_engine *uc = test_arm_v8m_ns_open(code, size, run);

    test_arm_v8m_sau_region(uc, 0, 0x4000, 0x43e0 | V8M_SAU_ENABLE);
    OK(uc_hook_add(uc, &handle, UC_HOOK_MEM_WRITE, test_arm_v8m_sau_store, hook,
                   0x4000, 0x4003));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, 0x4000);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, V8M_LOOP_COUNT);
    return uc;
}

static void test_arm_v8m_loop_run(uc_engine *uc, V8M_RUN *run, uint64_t begin,
                                  size_t per_iteration)
{
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, begin | 1, code_start + 0x100, 0,
                    per_iteration * V8M_LOOP_COUNT + V8M_STEP_BOUND));
}

static void test_arm_v8m_sau_chained_tb(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x10\x60\x01\x3b\xfc\xd1\x00\xbf";
    V8M_SAU_HOOK hook = {0, 10, false};

    uc = test_arm_v8m_loop_open(code, sizeof(code) - 1, &run, &hook);
    test_arm_v8m_loop_run(uc, &run, code_start, 3);
    TEST_CHECK(hook.calls == V8M_LOOP_COUNT);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == code_start + 6);
    OK(uc_close(uc));

    hook.calls = 0;
    hook.stop = true;
    uc = test_arm_v8m_loop_open(code, sizeof(code) - 1, &run, &hook);
    test_arm_v8m_loop_run(uc, &run, code_start, 3);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(hook.calls == 10);
    test_arm_v8m_loop_run(uc, &run, test_arm_v8m_get(uc, UC_ARM_REG_PC), 3);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == code_start);
    TEST_CHECK(hook.calls == 10);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == V8M_LOOP_COUNT - 9);
    OK(uc_close(uc));
}

static void test_arm_v8m_sau_isb(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x10\x60\xbf\xf3\x4f\x8f\xbf\xf3\x6f\x8f\x01\x3b\xf8\xd1"
                  "\x00\xbf";
    V8M_SAU_HOOK hook = {0, 10, false};

    uc = test_arm_v8m_loop_open(code, sizeof(code) - 1, &run, &hook);
    test_arm_v8m_loop_run(uc, &run, code_start, 5);
    TEST_CHECK(hook.calls == V8M_LOOP_COUNT);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == code_start + 14);
    TEST_MSG("calls %d r3 %u pc 0x%x", hook.calls,
             test_arm_v8m_get(uc, UC_ARM_REG_R3), run.pc);
    OK(uc_close(uc));
}

static void test_arm_v8m_sau_subpage(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x08\x68\x08\x68\x08\x68\x08\x68";
    uint64_t count = 0;

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    test_arm_v8m_sau_region(uc, 0, 0x4000, 0x4000 | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x4010);
    OK(uc_ctl_slow_path_count_reset(uc));
    test_arm_v8m_run(uc, &run, code_start, code_start + 8);
    OK(uc_ctl_slow_path_count(uc, &count));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(count >= 4);
    TEST_MSG("slow path count %" PRIu64, count);

    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x4100);
    test_arm_v8m_run(uc, &run, code_start, code_start + 2);
    TEST_CHECK(run.intno == V8M_EXCP_DATA_ABORT);
    OK(uc_close(uc));

    uc = test_arm_v8m_ns_open(code, 2, &run);
    test_arm_v8m_sau_region(uc, 0, 0x4000, 0x43e0 | V8M_SAU_ENABLE);
    test_arm_v8m_sau_region(uc, 1, 0x4000, 0x4000 | V8M_SAU_ENABLE);
    test_arm_v8m_load(uc, &run, 0x4100);
    TEST_CHECK(run.intno == -1);
    test_arm_v8m_load(uc, &run, 0x4010);
    TEST_CHECK(run.intno == V8M_EXCP_DATA_ABORT);
    OK(uc_close(uc));
}

#define V8M_EXCP_NOCP 17
#define V8M_CONTROL_FPCA (1u << 2)
#define V8M_CONTROL_SFPA (1u << 3)
#define V8M_CPACR_CP10_CP11 0x00f00000u
#define V8M_FPCCR_S_RESET 0xc0000004u
#define V8M_FPCCR_S_NO_LSPEN 0x80000004u
#define V8M_FPCCR_S_NO_ASPEN 0x00000004u
#define V8M_FRAME 0x4000
#define V8M_UNTOUCHED 0xa5a5a5a5u

static uint32_t test_arm_v8m_word(uc_engine *uc, uint64_t address)
{
    uint32_t value = 0;

    OK(uc_mem_read(uc, address, &value, sizeof(value)));
    return value;
}

static uc_engine *test_arm_v8m_fp_open(V8M_RUN *run, uint32_t cpacr_s)
{
    char code[] = "\x00\xee\x10\x2a\xe1\xee\x10\x3a\x20\xec\x00\x0a";
    char vlldm[] = "\x30\xec\x00\x0a";
    uint32_t fill = V8M_UNTOUCHED;
    uc_engine *uc = test_arm_v8m_open(code, sizeof(code) - 1, run);
    uint64_t at;

    OK(uc_mem_write(uc, code_start + 0x10, vlldm, sizeof(vlldm) - 1));
    for (at = V8M_FRAME; at < V8M_FRAME + 0x88; at += 4) {
        OK(uc_mem_write(uc, at, &fill, sizeof(fill)));
    }
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, V8M_FRAME);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, 0x3f800000);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, 0x00c00000);
    test_arm_v8m_reg(uc, UC_ARM_REG_CPACR_S, cpacr_s);
    return uc;
}

static void test_arm_v8m_fp_context(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x00\xbf";
    uint32_t ignored = 0;
    uint32_t saved;

    uc = test_arm_v8m_fp_open(&run, V8M_CPACR_CP10_CP11);
    test_arm_v8m_reg(uc, UC_ARM_REG_FPCCR_S, V8M_FPCCR_S_NO_LSPEN);
    test_arm_v8m_run(uc, &run, code_start, code_start + 12);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_word(uc, V8M_FRAME) == 0x3f800000);
    saved = test_arm_v8m_word(uc, V8M_FRAME + 0x40);
    TEST_CHECK(saved == 0x00c00000);
    TEST_MSG("frame FPSCR 0x%08x", saved);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FPCCR_S) ==
               V8M_FPCCR_S_NO_LSPEN);
    TEST_CHECK((test_arm_v8m_get(uc, UC_ARM_REG_CONTROL) & V8M_CONTROL_FPCA) ==
               0);
    test_arm_v8m_reg(uc, UC_ARM_REG_S0, 0);
    test_arm_v8m_run(uc, &run, code_start + 0x10, code_start + 0x14);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_S0) == 0x3f800000);
    OK(uc_close(uc));

    uc = test_arm_v8m_fp_open(&run, V8M_CPACR_CP10_CP11);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FPCCR_S) == V8M_FPCCR_S_RESET);
    test_arm_v8m_run(uc, &run, code_start, code_start + 12);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_word(uc, V8M_FRAME) == V8M_UNTOUCHED);
    TEST_CHECK(test_arm_v8m_word(uc, V8M_FRAME + 0x40) == V8M_UNTOUCHED);
    OK(uc_close(uc));

    uc = test_arm_v8m_fp_open(&run, V8M_CPACR_CP10_CP11);
    test_arm_v8m_reg(uc, UC_ARM_REG_FPCCR_S, V8M_FPCCR_S_NO_ASPEN);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK((test_arm_v8m_get(uc, UC_ARM_REG_CONTROL) &
                (V8M_CONTROL_FPCA | V8M_CONTROL_SFPA)) == 0);
    OK(uc_close(uc));

    uc = test_arm_v8m_fp_open(&run, 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_FPCCR_S, V8M_FPCCR_S_NO_LSPEN);
    test_arm_v8m_run(uc, &run, code_start, code_start + 12);
    TEST_CHECK(run.intno == V8M_EXCP_NOCP);
    TEST_CHECK(run.pc == code_start + 8);
    TEST_CHECK(test_arm_v8m_word(uc, V8M_FRAME) == V8M_UNTOUCHED);
    TEST_CHECK((test_arm_v8m_get(uc, UC_ARM_REG_CONTROL) &
                (V8M_CONTROL_FPCA | V8M_CONTROL_SFPA)) ==
               (V8M_CONTROL_FPCA | V8M_CONTROL_SFPA));

    test_arm_v8m_reg(uc, UC_ARM_REG_CPACR_S, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_CPACR_NS, 0x00500000);
    test_arm_v8m_reg(uc, UC_ARM_REG_FPCCR_S, 0x12345678);
    test_arm_v8m_reg(uc, UC_ARM_REG_FPCCR_NS, 0x87654321);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_CPACR_S) == V8M_CPACR_CP10_CP11);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_CPACR_NS) == 0x00500000);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FPCCR_S) == 0x12345678);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FPCCR_NS) == 0x87654321);
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_CPACR_S, &ignored));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_FPCCR_S, &ignored));
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_FPCCR_NS, &ignored));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_ARM, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_A15);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_CPACR_NS, &ignored));
    OK(uc_close(uc));
}

#define V8M_CONTROL_SPSEL (1u << 1)

typedef struct _V8M_SP_CASE {
    uint32_t secure;
    uint32_t spsel;
} V8M_SP_CASE;

static void test_arm_v8m_check_sps(uc_engine *uc)
{
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSP_S) == 0x1100);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PSP_S) == 0x1200);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSP_NS) == 0x1300);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PSP_NS) == 0x1400);
}

static void test_arm_v8m_banked_sp(void)
{
    static const V8M_SP_CASE cases[] = {{1, 0}, {1, 1}, {0, 0}, {0, 1}};
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x00\xbf";
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const V8M_SP_CASE *one = &cases[i];
        int control =
            one->secure ? UC_ARM_REG_CONTROL_S : UC_ARM_REG_CONTROL_NS;
        uint32_t active = one->spsel ? (one->secure ? 0x1200 : 0x1400)
                                     : (one->secure ? 0x1100 : 0x1300);

        TEST_CASE_("secure %u spsel %u", one->secure, one->spsel);
        uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
        test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, one->secure);
        test_arm_v8m_reg(uc, control, one->spsel ? V8M_CONTROL_SPSEL : 0);
        test_arm_v8m_reg(uc, UC_ARM_REG_MSP_S, 0x1100);
        test_arm_v8m_reg(uc, UC_ARM_REG_PSP_S, 0x1200);
        test_arm_v8m_reg(uc, UC_ARM_REG_MSP_NS, 0x1300);
        test_arm_v8m_reg(uc, UC_ARM_REG_PSP_NS, 0x1400);
        test_arm_v8m_check_sps(uc);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SP) == active);

        test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, !one->secure);
        test_arm_v8m_check_sps(uc);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SP) ==
                   (one->secure ? 0x1300 : 0x1100));

        test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, one->secure);
        test_arm_v8m_check_sps(uc);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SP) == active);

        test_arm_v8m_reg(uc, UC_ARM_REG_IPSR, 3);
        test_arm_v8m_check_sps(uc);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SP) ==
                   (one->secure ? 0x1100 : 0x1300));
        OK(uc_close(uc));
    }
    TEST_CASE(NULL);
}

static void test_arm_v8m_banked_special(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x00\xbf";
    uint32_t ignored = 0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    test_arm_v8m_reg(uc, UC_ARM_REG_CONTROL_S, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_CONTROL_NS, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_PRIMASK_S, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_PRIMASK_NS, 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_BASEPRI_S, 0x1f0);
    test_arm_v8m_reg(uc, UC_ARM_REG_BASEPRI_NS, 0x40);
    test_arm_v8m_reg(uc, UC_ARM_REG_FAULTMASK_S, 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_FAULTMASK_NS, 3);
    test_arm_v8m_reg(uc, UC_ARM_REG_MSPLIM_S, 0x2007);
    test_arm_v8m_reg(uc, UC_ARM_REG_MSPLIM_NS, 0x3008);
    test_arm_v8m_reg(uc, UC_ARM_REG_PSPLIM_S, 0x4010);
    test_arm_v8m_reg(uc, UC_ARM_REG_PSPLIM_NS, 0x501f);

    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_CONTROL_S) == 0xf);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_CONTROL_NS) == 0x3);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PRIMASK_S) == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PRIMASK_NS) == 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_BASEPRI_S) == 0xf0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_BASEPRI_NS) == 0x40);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FAULTMASK_S) == 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FAULTMASK_NS) == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSPLIM_S) == 0x2000);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSPLIM_NS) == 0x3008);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PSPLIM_S) == 0x4010);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PSPLIM_NS) == 0x5018);

    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PRIMASK) == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_BASEPRI) == 0xf0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSPLIM) == 0x2000);
    test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PRIMASK) == 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_BASEPRI) == 0x40);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_FAULTMASK) == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_MSPLIM) == 0x3008);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PSPLIM) == 0x5018);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PRIMASK_S) == 1);
    uc_assert_err(UC_ERR_ARG,
                  uc_reg_write(uc, UC_ARM_REG_FAULT_STATUS, &ignored));
    uc_assert_err(UC_ERR_ARG,
                  uc_reg_write(uc, UC_ARM_REG_FAULT_ADDRESS, &ignored));
    OK(uc_close(uc));

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_MSP_S, &ignored));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_PSPLIM_NS, &ignored));
    OK(uc_close(uc));
}

typedef struct _V8M_FAULT {
    int intno;
    uint32_t status;
    uint32_t address;
} V8M_FAULT;

static void test_arm_v8m_fault_intr(uc_engine *uc, int intno, void *user_data)
{
    V8M_FAULT *fault = user_data;

    fault->intno = intno;
    OK(uc_reg_read(uc, UC_ARM_REG_FAULT_STATUS, &fault->status));
    OK(uc_reg_read(uc, UC_ARM_REG_FAULT_ADDRESS, &fault->address));
    OK(uc_emu_stop(uc));
}

static void test_arm_v8m_nsc_idau(void *user, uint32_t address, int *region,
                                  bool *exempt, bool *ns, bool *nsc)
{
    if (address >= 0x3000 && address <= 0x33ff) {
        *ns = false;
        *nsc = true;
    }
}

static void test_arm_v8m_fault_status(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_FAULT fault = {-1, 0, 0};
    uc_hook hook;
    char code[] = "\x08\x68\x08\x60\x08\x47";

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_fault_intr, &fault, 1,
                   0));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x4404);
    test_arm_v8m_run(uc, &run, code_start, code_start + 2);
    TEST_CHECK(fault.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(fault.status == UC_ARM_FAULT_SECURE);
    TEST_CHECK(fault.address == 0x4404);

    fault.intno = -1;
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x4408);
    test_arm_v8m_run(uc, &run, code_start + 2, code_start + 4);
    TEST_CHECK(fault.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(fault.status == (UC_ARM_FAULT_SECURE | 1u << 11));
    TEST_CHECK(fault.address == 0x4408);

    fault.intno = -1;
    OK(uc_ctl_arm_idau(uc, test_arm_v8m_nsc_idau, NULL));
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, 2);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x3001);
    test_arm_v8m_run(uc, &run, code_start + 4, code_start + 6);
    TEST_CHECK(fault.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(fault.status == UC_ARM_FAULT_NSC_EXEC);
    TEST_CHECK(fault.address == 0x3000);
    OK(uc_close(uc));
}

#define V8M_MMIO 0x40000000
#define V8M_SAU_ALLNS 2

typedef struct _V8M_MMIO_LOG {
    int reads;
    int writes;
    uint64_t offset;
    uint64_t value;
    unsigned size;
    uint64_t write_offset;
    unsigned write_size;
    uc_mem_attrs read;
    uc_mem_attrs write;
    int secure_writes;
    int switch_at;
    bool stop;
    bool precise;
} V8M_MMIO_LOG;

static uint64_t test_arm_v8m_mmio_read(uc_engine *uc, uint64_t offset,
                                       unsigned size, uc_mem_attrs attrs,
                                       void *user_data)
{
    V8M_MMIO_LOG *log = user_data;

    log->reads++;
    log->offset = offset;
    log->size = size;
    log->read = attrs;
    return 0x5a5a0000 | offset;
}

static void test_arm_v8m_mmio_write(uc_engine *uc, uint64_t offset,
                                    unsigned size, uint64_t value,
                                    uc_mem_attrs attrs, void *user_data)
{
    V8M_MMIO_LOG *log = user_data;

    log->writes++;
    log->offset = offset;
    log->size = size;
    log->write_offset = offset;
    log->write_size = size;
    log->value = value;
    log->write = attrs;
    if (attrs.secure) {
        log->secure_writes++;
    }
    if (log->writes == log->switch_at) {
        test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 0);
        if (log->stop) {
            OK(uc_emu_stop(uc));
        }
        if (log->precise) {
            OK(uc_ctl_exit_after_insn(uc));
        }
    }
}

static uc_engine *test_arm_v8m_mmio_open(const char *code, uint64_t size,
                                         V8M_RUN *run, V8M_MMIO_LOG *log)
{
    uc_engine *uc = test_arm_v8m_open(code, size, run);

    memset(log, 0, sizeof(*log));
    OK(uc_mmio_map_attrs(uc, V8M_MMIO, 0x1000, test_arm_v8m_mmio_read, log,
                         test_arm_v8m_mmio_write, log));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x11223344);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x24);
    return uc;
}

static void test_arm_v8m_mmio_access(uc_engine *uc, V8M_RUN *run,
                                     V8M_MMIO_LOG *log)
{
    log->reads = 0;
    log->writes = 0;
    log->read = (uc_mem_attrs){!log->read.secure, !log->read.privileged};
    log->write = (uc_mem_attrs){!log->write.secure, !log->write.privileged};
    test_arm_v8m_run(uc, run, code_start, code_start + 4);
    TEST_CHECK(run->intno == -1);
    TEST_CHECK(log->writes == 1);
    TEST_CHECK(log->reads == 1);
}

static void test_arm_v8m_mmio_attrs(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_MMIO_LOG log;
    char code[] = "\x11\x60\x10\x68";
    uint32_t word = 0;

    uc = test_arm_v8m_mmio_open(code, sizeof(code) - 1, &run, &log);
    test_arm_v8m_mmio_access(uc, &run, &log);
    TEST_CHECK(log.write.secure && log.write.privileged);
    TEST_CHECK(log.read.secure && log.read.privileged);
    TEST_CHECK(log.value == 0x11223344);
    TEST_CHECK(log.write_offset == 0x24);
    TEST_CHECK(log.write_size == 4);
    TEST_CHECK(log.offset == 0x24);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x5a5a0024);

    test_arm_v8m_reg(uc, UC_ARM_REG_CONTROL, 1);
    test_arm_v8m_mmio_access(uc, &run, &log);
    TEST_CHECK(log.write.secure && !log.write.privileged);
    TEST_CHECK(log.read.secure && !log.read.privileged);

    OK(uc_mem_read(uc, V8M_MMIO + 0x30, &word, sizeof(word)));
    TEST_CHECK(word == 0x5a5a0030);
    TEST_CHECK(!log.read.secure && log.read.privileged);
    OK(uc_mem_write(uc, V8M_MMIO + 0x34, &word, sizeof(word)));
    TEST_CHECK(!log.write.secure && log.write.privileged);

    OK(uc_mem_read_attrs(uc, V8M_MMIO + 0x38, &word, sizeof(word),
                         (uc_mem_attrs){true, false}));
    TEST_CHECK(word == 0x5a5a0038);
    TEST_CHECK(log.read.secure && !log.read.privileged);
    OK(uc_mem_write_attrs(uc, V8M_MMIO + 0x3c, &word, sizeof(word),
                          (uc_mem_attrs){true, false}));
    TEST_CHECK(log.write.secure && !log.write.privileged);
    TEST_CHECK(log.write_offset == 0x3c && log.value == 0x5a5a0038);
    OK(uc_close(uc));

    uc = test_arm_v8m_mmio_open(code, sizeof(code) - 1, &run, &log);
    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ALLNS);
    test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 0);
    test_arm_v8m_mmio_access(uc, &run, &log);
    TEST_CHECK(!log.write.secure && log.write.privileged);
    TEST_CHECK(!log.read.secure && log.read.privileged);
    OK(uc_close(uc));
}

static void test_arm_v8m_mmio_attrs_sau(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_MMIO_LOG log;
    char code[] = "\x11\x60\x10\x68";

    uc = test_arm_v8m_mmio_open(code, sizeof(code) - 1, &run, &log);
    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_mmio_access(uc, &run, &log);
    TEST_CHECK(log.write.secure && log.read.secure);

    test_arm_v8m_sau_region(uc, 0, V8M_MMIO,
                            (V8M_MMIO + 0x3e0) | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    test_arm_v8m_mmio_access(uc, &run, &log);
    TEST_CHECK(!log.write.secure && !log.read.secure);
    TEST_CHECK(log.write.privileged && log.read.privileged);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SECURE) == 1);
    OK(uc_close(uc));
}

static void test_arm_v8m_exempt_idau(void *user, uint32_t address, int *region,
                                     bool *exempt, bool *ns, bool *nsc)
{
    if (address < code_start + 0x400 ||
        (address >= V8M_MMIO && address < V8M_MMIO + 0x400)) {
        *exempt = true;
    }
}

static uc_engine *test_arm_v8m_mmio_loop_open(const char *code, uint64_t size,
                                              V8M_RUN *run, V8M_MMIO_LOG *log,
                                              bool stop, bool precise)
{
    uc_engine *uc = test_arm_v8m_mmio_open(code, size, run, log);

    OK(uc_ctl_arm_security(uc, 1));
    OK(uc_ctl_arm_idau(uc, test_arm_v8m_exempt_idau, NULL));
    if (precise) {
        OK(uc_ctl_exit_after_insn_enable(uc));
    }
    log->switch_at = 10;
    log->stop = stop;
    log->precise = precise;
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, V8M_LOOP_COUNT);
    return uc;
}

static void test_arm_v8m_mmio_attrs_chained(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_MMIO_LOG log;
    char code[] = "\x10\x60\x01\x3b\xfc\xd1\x00\xbf";

    uc = test_arm_v8m_mmio_loop_open(code, sizeof(code) - 1, &run, &log, false,
                                     false);
    test_arm_v8m_loop_run(uc, &run, code_start, 3);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.writes == V8M_LOOP_COUNT);
    TEST_CHECK(log.secure_writes == V8M_LOOP_COUNT);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SECURE) == 0);
    TEST_MSG("writes %d secure %d", log.writes, log.secure_writes);
    OK(uc_close(uc));

    uc = test_arm_v8m_mmio_loop_open(code, sizeof(code) - 1, &run, &log, true,
                                     false);
    test_arm_v8m_loop_run(uc, &run, code_start, 3);
    TEST_CHECK(log.writes == 10);
    test_arm_v8m_loop_run(uc, &run, test_arm_v8m_get(uc, UC_ARM_REG_PC), 3);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.writes == V8M_LOOP_COUNT + 1);
    TEST_CHECK(log.secure_writes == 10);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_MSG("uc_emu_stop repeats the store: writes %d secure %d", log.writes,
             log.secure_writes);
    OK(uc_close(uc));

    uc = test_arm_v8m_mmio_loop_open(code, sizeof(code) - 1, &run, &log, false,
                                     true);
    test_arm_v8m_loop_run(uc, &run, code_start, 3);
    TEST_CHECK(log.writes == 10);
    test_arm_v8m_loop_run(uc, &run, test_arm_v8m_get(uc, UC_ARM_REG_PC), 3);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.writes == V8M_LOOP_COUNT);
    TEST_CHECK(log.secure_writes == 10);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_MSG("writes %d secure %d", log.writes, log.secure_writes);
    OK(uc_close(uc));
}

static void test_arm_mmio_attrs_no_security(void)
{
    uc_cpu_arm models[] = {UC_CPU_ARM_CORTEX_M0, UC_CPU_ARM_CORTEX_M3,
                           UC_CPU_ARM_CORTEX_M4, UC_CPU_ARM_CORTEX_M7};
    char code[] = "\x11\x60\x10\x68";
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    V8M_MMIO_LOG log;
    size_t i;

    for (i = 0; i < sizeof(models) / sizeof(models[0]); i++) {
        uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                        models[i]);
        OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_intr, &run, 1, 0));
        memset(&log, 0, sizeof(log));
        OK(uc_mmio_map_attrs(uc, V8M_MMIO, 0x1000, test_arm_v8m_mmio_read, &log,
                             test_arm_v8m_mmio_write, &log));
        test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x24);
        test_arm_v8m_mmio_access(uc, &run, &log);
        TEST_CHECK(!log.write.secure && !log.read.secure);
        TEST_CHECK(log.write.privileged && log.read.privileged);
        TEST_MSG("model %d", (int)models[i]);
        OK(uc_close(uc));
    }
}

typedef struct _V8M_PLAIN_LOG {
    int reads;
    int writes;
    uint64_t offset;
    uint64_t value;
    unsigned size;
    uint64_t write_offset;
    unsigned write_size;
} V8M_PLAIN_LOG;

static uint64_t test_arm_mmio_plain_read(uc_engine *uc, uint64_t offset,
                                         unsigned size, void *user_data)
{
    V8M_PLAIN_LOG *log = user_data;

    log->reads++;
    log->offset = offset;
    log->size = size;
    return 0x5a5a0000 | offset;
}

static void test_arm_mmio_plain_write(uc_engine *uc, uint64_t offset,
                                      unsigned size, uint64_t value,
                                      void *user_data)
{
    V8M_PLAIN_LOG *log = user_data;

    log->writes++;
    log->offset = offset;
    log->size = size;
    log->write_offset = offset;
    log->write_size = size;
    log->value = value;
}

static void test_arm_mmio_plain_unchanged(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_PLAIN_LOG log = {0};
    char code[] = "\x11\x80\x10\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mmio_map(uc, V8M_MMIO, 0x1000, test_arm_mmio_plain_read, &log,
                   test_arm_mmio_plain_write, &log));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x11223344);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x24);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.writes == 1);
    TEST_CHECK(log.reads == 1);
    TEST_CHECK(log.value == 0x3344);
    TEST_CHECK(log.write_offset == 0x24);
    TEST_CHECK(log.write_size == 2);
    TEST_CHECK(log.offset == 0x24);
    TEST_CHECK(log.size == 4);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x5a5a0024);
    OK(uc_close(uc));
}

static void test_arm_mmio_attrs_split(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_MMIO_LOG log;
    char code[] = "\x11\x60\x10\x68";
    uint32_t base;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    memset(&log, 0, sizeof(log));
    OK(uc_mmio_map_attrs(uc, V8M_MMIO, 0x3000, test_arm_v8m_mmio_read, &log,
                         test_arm_v8m_mmio_write, &log));
    OK(uc_mem_protect(uc, V8M_MMIO + 0x1000, 0x1000, UC_PROT_READ));
    for (base = V8M_MMIO; base < V8M_MMIO + 0x3000; base += 0x2000) {
        test_arm_v8m_reg(uc, UC_ARM_REG_R2, base + 0x10);
        test_arm_v8m_mmio_access(uc, &run, &log);
        TEST_CHECK(log.write_offset == 0x10);
        TEST_CHECK(log.offset == 0x10);
        TEST_CHECK(log.write.secure && log.read.secure);
    }

    log.reads = 0;
    log.writes = 0;
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x1010);
    test_arm_v8m_run(uc, &run, code_start + 2, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.reads == 1);
    TEST_CHECK(log.offset == 0x10);
    TEST_CHECK(log.read.secure && log.read.privileged);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x5a5a0010);
    run.intno = -1;
    TEST_CHECK(uc_emu_start(uc, code_start | 1, code_start + 2, 0,
                            V8M_STEP_BOUND) != UC_ERR_OK);
    TEST_CHECK(log.writes == 0);
    OK(uc_close(uc));
}

#define V8M_REMOVE_COUNT 100000
#define V8M_REMOVE_AT 1000
#define V8M_PTR_BASE 0x8000

typedef struct _V8M_REMOVE {
    int calls;
    uint64_t begin;
} V8M_REMOVE;

static void test_arm_v8m_remove_store(uc_engine *uc, uc_mem_type type,
                                      uint64_t address, int size,
                                      int64_t value, void *user_data)
{
    V8M_REMOVE *remove = user_data;

    if (++remove->calls == V8M_REMOVE_AT) {
        OK(uc_ctl_remove_cache(uc, remove->begin, remove->begin + 0x400));
        OK(uc_emu_stop(uc));
    }
}

static void test_arm_v8m_remove_cache_secure_fault(void)
{
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    V8M_REMOVE remove = {0, 0x4000};
    char code[] = "\x10\x60\x01\x3b\xfc\xd1\x00\xbf";

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, test_arm_v8m_remove_store,
                   &remove, code_start + 0x200, code_start + 0x203));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, code_start + 0x200);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, V8M_REMOVE_COUNT);
    run.intno = -1;
    OK(uc_emu_start(uc, code_start | 1, code_start + 6, 0,
                    3 * V8M_REMOVE_COUNT + V8M_STEP_BOUND));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(remove.calls == V8M_REMOVE_AT);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) ==
               V8M_REMOVE_COUNT - (V8M_REMOVE_AT - 1));
    OK(uc_emu_start(uc, test_arm_v8m_get(uc, UC_ARM_REG_PC) | 1,
                    code_start + 6, 0, 3 * V8M_REMOVE_COUNT + V8M_STEP_BOUND));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_MSG("intno %d pc 0x%x r3 %u", run.intno, run.pc,
             test_arm_v8m_get(uc, UC_ARM_REG_R3));
    OK(uc_close(uc));
}

static uc_engine *test_arm_v8m_ptr_open(V8M_RUN *run, uint8_t *buffer)
{
    char code[] = "\x00\xbf\x00\xbf";
    char target[] = "\x01\x20\x08\x47";
    uc_engine *uc = test_arm_v8m_open(code, sizeof(code) - 1, run);

    memset(buffer, 0, 0x400);
    memcpy(buffer, target, sizeof(target) - 1);
    OK(uc_mem_map_ptr(uc, V8M_PTR_BASE, 0x400, UC_PROT_ALL, buffer));
    OK(uc_ctl_arm_security(uc, 1));
    return uc;
}

static uint32_t test_arm_v8m_ptr_run(uc_engine *uc, V8M_RUN *run)
{
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, code_start | 1);
    test_arm_v8m_run(uc, run, V8M_PTR_BASE, code_start + 2);
    TEST_CHECK(run->intno == -1);
    return test_arm_v8m_get(uc, UC_ARM_REG_R0);
}

static uint32_t test_arm_v8m_unfetchable_leg(bool remove)
{
    uc_engine *uc;
    V8M_RUN run;
    uint32_t r0;
    static uint8_t buffer[0x400];

    uc = test_arm_v8m_ptr_open(&run, buffer);
    TEST_CHECK(test_arm_v8m_ptr_run(uc, &run) == 1);
    buffer[0] = 2;
    test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 0);
    if (remove) {
        OK(uc_ctl_remove_cache(uc, V8M_PTR_BASE, V8M_PTR_BASE + 0x400));
    }
    test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, 1);
    r0 = test_arm_v8m_ptr_run(uc, &run);
    OK(uc_close(uc));
    return r0;
}

static void test_arm_v8m_remove_cache_unfetchable(void)
{
    uint32_t removed = test_arm_v8m_unfetchable_leg(true);
    uint32_t kept = test_arm_v8m_unfetchable_leg(false);

    TEST_CHECK(removed == 2);
    TEST_CHECK(kept == 1);
    TEST_MSG("removed r0 %u, without removal r0 %u", removed, kept);
}

static uint32_t test_arm_v8m_subpage_leg(bool remove)
{
    uc_engine *uc;
    V8M_RUN run;
    uint32_t r0;
    static uint8_t buffer[0x400];

    uc = test_arm_v8m_ptr_open(&run, buffer);
    test_arm_v8m_sau_region(uc, 0, V8M_PTR_BASE + 0x100,
                            (V8M_PTR_BASE + 0x100) | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    TEST_CHECK(test_arm_v8m_ptr_run(uc, &run) == 1);
    buffer[0] = 2;
    if (remove) {
        OK(uc_ctl_remove_cache(uc, V8M_PTR_BASE, V8M_PTR_BASE + 0x400));
    }
    r0 = test_arm_v8m_ptr_run(uc, &run);
    OK(uc_close(uc));
    return r0;
}

static void test_arm_v8m_remove_cache_subpage(void)
{
    uint32_t removed = test_arm_v8m_subpage_leg(true);
    uint32_t kept = test_arm_v8m_subpage_leg(false);

    TEST_CHECK(removed == 2);
    TEST_CHECK(kept == 2);
    TEST_MSG("removed r0 %u, without removal r0 %u", removed, kept);
}

#define BUS_PAGE 0x4000
#define BUS_WORD 0x11223344u
#define BUS_DATA 0x8000
#define BUS_FAULT_EXTERNAL (0x8u | 1u << 12)
#define BUS_FAULT_WNR (1u << 11)
#define BUS_CHAIN_COUNT 100000
#define BUS_CHAIN_FLIP 1000

typedef struct _BUS_LOG {
    uint64_t page;
    uint64_t alias;
    unsigned secure_verdict;
    unsigned ns_verdict;
    int access_verdict;
    uint64_t raz_value;
    int page_fills;
    uc_mem_type first_type;
    uint64_t first_address;
    uc_mem_attrs page_attrs;
    int accesses;
    int secure_accesses;
    int writes;
    uint64_t stored;
    uint64_t store_address;
    uint64_t address;
    unsigned size;
    uc_mem_type type;
    uc_mem_attrs attrs;
} BUS_LOG;

static unsigned test_arm_bus_page(uc_engine *uc, uint64_t page,
                                  uint64_t address, uc_mem_attrs attrs,
                                  uc_mem_type access_type, void *user_data)
{
    BUS_LOG *log = user_data;

    if (page != log->page && page != log->alias) {
        return 0;
    }
    if (log->page_fills++ == 0) {
        log->first_type = access_type;
        log->first_address = address;
    }
    log->page_attrs = attrs;
    return attrs.secure ? log->secure_verdict : log->ns_verdict;
}

static int test_arm_bus_access(uc_engine *uc, uint64_t address, unsigned size,
                               uc_mem_attrs attrs, uc_mem_type access_type,
                               uint64_t *value, void *user_data)
{
    BUS_LOG *log = user_data;

    log->accesses++;
    if (attrs.secure) {
        log->secure_accesses++;
    }
    log->address = address;
    log->size = size;
    log->type = access_type;
    log->attrs = attrs;
    if (access_type == UC_MEM_WRITE) {
        log->writes++;
        log->stored = *value;
        log->store_address = address;
    } else {
        TEST_CHECK(*value == 0);
        *value = log->raz_value;
    }
    return log->access_verdict;
}

static void test_arm_bus_log(BUS_LOG *log, uint64_t page, unsigned verdict,
                             int access_verdict)
{
    memset(log, 0, sizeof(*log));
    log->page = page;
    log->alias = page;
    log->secure_verdict = verdict;
    log->ns_verdict = verdict;
    log->access_verdict = access_verdict;
}

static void test_arm_bus_install(uc_engine *uc, BUS_LOG *log)
{
    OK(uc_ctl_bus_filter(uc, test_arm_bus_page, test_arm_bus_access, log));
}

static void test_arm_bus_put(uc_engine *uc, uint64_t address, uint32_t value)
{
    OK(uc_mem_write(uc, address, &value, sizeof(value)));
}

static void test_arm_bus_filter_fetch_first(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68";

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    test_arm_bus_put(uc, code_start + 0x200, BUS_WORD);
    test_arm_bus_log(&log, code_start, 0, UC_BUS_RAZ_WI);
    log.ns_verdict = UC_BUS_FILTER_DATA;
    test_arm_bus_install(uc, &log);
    test_arm_v8m_load(uc, &run, code_start + 0x200);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(log.first_type == UC_MEM_FETCH);
    TEST_CHECK(log.first_address == code_start);
    TEST_CHECK(log.page_fills == 1);
    TEST_CHECK(log.accesses == 1);
    TEST_CHECK(!log.page_attrs.secure && !log.attrs.secure);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0);
    TEST_MSG("fills %d accesses %d r0 0x%x", log.page_fills, log.accesses,
             test_arm_v8m_get(uc, UC_ARM_REG_R0));
    OK(uc_close(uc));
}

static uint64_t test_arm_bus_fast_loop(BUS_LOG *log)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x10\x60\x11\x68\x01\x3b\xfb\xd1";
    uint64_t slow = 0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mem_map(uc, BUS_DATA, 0x400, UC_PROT_READ | UC_PROT_WRITE));
    if (log) {
        test_arm_bus_install(uc, log);
    }
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_DATA);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, V8M_LOOP_COUNT);
    OK(uc_ctl_slow_path_count_reset(uc));
    test_arm_v8m_loop_run(uc, &run, code_start, 4);
    OK(uc_ctl_slow_path_count(uc, &slow));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0x55aa);
    TEST_CHECK(test_arm_v8m_word(uc, BUS_DATA) == 0x55aa);
    OK(uc_close(uc));
    return slow;
}

static void test_arm_bus_filter_fast(void)
{
    BUS_LOG log;
    uint64_t plain = test_arm_bus_fast_loop(NULL);
    uint64_t slow;

    test_arm_bus_log(&log, BUS_DATA, 0, UC_BUS_RAZ_WI);
    slow = test_arm_bus_fast_loop(&log);
    TEST_CHECK(log.page_fills > 0);
    TEST_CHECK(log.accesses == 0);
    TEST_CHECK(slow == plain);
    TEST_CHECK(slow <= 2);
    TEST_MSG("fills %d accesses %d slow %" PRIu64 " plain %" PRIu64,
             log.page_fills, log.accesses, slow, plain);
}

static void test_arm_bus_exempt_idau(void *user, uint32_t address, int *region,
                                     bool *exempt, bool *ns, bool *nsc)
{
    if (address < code_start + 0x400 ||
        (address >= BUS_PAGE && address < BUS_PAGE + 0x400)) {
        *exempt = true;
    }
}

static void test_arm_bus_filter_per_entry(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68";
    int i;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_ctl_arm_security(uc, 1));
    OK(uc_ctl_arm_idau(uc, test_arm_bus_exempt_idau, NULL));
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, 0, UC_BUS_RAZ_WI);
    log.ns_verdict = UC_BUS_FILTER_DATA;
    test_arm_bus_install(uc, &log);
    for (i = 0; i < 4; i++) {
        uint32_t secure = i & 1;
        int accesses = log.accesses;

        test_arm_v8m_reg(uc, UC_ARM_REG_SECURE, secure);
        test_arm_v8m_load(uc, &run, BUS_PAGE);
        TEST_CHECK(run.intno == -1);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) ==
                   (secure ? BUS_WORD : 0));
        TEST_CHECK(log.accesses == accesses + (secure ? 0 : 1));
        TEST_MSG("pass %d secure %u", i, secure);
    }
    TEST_CHECK(log.secure_accesses == 0);
    TEST_CHECK(log.page_fills == 2);
    TEST_MSG("fills %d", log.page_fills);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_sau_downgrade(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_ctl_arm_security(uc, 1));
    test_arm_v8m_sau_region(uc, 0, BUS_PAGE,
                            (BUS_PAGE + 0x3e0) | V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    test_arm_bus_put(uc, BUS_PAGE + 0x100, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA, UC_BUS_ALLOW);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_load(uc, &run, BUS_PAGE + 0x100);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_SECURE) == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(log.page_fills == 1);
    TEST_CHECK(log.first_address == BUS_PAGE + 0x100);
    TEST_CHECK(!log.page_attrs.secure && log.page_attrs.privileged);
    TEST_CHECK(log.accesses == 1);
    TEST_CHECK(!log.attrs.secure && log.attrs.privileged);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_raz_wi(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x10\x60\x11\x68";
    uint64_t slow = 0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA, UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    OK(uc_ctl_slow_path_count_reset(uc));
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    OK(uc_ctl_slow_path_count(uc, &slow));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_word(uc, BUS_PAGE) == BUS_WORD);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0);
    TEST_CHECK(log.accesses == 2);
    TEST_CHECK(log.writes == 1);
    TEST_CHECK(log.stored == 0x55aa);
    TEST_CHECK(log.store_address == BUS_PAGE);
    TEST_CHECK(log.type == UC_MEM_READ);
    TEST_CHECK(log.address == BUS_PAGE && log.size == 4);
    TEST_CHECK(log.attrs.secure && log.attrs.privileged);
    TEST_CHECK(slow == 2);

    log.raz_value = 0xa5;
    test_arm_v8m_run(uc, &run, code_start + 2, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0xa5);
    TEST_CHECK(log.accesses == 3);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_fault(void)
{
    uc_engine *uc;
    V8M_RUN run;
    V8M_FAULT fault = {-1, 0, 0};
    BUS_LOG log;
    uc_hook hook;
    char code[] = "\x10\x60\x11\x68\x18\x47";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_fault_intr, &fault, 1,
                   0));
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA | UC_BUS_DENY_FETCH,
                     UC_BUS_FAULT);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, BUS_PAGE | 1);

    test_arm_v8m_run(uc, &run, code_start, code_start + 2);
    TEST_CHECK(fault.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(fault.status == (BUS_FAULT_EXTERNAL | BUS_FAULT_WNR));
    TEST_CHECK(fault.address == BUS_PAGE);
    TEST_CHECK(test_arm_v8m_word(uc, BUS_PAGE) == BUS_WORD);
    TEST_MSG("store status 0x%x address 0x%x", fault.status, fault.address);

    fault.intno = -1;
    test_arm_v8m_run(uc, &run, code_start + 2, code_start + 4);
    TEST_CHECK(fault.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(fault.status == BUS_FAULT_EXTERNAL);
    TEST_CHECK(fault.address == BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0xffffffff);
    TEST_CHECK(log.accesses == 2);

    fault.intno = -1;
    test_arm_v8m_run(uc, &run, code_start + 4, BUS_PAGE + 2);
    TEST_CHECK(fault.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(fault.status == BUS_FAULT_EXTERNAL);
    TEST_CHECK(fault.address == BUS_PAGE);
    TEST_CHECK(log.accesses == 2);
    TEST_MSG("fetch status 0x%x address 0x%x", fault.status, fault.address);
    OK(uc_close(uc));
}

typedef struct _REFUSE_LOG {
    int reads;
    int writes;
    bool refuse_reads;
    bool refuse_writes;
    int intno;
    uint32_t status;
    uint32_t address;
    uint32_t pc;
    uint32_t xpsr;
} REFUSE_LOG;

static uint64_t test_arm_refuse_read(uc_engine *uc, uint64_t offset,
                                     unsigned size, uc_mem_attrs attrs,
                                     void *user_data)
{
    REFUSE_LOG *log = user_data;

    log->reads++;
    if (log->refuse_reads) {
        OK(uc_ctl_refuse_mmio(uc));
    }
    return 0x5a5a0000 | offset;
}

static void test_arm_refuse_write(uc_engine *uc, uint64_t offset,
                                  unsigned size, uint64_t value,
                                  uc_mem_attrs attrs, void *user_data)
{
    REFUSE_LOG *log = user_data;

    log->writes++;
    if (log->refuse_writes) {
        OK(uc_ctl_refuse_mmio(uc));
    }
}

static void test_arm_refuse_intr(uc_engine *uc, int intno, void *user_data)
{
    REFUSE_LOG *log = user_data;

    log->intno = intno;
    OK(uc_reg_read(uc, UC_ARM_REG_FAULT_STATUS, &log->status));
    OK(uc_reg_read(uc, UC_ARM_REG_FAULT_ADDRESS, &log->address));
    OK(uc_reg_read(uc, UC_ARM_REG_PC, &log->pc));
    OK(uc_reg_read(uc, UC_ARM_REG_XPSR, &log->xpsr));
    OK(uc_emu_stop(uc));
}

static void test_arm_refuse_run(uc_engine *uc, REFUSE_LOG *log, uint64_t begin,
                                uint64_t until)
{
    log->reads = 0;
    log->writes = 0;
    log->intno = -1;
    log->status = 0;
    log->address = 0;
    log->pc = 0;
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0xdeadbeef);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, 0);
    OK(uc_emu_start(uc, begin | 1, until, 0, V8M_STEP_BOUND));
}

static void test_arm_mmio_refuse(void)
{
    uc_engine *uc;
    REFUSE_LOG log;
    uc_hook hook;
    uint32_t word = 0;
    char code[] = "\x01\x23\x11\x60\x02\x23\x10\x68\x03\x23";

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_M33);
    memset(&log, 0, sizeof(log));
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_refuse_intr, &log, 1, 0));
    OK(uc_mmio_map_attrs(uc, V8M_MMIO, 0x1000, test_arm_refuse_read, &log,
                         test_arm_refuse_write, &log));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x11223344);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x24);

    test_arm_refuse_run(uc, &log, code_start, code_start + 10);
    TEST_CHECK(log.intno == -1);
    TEST_CHECK(log.writes == 1 && log.reads == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x5a5a0024);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 3);

    log.refuse_writes = true;
    test_arm_refuse_run(uc, &log, code_start, code_start + 10);
    TEST_CHECK(log.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(log.status == (BUS_FAULT_EXTERNAL | BUS_FAULT_WNR));
    TEST_CHECK(log.address == V8M_MMIO + 0x24);
    TEST_CHECK(log.pc == code_start + 2);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 1);
    TEST_CHECK(log.reads == 0);
    TEST_MSG("store status 0x%x address 0x%x pc 0x%x", log.status, log.address,
             log.pc);

    log.refuse_writes = false;
    log.refuse_reads = true;
    test_arm_refuse_run(uc, &log, code_start + 4, code_start + 10);
    TEST_CHECK(log.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(log.status == BUS_FAULT_EXTERNAL);
    TEST_CHECK(log.address == V8M_MMIO + 0x24);
    TEST_CHECK(log.pc == code_start + 6);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0xdeadbeef);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 2);
    TEST_MSG("load status 0x%x address 0x%x pc 0x%x", log.status, log.address,
             log.pc);

    uc_assert_err(UC_ERR_READ_UNMAPPED,
                  uc_mem_read_attrs(uc, V8M_MMIO + 0x30, &word, sizeof(word),
                                    (uc_mem_attrs){true, true}));
    log.refuse_reads = false;
    log.refuse_writes = true;
    uc_assert_err(UC_ERR_WRITE_UNMAPPED,
                  uc_mem_write_attrs(uc, V8M_MMIO + 0x30, &word, sizeof(word),
                                     (uc_mem_attrs){true, true}));
    log.refuse_writes = false;
    OK(uc_mem_read_attrs(uc, V8M_MMIO + 0x30, &word, sizeof(word),
                         (uc_mem_attrs){true, true}));
    TEST_CHECK(word == 0x5a5a0030);

    test_arm_refuse_run(uc, &log, code_start, code_start + 10);
    TEST_CHECK(log.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 3);
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl(uc, UC_CTL_READ(UC_CTL_UC_REFUSE_MMIO, 0)));
    OK(uc_close(uc));
}

#define V8M_XPSR_IT (0x3fu << 10 | 3u << 25)
#define V8M_IT_EQ_TWO_LEFT (0x04u << 8)

static void test_arm_mmio_refuse_it_block(void)
{
    uc_engine *uc;
    REFUSE_LOG log;
    uc_hook hook;
    char code[] = "\x01\x23\x9b\x42\x02\xbf\x04\x23\x11\x60\x05\x23\x06\x23"
                  "\x9b\x42\x04\xbf\x10\x68\x07\x23\x08\x23";

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS, code,
                    sizeof(code) - 1, UC_CPU_ARM_CORTEX_M33);
    memset(&log, 0, sizeof(log));
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_refuse_intr, &log, 1, 0));
    OK(uc_mmio_map_attrs(uc, V8M_MMIO, 0x1000, test_arm_refuse_read, &log,
                         test_arm_refuse_write, &log));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0x11223344);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, V8M_MMIO + 0x24);

    log.refuse_writes = true;
    test_arm_refuse_run(uc, &log, code_start, code_start + 14);
    TEST_CHECK(log.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(log.pc == code_start + 8);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 4);
    TEST_CHECK((log.xpsr & V8M_XPSR_IT) == V8M_IT_EQ_TWO_LEFT);
    TEST_MSG("store xpsr 0x%x pc 0x%x", log.xpsr, log.pc);

    log.refuse_writes = false;
    log.refuse_reads = true;
    test_arm_refuse_run(uc, &log, code_start + 14, code_start + 24);
    TEST_CHECK(log.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(log.pc == code_start + 18);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0xdeadbeef);
    TEST_CHECK((log.xpsr & V8M_XPSR_IT) == V8M_IT_EQ_TWO_LEFT);
    TEST_MSG("load xpsr 0x%x pc 0x%x", log.xpsr, log.pc);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_exec_after_data(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68\x10\x47";
    char target[] = "\x5a\x20";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mem_write(uc, BUS_PAGE, target, sizeof(target) - 1));
    test_arm_bus_put(uc, BUS_PAGE + 4, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_DENY_FETCH, UC_BUS_ALLOW);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, BUS_PAGE + 4);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE | 1);
    test_arm_v8m_run(uc, &run, code_start, code_start + 0x100);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(log.first_type == UC_MEM_READ);
    TEST_CHECK(log.first_address == BUS_PAGE + 4);
    TEST_CHECK(log.page_fills == 2);
    TEST_CHECK(log.accesses == 0);
    TEST_MSG("intno %d pc 0x%x r0 0x%x fills %d", run.intno, run.pc,
             test_arm_v8m_get(uc, UC_ARM_REG_R0), log.page_fills);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_flush(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, 0, UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);

    log.secure_verdict = UC_BUS_FILTER_DATA;
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(log.page_fills == 1);
    TEST_CHECK(log.accesses == 0);

    OK(uc_ctl_flush_tlb(uc));
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0);
    TEST_CHECK(log.page_fills == 2);
    TEST_CHECK(log.accesses == 1);
    OK(uc_close(uc));
}

typedef struct _BUS_CHAIN {
    BUS_LOG *log;
    int calls;
    bool deny;
    bool remove;
    bool stop;
    uint64_t target;
} BUS_CHAIN;

static void test_arm_bus_chain_store(uc_engine *uc, uc_mem_type type,
                                     uint64_t address, int size, int64_t value,
                                     void *user_data)
{
    BUS_CHAIN *chain = user_data;

    if (++chain->calls == BUS_CHAIN_FLIP) {
        if (chain->deny) {
            chain->log->secure_verdict = UC_BUS_DENY_FETCH;
            OK(uc_ctl_flush_tlb(uc));
        }
        if (chain->remove) {
            OK(uc_ctl_remove_cache(uc, chain->target, chain->target + 0x400));
        }
        if (chain->stop) {
            OK(uc_emu_stop(uc));
        }
    }
}

static uc_engine *test_arm_bus_chain_open(V8M_RUN *run, BUS_LOG *log,
                                          BUS_CHAIN *chain, uint64_t page)
{
    uc_engine *uc;
    uc_hook hook;
    char code[] = "\x10\x60\x01\x3b\xfc\xd1\x00\xbf";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, run);
    test_arm_bus_log(log, page, 0, UC_BUS_ALLOW);
    test_arm_bus_install(uc, log);
    chain->log = log;
    chain->calls = 0;
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, test_arm_bus_chain_store,
                   chain, BUS_PAGE, BUS_PAGE + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, BUS_CHAIN_COUNT);
    return uc;
}

static void test_arm_bus_chain_run(uc_engine *uc, V8M_RUN *run, uint64_t begin)
{
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, begin | 1, code_start + 0x100, 0,
                    3 * BUS_CHAIN_COUNT + V8M_STEP_BOUND));
}

static void test_arm_bus_chain_leg(const char *name, uint64_t page,
                                   uint64_t target, bool deny, bool remove,
                                   bool stop, int intno, uint32_t pc,
                                   uint32_t r3)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    BUS_CHAIN chain = {NULL, 0, deny, remove, stop, target};

    uc = test_arm_bus_chain_open(&run, &log, &chain, page);
    test_arm_bus_chain_run(uc, &run, code_start);
    if (stop) {
        TEST_CHECK(run.intno == -1);
        TEST_CHECK(chain.calls == BUS_CHAIN_FLIP);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) ==
                   BUS_CHAIN_COUNT - (BUS_CHAIN_FLIP - 1));
        TEST_MSG("%s: first run intno %d pc 0x%x", name, run.intno, run.pc);
        test_arm_bus_chain_run(uc, &run, test_arm_v8m_get(uc, UC_ARM_REG_PC));
    }
    TEST_CHECK(run.intno == intno);
    TEST_CHECK(intno == -1 || run.pc == pc);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == r3);
    TEST_MSG("%s: intno %d pc 0x%x calls %d r3 %u", name, run.intno, run.pc,
             chain.calls, test_arm_v8m_get(uc, UC_ARM_REG_R3));
    OK(uc_close(uc));
}

static void test_arm_bus_filter_chained_tb(void)
{
    uint32_t at_flip = BUS_CHAIN_COUNT - (BUS_CHAIN_FLIP - 1);

    test_arm_bus_chain_leg("flush", code_start, code_start, true, false, false,
                           V8M_EXCP_PREFETCH_ABORT, code_start + 6, 0);
    test_arm_bus_chain_leg("flush and remove", code_start, code_start, true,
                           true, false, V8M_EXCP_PREFETCH_ABORT,
                           code_start + 6, 0);
    test_arm_bus_chain_leg("flush and stop", code_start, code_start, true,
                           false, true, V8M_EXCP_PREFETCH_ABORT, code_start,
                           at_flip);
    test_arm_bus_chain_leg("flush, remove and stop", code_start, code_start,
                           true, true, true, V8M_EXCP_PREFETCH_ABORT,
                           code_start, at_flip);
}

static void test_arm_bus_filter_remove_denied(void)
{
    test_arm_bus_chain_leg("data page denied, remove and stop", BUS_PAGE,
                           BUS_PAGE, true, true, true, -1, 0, 0);
    test_arm_bus_chain_leg("data page denied, remove", BUS_PAGE, BUS_PAGE,
                           true, true, false, -1, 0, 0);
    test_arm_bus_chain_leg("unmapped removal and stop", 0x777000, 0x9000,
                           false, true, true, -1, 0, 0);
}

static void test_arm_bus_filter_remove_new_code(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    static uint8_t buffer[0x400];
    char code[] = "\x00\xbf\x00\xbf";
    char target[] = "\x01\x20\x08\x47";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, target, sizeof(target) - 1);
    OK(uc_mem_map_ptr(uc, BUS_DATA, 0x400, UC_PROT_ALL, buffer));
    test_arm_bus_log(&log, BUS_DATA, 0, UC_BUS_ALLOW);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, code_start | 1);
    test_arm_v8m_run(uc, &run, BUS_DATA, code_start + 2);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 1);

    buffer[0] = 2;
    log.secure_verdict = UC_BUS_DENY_FETCH;
    OK(uc_ctl_flush_tlb(uc));
    OK(uc_ctl_remove_cache(uc, BUS_DATA, BUS_DATA + 0x400));
    log.secure_verdict = 0;
    OK(uc_ctl_flush_tlb(uc));
    test_arm_v8m_run(uc, &run, BUS_DATA, code_start + 2);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 2);
    TEST_MSG("intno %d r0 %u", run.intno, test_arm_v8m_get(uc, UC_ARM_REG_R0));
    OK(uc_close(uc));
}

typedef struct _BUS_FLIP {
    BUS_LOG *log;
    int calls;
    bool flush;
    bool exit_after;
} BUS_FLIP;

static void test_arm_bus_flip_store(uc_engine *uc, uc_mem_type type,
                                    uint64_t address, int size, int64_t value,
                                    void *user_data)
{
    BUS_FLIP *flip = user_data;

    if (++flip->calls != BUS_CHAIN_FLIP) {
        return;
    }
    if (flip->exit_after) {
        flip->log->secure_verdict = UC_BUS_DENY_FETCH;
    } else {
        flip->log->secure_verdict = UC_BUS_FILTER_DATA;
    }
    if (flip->flush) {
        OK(uc_ctl_flush_tlb(uc));
    }
    if (flip->exit_after) {
        OK(uc_ctl_exit_after_insn(uc));
    }
}

static int test_arm_bus_data_flip_leg(bool flush, BUS_LOG *log)
{
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    BUS_FLIP flip = {log, 0, flush, false};
    char code[] = "\x10\x60\x29\x68\x01\x3b\xfb\xd1\x00\xbf";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mem_map(uc, BUS_DATA, 0x400, UC_PROT_READ | UC_PROT_WRITE));
    test_arm_bus_put(uc, BUS_DATA, BUS_WORD);
    test_arm_bus_log(log, BUS_DATA, 0, UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, log);
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, test_arm_bus_flip_store, &flip,
                   BUS_PAGE, BUS_PAGE + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, BUS_CHAIN_COUNT);
    test_arm_v8m_reg(uc, UC_ARM_REG_R5, BUS_DATA);
    run.intno = -1;
    OK(uc_emu_start(uc, code_start | 1, code_start + 8, 0,
                    4 * BUS_CHAIN_COUNT + V8M_STEP_BOUND));
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(flip.calls == BUS_CHAIN_COUNT);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == 0);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == (flush ? 0 : BUS_WORD));
    OK(uc_close(uc));
    return log->accesses;
}

static void test_arm_bus_filter_chained_data(void)
{
    BUS_LOG log;
    int flushed = test_arm_bus_data_flip_leg(true, &log);
    int kept = test_arm_bus_data_flip_leg(false, &log);

    TEST_CHECK(flushed == BUS_CHAIN_COUNT - (BUS_CHAIN_FLIP - 1));
    TEST_CHECK(kept == 0);
    TEST_MSG("filtered loads with the flush %d, without %d", flushed, kept);
}

static void test_arm_bus_filter_exit_after_insn(void)
{
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    BUS_LOG log;
    BUS_FLIP flip = {&log, 0, true, true};
    char code[] = "\x10\x60\x01\x3b\xfc\xd1\x00\xbf";
    uint32_t at_flip = BUS_CHAIN_COUNT - (BUS_CHAIN_FLIP - 1);

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_ctl_exit_after_insn_enable(uc));
    test_arm_bus_log(&log, code_start, 0, UC_BUS_ALLOW);
    test_arm_bus_install(uc, &log);
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, test_arm_bus_flip_store, &flip,
                   BUS_PAGE, BUS_PAGE + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, BUS_CHAIN_COUNT);
    test_arm_bus_chain_run(uc, &run, code_start);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(flip.calls == BUS_CHAIN_FLIP);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_PC) == code_start + 2);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == at_flip);
    TEST_MSG("first run intno %d pc 0x%x r3 %u", run.intno,
             test_arm_v8m_get(uc, UC_ARM_REG_PC),
             test_arm_v8m_get(uc, UC_ARM_REG_R3));
    test_arm_bus_chain_run(uc, &run, test_arm_v8m_get(uc, UC_ARM_REG_PC));
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == code_start + 2);
    TEST_CHECK(flip.calls == BUS_CHAIN_FLIP);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R3) == at_flip);
    TEST_MSG("resume intno %d pc 0x%x r3 %u", run.intno, run.pc,
             test_arm_v8m_get(uc, UC_ARM_REG_R3));
    OK(uc_close(uc));
}

static void test_arm_bus_filter_unaligned(void)
{
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    V8M_FAULT fault = {-1, 0, 0};
    BUS_LOG log;
    char code[] = "\x08\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_fault_intr, &fault, 1,
                   0));
    test_arm_bus_put(uc, BUS_PAGE, 0x33221100);
    test_arm_bus_put(uc, BUS_PAGE + 4, 0x77665544);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA, UC_BUS_ALLOW);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_load(uc, &run, BUS_PAGE + 2);
    TEST_CHECK(fault.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x55443322);
    TEST_CHECK(log.accesses == 2);
    TEST_CHECK(log.address == BUS_PAGE + 4 && log.size == 4);

    log.accesses = 0;
    log.access_verdict = UC_BUS_FAULT;
    test_arm_v8m_load(uc, &run, BUS_PAGE + 2);
    TEST_CHECK(fault.intno == V8M_EXCP_DATA_ABORT);
    TEST_CHECK(fault.status == BUS_FAULT_EXTERNAL);
    TEST_CHECK(fault.address == BUS_PAGE);
    TEST_CHECK(log.accesses == 1);
    TEST_CHECK(log.address == BUS_PAGE && log.size == 4);
    TEST_MSG("status 0x%x address 0x%x calls %d", fault.status, fault.address,
             log.accesses);
    OK(uc_close(uc));
}

typedef struct _BUS_HOOKS {
    int reads;
    int writes;
    int64_t written;
    bool flush;
} BUS_HOOKS;

static void test_arm_bus_hook_access(uc_engine *uc, uc_mem_type type,
                                     uint64_t address, int size, int64_t value,
                                     void *user_data)
{
    BUS_HOOKS *hooks = user_data;

    if (type == UC_MEM_WRITE) {
        hooks->writes++;
        hooks->written = value;
        if (hooks->flush) {
            OK(uc_ctl_flush_tlb(uc));
        }
    } else {
        hooks->reads++;
    }
}

static uc_engine *test_arm_bus_hooked_open(V8M_RUN *run, BUS_LOG *log,
                                           BUS_HOOKS *hooks, int types)
{
    uc_engine *uc;
    uc_hook hook;
    char code[] = "\x10\x60\x11\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, run);
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(log, BUS_PAGE, UC_BUS_FILTER_DATA, UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, log);
    OK(uc_hook_add(uc, &hook, types, test_arm_bus_hook_access, hooks, BUS_PAGE,
                   BUS_PAGE + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE);
    test_arm_v8m_run(uc, run, code_start, code_start + 4);
    TEST_CHECK(run->intno == -1);
    TEST_CHECK(test_arm_v8m_word(uc, BUS_PAGE) == BUS_WORD);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0);
    TEST_CHECK(log->accesses == 2);
    TEST_CHECK(log->stored == 0x55aa);
    TEST_CHECK(hooks->writes == 1);
    TEST_CHECK(hooks->written == 0x55aa);
    return uc;
}

static void test_arm_bus_filter_mem_hooks(void)
{
    V8M_RUN run;
    BUS_LOG log;
    BUS_HOOKS hooks = {0, 0, 0, false};
    uc_engine *uc = test_arm_bus_hooked_open(
        &run, &log, &hooks, UC_HOOK_MEM_WRITE | UC_HOOK_MEM_READ);

    TEST_CHECK(hooks.reads == 0);
    TEST_CHECK(log.page_fills == 1);
    OK(uc_close(uc));
}

typedef struct _BUS_ORDER {
    int reads;
    int afters;
    int64_t after_value;
} BUS_ORDER;

static void test_arm_bus_order_hook(uc_engine *uc, uc_mem_type type,
                                    uint64_t address, int size, int64_t value,
                                    void *user_data)
{
    BUS_ORDER *order = user_data;

    if (type == UC_MEM_READ_AFTER) {
        order->afters++;
        order->after_value = value;
    } else {
        order->reads++;
    }
}

static void test_arm_bus_order_leg(int verdict, uint32_t offset)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    BUS_ORDER order = {0, 0, -1};
    uc_hook hook;
    char code[] = "\x11\x68";
    uint32_t stored = 0;
    uint32_t expected;
    bool raz = verdict == UC_BUS_RAZ_WI;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_put(uc, BUS_PAGE + 4, ~BUS_WORD);
    OK(uc_mem_read(uc, BUS_PAGE + offset, &stored, 4));
    expected = raz ? 0 : stored;
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA, verdict);
    test_arm_bus_install(uc, &log);
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_READ | UC_HOOK_MEM_READ_AFTER,
                   test_arm_bus_order_hook, &order, BUS_PAGE, BUS_PAGE + 7));
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, BUS_PAGE + offset);
    test_arm_v8m_run(uc, &run, code_start, code_start + 2);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == expected);
    TEST_CHECK(order.reads == (raz ? 0 : 1));
    TEST_CHECK(order.afters == 1);
    TEST_CHECK(order.after_value == expected);
    TEST_CHECK(log.accesses == (offset == 0 ? 1 : 2));
    TEST_MSG("%s at +%u: r1 0x%x reads %d afters %d after 0x%llx calls %d",
             raz ? "RAZ" : "allow", offset,
             test_arm_v8m_get(uc, UC_ARM_REG_R1), order.reads, order.afters,
             (unsigned long long)order.after_value, log.accesses);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_read_hook_order(void)
{
    test_arm_bus_order_leg(UC_BUS_RAZ_WI, 0);
    test_arm_bus_order_leg(UC_BUS_ALLOW, 0);
    test_arm_bus_order_leg(UC_BUS_RAZ_WI, 2);
    test_arm_bus_order_leg(UC_BUS_ALLOW, 2);
}

static void test_arm_bus_filter_hook_refill(void)
{
    V8M_RUN run;
    BUS_LOG log;
    BUS_HOOKS hooks = {0, 0, 0, true};
    uc_engine *uc =
        test_arm_bus_hooked_open(&run, &log, &hooks, UC_HOOK_MEM_WRITE);

    TEST_CHECK(log.page_fills == 2);
    TEST_MSG("fills %d", log.page_fills);
    OK(uc_close(uc));
}

static bool test_arm_bus_prot_drop(uc_engine *uc, uc_mem_type type,
                                   uint64_t address, int size, int64_t value,
                                   void *user_data)
{
    int *calls = user_data;

    (*calls)++;
    OK(uc_ctl_drop_store(uc));
    return true;
}

static void test_arm_bus_filter_prot_drop(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    uc_hook hook;
    int drops = 0;
    char code[] = "\x10\x60\x11\x68";
    uint32_t flash = 0x6000;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mem_map(uc, flash, 0x400, UC_PROT_READ | UC_PROT_EXEC));
    test_arm_bus_put(uc, flash, BUS_WORD);
    test_arm_bus_log(&log, flash, UC_BUS_FILTER_DATA, UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, &log);
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE_PROT, test_arm_bus_prot_drop,
                   &drops, flash, flash + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, flash);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(drops == 1);
    TEST_CHECK(test_arm_v8m_word(uc, flash) == BUS_WORD);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0);
    TEST_CHECK(log.accesses == 1);
    TEST_CHECK(log.writes == 0);
    TEST_CHECK(log.type == UC_MEM_READ);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_alias(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x10\x60\x19\x68";
    uint32_t target = 0x20000;
    uint32_t window = 0x30000;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mem_map(uc, target, 0x400, UC_PROT_ALL));
    OK(uc_mem_map_alias(uc, window, 0x400, UC_PROT_ALL, target));
    test_arm_bus_put(uc, target, BUS_WORD);
    test_arm_bus_log(&log, target, UC_BUS_FILTER_DATA, UC_BUS_RAZ_WI);
    log.alias = window;
    test_arm_bus_install(uc, &log);
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0x55aa);
    test_arm_v8m_reg(uc, UC_ARM_REG_R1, 0xffffffff);
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, window);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, target);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_word(uc, target) == BUS_WORD);
    TEST_CHECK(test_arm_v8m_word(uc, window) == BUS_WORD);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R1) == 0);
    TEST_CHECK(log.page_fills == 2);
    TEST_CHECK(log.store_address == window);
    TEST_CHECK(log.address == target);
    TEST_CHECK(log.accesses == 2);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_ctl(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    char code[] = "\x08\x68";
    int ignored = 0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
    test_arm_bus_log(&log, BUS_PAGE, UC_BUS_FILTER_DATA, UC_BUS_RAZ_WI);
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl_bus_filter(uc, test_arm_bus_page, NULL, &log));
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl_bus_filter(uc, NULL, test_arm_bus_access, &log));
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(log.page_fills == 0);

    test_arm_bus_install(uc, &log);
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl_bus_filter(uc, NULL, test_arm_bus_access, &log));
    uc_assert_err(UC_ERR_ARG,
                  uc_ctl(uc, UC_CTL_READ(UC_CTL_BUS_FILTER, 1), &ignored));
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0);
    TEST_CHECK(log.accesses == 1);

    OK(uc_ctl_bus_filter(uc, NULL, NULL, NULL));
    test_arm_v8m_load(uc, &run, BUS_PAGE);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(log.accesses == 1);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_ram_only(void)
{
    uc_engine *uc;
    V8M_RUN run;
    BUS_LOG log;
    V8M_PLAIN_LOG mmio = {0};
    char code[] = "\x08\x68";

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    OK(uc_mmio_map(uc, V8M_MMIO, 0x1000, test_arm_mmio_plain_read, &mmio,
                   test_arm_mmio_plain_write, &mmio));
    test_arm_bus_log(&log, V8M_MMIO, UC_BUS_FILTER_DATA | UC_BUS_DENY_FETCH,
                     UC_BUS_RAZ_WI);
    test_arm_bus_install(uc, &log);
    test_arm_v8m_load(uc, &run, V8M_MMIO + 0x24);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(mmio.reads == 1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0x5a5a0024);
    TEST_CHECK(log.page_fills == 0);
    TEST_CHECK(log.accesses == 0);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_no_security(void)
{
    uc_cpu_arm models[] = {UC_CPU_ARM_CORTEX_M0, UC_CPU_ARM_CORTEX_M3,
                           UC_CPU_ARM_CORTEX_M4, UC_CPU_ARM_CORTEX_M7};
    char code[] = "\x08\x68";
    uc_engine *uc;
    uc_hook hook;
    V8M_RUN run;
    BUS_LOG log;
    size_t i;

    for (i = 0; i < sizeof(models) / sizeof(models[0]); i++) {
        uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                        models[i]);
        OK(uc_hook_add(uc, &hook, UC_HOOK_INTR, test_arm_v8m_intr, &run, 1, 0));
        test_arm_bus_put(uc, BUS_PAGE, BUS_WORD);
        test_arm_bus_log(&log, BUS_PAGE, 0, UC_BUS_RAZ_WI);
        log.ns_verdict = UC_BUS_FILTER_DATA;
        test_arm_bus_install(uc, &log);
        test_arm_v8m_load(uc, &run, BUS_PAGE);
        TEST_CHECK(run.intno == -1);
        TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == 0);
        TEST_CHECK(log.page_fills == 1);
        TEST_CHECK(!log.page_attrs.secure && log.page_attrs.privileged);
        TEST_CHECK(log.accesses == 1 && !log.attrs.secure);
        TEST_MSG("model %d", (int)models[i]);
        OK(uc_close(uc));
    }
}

#define BLOCK_HIGH (code_start + 0x200)

typedef struct _BUS_BLOCKS {
    BUS_LOG log;
    uint64_t denied;
    bool each;
    int fetch_fills;
} BUS_BLOCKS;

static unsigned test_arm_bus_block_page(uc_engine *uc, uint64_t page,
                                        uint64_t address, uc_mem_attrs attrs,
                                        uc_mem_type access_type,
                                        void *user_data)
{
    BUS_BLOCKS *blocks = user_data;
    unsigned verdict = blocks->each ? UC_BUS_FETCH_EACH : 0;

    if (page != code_start) {
        return 0;
    }
    if (access_type == UC_MEM_FETCH) {
        blocks->fetch_fills++;
    }
    if (address >= blocks->denied && address < blocks->denied + 0x200) {
        verdict |= UC_BUS_DENY_FETCH;
    }
    return verdict;
}

static uc_engine *test_arm_bus_blocks_open(BUS_BLOCKS *blocks, V8M_RUN *run,
                                           uint64_t denied, bool each)
{
    uc_engine *uc;
    char nop[] = "\x00\xbf";

    uc = test_arm_v8m_open(nop, sizeof(nop) - 1, run);
    test_arm_bus_log(&blocks->log, code_start, 0, UC_BUS_ALLOW);
    blocks->denied = denied;
    blocks->each = each;
    blocks->fetch_fills = 0;
    OK(uc_ctl_bus_filter(uc, test_arm_bus_block_page, test_arm_bus_access,
                         blocks));
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0);
    return uc;
}

static void test_arm_bus_adds(uc_engine *uc, uint64_t at, int count)
{
    char adds[] = "\x01\x30";
    int i;

    for (i = 0; i < count; i++) {
        OK(uc_mem_write(uc, at + 2 * i, adds, 2));
    }
}

static void test_arm_bus_into_high_leg(bool each, V8M_RUN *run, uint32_t *r0,
                                       int *fills)
{
    BUS_BLOCKS blocks;
    uc_engine *uc = test_arm_bus_blocks_open(&blocks, run, BLOCK_HIGH, each);

    test_arm_bus_adds(uc, BLOCK_HIGH - 0x10, 16);
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, (BLOCK_HIGH - 0x10) | 1, BLOCK_HIGH + 0x10, 0, 64));
    *r0 = test_arm_v8m_get(uc, UC_ARM_REG_R0);
    *fills = blocks.fetch_fills;
    OK(uc_close(uc));
}

static void test_arm_bus_into_low_leg(bool each, V8M_RUN *run, uint32_t *r0)
{
    BUS_BLOCKS blocks;
    char branch[] = "\xfd\xe6";
    uc_engine *uc = test_arm_bus_blocks_open(&blocks, run, code_start, each);

    test_arm_bus_adds(uc, code_start, 1);
    test_arm_bus_adds(uc, BLOCK_HIGH, 1);
    OK(uc_mem_write(uc, BLOCK_HIGH + 2, branch, 2));
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, BLOCK_HIGH | 1, code_start + 2, 0, 64));
    *r0 = test_arm_v8m_get(uc, UC_ARM_REG_R0);
    OK(uc_close(uc));
}

static void test_arm_bus_filter_fetch_each(void)
{
    V8M_RUN run;
    BUS_BLOCKS blocks;
    uc_engine *uc;
    uint32_t r0 = 0;
    int fills = 0;
    char load[] = "\x08\x68";

    test_arm_bus_into_high_leg(true, &run, &r0, &fills);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == BLOCK_HIGH);
    TEST_CHECK(r0 == 8);
    TEST_CHECK(fills >= 9);
    TEST_MSG("into the high block: intno %d pc 0x%x r0 %u fills %d",
             run.intno, run.pc, r0, fills);

    test_arm_bus_into_high_leg(false, &run, &r0, &fills);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(r0 == 16);
    TEST_MSG("into the high block, verdict kept: intno %d r0 %u fills %d",
             run.intno, r0, fills);

    test_arm_bus_into_low_leg(true, &run, &r0);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == code_start);
    TEST_CHECK(r0 == 1);
    TEST_MSG("into the low block: intno %d pc 0x%x r0 %u", run.intno, run.pc,
             r0);

    test_arm_bus_into_low_leg(false, &run, &r0);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(r0 == 2);
    TEST_MSG("into the low block, verdict kept: intno %d r0 %u", run.intno,
             r0);

    uc = test_arm_bus_blocks_open(&blocks, &run, BLOCK_HIGH, true);
    OK(uc_mem_write(uc, code_start, load, sizeof(load) - 1));
    test_arm_bus_put(uc, BLOCK_HIGH + 0x100, BUS_WORD);
    test_arm_v8m_load(uc, &run, BLOCK_HIGH + 0x100);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_R0) == BUS_WORD);
    TEST_CHECK(blocks.log.accesses == 0);
    OK(uc_close(uc));
}

#define STRADDLE_LOOP (code_start + 0x3f4)
#define STRADDLE_MOVW (code_start + 0x3fe)
#define STRADDLE_PAGE (code_start + 0x400)
#define STRADDLE_EXIT (code_start + 0x406)
#define STRADDLE_DATA 0x2000
#define STRADDLE_COUNT 3000
#define STRADDLE_FLIP 1000

typedef struct _BUS_STRADDLE {
    bool sau;
    bool flip;
    bool denied;
    int stores;
} BUS_STRADDLE;

static unsigned test_arm_bus_straddle_page(uc_engine *uc, uint64_t page,
                                           uint64_t address, uc_mem_attrs attrs,
                                           uc_mem_type access_type,
                                           void *user_data)
{
    BUS_STRADDLE *straddle = user_data;

    if (page != STRADDLE_PAGE) {
        return 0;
    }
    if (straddle->denied && address < STRADDLE_PAGE + 0x200) {
        return UC_BUS_FETCH_EACH | UC_BUS_DENY_FETCH;
    }
    return UC_BUS_FETCH_EACH;
}

static int test_arm_bus_straddle_access(uc_engine *uc, uint64_t address,
                                        unsigned size, uc_mem_attrs attrs,
                                        uc_mem_type access_type,
                                        uint64_t *value, void *user_data)
{
    return UC_BUS_ALLOW;
}

static void test_arm_bus_straddle_store(uc_engine *uc, uc_mem_type type,
                                        uint64_t address, int size,
                                        int64_t value, void *user_data)
{
    BUS_STRADDLE *straddle = user_data;

    if (++straddle->stores != STRADDLE_FLIP || !straddle->flip) {
        return;
    }
    if (straddle->sau) {
        test_arm_v8m_sau_region(uc, 0, STRADDLE_PAGE,
                                STRADDLE_PAGE | V8M_SAU_ENABLE);
        return;
    }
    straddle->denied = true;
    OK(uc_ctl_flush_tlb(uc));
}

static void test_arm_bus_straddle_leg(bool sau, bool flip, V8M_RUN *run,
                                      uint32_t *r3)
{
    uc_engine *uc;
    uc_hook hook;
    BUS_STRADDLE straddle = {sau, flip, false, 0};
    char loop[] = "\x10\x60\x01\x30\x01\x3b\x04\xd0\x00\xbf"
                  "\x40\xf2\x00\x01\xf7\xe7\x00\xbf\x00\xbf";

    uc = test_arm_v8m_open("\x00\xbf", 2, run);
    OK(uc_mem_write(uc, STRADDLE_LOOP, loop, sizeof(loop) - 1));
    if (sau) {
        OK(uc_ctl_arm_security(uc, 1));
        test_arm_v8m_sau_region(uc, 0, STRADDLE_PAGE + 0x20,
                                (STRADDLE_PAGE + 0x20) | V8M_SAU_ENABLE);
        test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    } else {
        OK(uc_ctl_bus_filter(uc, test_arm_bus_straddle_page,
                             test_arm_bus_straddle_access, &straddle));
    }
    OK(uc_hook_add(uc, &hook, UC_HOOK_MEM_WRITE, test_arm_bus_straddle_store,
                   &straddle, STRADDLE_DATA, STRADDLE_DATA + 3));
    test_arm_v8m_reg(uc, UC_ARM_REG_R2, STRADDLE_DATA);
    test_arm_v8m_reg(uc, UC_ARM_REG_R3, STRADDLE_COUNT);
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, STRADDLE_LOOP | 1, STRADDLE_EXIT, 0,
                    9 * STRADDLE_COUNT));
    *r3 = test_arm_v8m_get(uc, UC_ARM_REG_R3);
    OK(uc_close(uc));
}

static void test_arm_bus_straddle_check(bool sau)
{
    V8M_RUN run;
    uint32_t r3 = 0;

    test_arm_bus_straddle_leg(sau, true, &run, &r3);
    TEST_CHECK(run.intno == V8M_EXCP_PREFETCH_ABORT);
    TEST_CHECK(run.pc == STRADDLE_MOVW);
    TEST_CHECK(r3 == STRADDLE_COUNT - STRADDLE_FLIP);
    TEST_MSG("%s, verdict changed: intno %d pc 0x%x r3 %u",
             sau ? "SAU" : "fetch each", run.intno, run.pc, r3);

    test_arm_bus_straddle_leg(sau, false, &run, &r3);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(r3 == 0);
    TEST_MSG("%s, verdict kept: intno %d r3 %u", sau ? "SAU" : "fetch each",
             run.intno, r3);
}

static void test_arm_bus_filter_fetch_each_straddle(void)
{
    test_arm_bus_straddle_check(false);
}

static void test_arm_subpage_straddle(void)
{
    test_arm_bus_straddle_check(true);
}

#define SUBPAGE_CODE_SIZE 0x60
#define SUBPAGE_BOUNDARY (code_start + 0x40)

static void test_arm_subpage_boundary_leg(bool split, V8M_RUN *run,
                                          uint32_t *r0)
{
    uc_engine *uc;
    char code[SUBPAGE_CODE_SIZE];
    size_t i;

    for (i = 0; i < sizeof(code); i += 2) {
        code[i] = 0x01;
        code[i + 1] = 0x30;
    }
    uc = test_arm_v8m_open(code, sizeof(code), run);
    OK(uc_ctl_arm_security(uc, 1));
    if (split) {
        test_arm_v8m_sau_region(uc, 0, SUBPAGE_BOUNDARY,
                                SUBPAGE_BOUNDARY | V8M_SAU_ENABLE);
    }
    test_arm_v8m_reg(uc, UC_ARM_REG_SAU_CTRL, V8M_SAU_ENABLE);
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, 0);
    run->intno = -1;
    run->pc = 0;
    OK(uc_emu_start(uc, code_start | 1, code_start + sizeof(code), 0, 0));
    *r0 = test_arm_v8m_get(uc, UC_ARM_REG_R0);
    OK(uc_close(uc));
}

static void test_arm_subpage_boundary_fetch(void)
{
    V8M_RUN run;
    uint32_t r0 = 0;

    test_arm_subpage_boundary_leg(true, &run, &r0);
    TEST_CHECK(run.intno != -1);
    TEST_CHECK(run.pc == SUBPAGE_BOUNDARY);
    TEST_CHECK(r0 == (SUBPAGE_BOUNDARY - code_start) / 2);
    TEST_MSG("split: intno %d pc 0x%x r0 %u", run.intno, run.pc, r0);

    test_arm_subpage_boundary_leg(false, &run, &r0);
    TEST_CHECK(run.intno == -1);
    TEST_CHECK(r0 == SUBPAGE_CODE_SIZE / 2);
    TEST_MSG("whole page Secure: intno %d r0 %u", run.intno, r0);
}

static uint32_t test_arm_v8m_nsacr_fpca(uint32_t nsacr)
{
    uc_engine *uc;
    V8M_RUN run;
    uint32_t control;
    char code[] = "\x80\xf3\x14\x88";

    uc = test_arm_v8m_ns_open(code, sizeof(code) - 1, &run);
    test_arm_v8m_reg(uc, UC_ARM_REG_NSACR, nsacr);
    test_arm_v8m_reg(uc, UC_ARM_REG_R0, V8M_CONTROL_FPCA);
    test_arm_v8m_run(uc, &run, code_start, code_start + 4);
    TEST_CHECK(run.intno == -1);
    control = test_arm_v8m_get(uc, UC_ARM_REG_CONTROL_S);
    OK(uc_close(uc));
    return control & V8M_CONTROL_FPCA;
}

static void test_arm_v8m_nsacr(void)
{
    uc_engine *uc;
    V8M_RUN run;
    char code[] = "\x00\xbf";
    uint32_t ignored = 0;

    uc = test_arm_v8m_open(code, sizeof(code) - 1, &run);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_NSACR) == 0);
    test_arm_v8m_reg(uc, UC_ARM_REG_NSACR, 0xffffffff);
    TEST_CHECK(test_arm_v8m_get(uc, UC_ARM_REG_NSACR) == 0xcff);
    OK(uc_close(uc));

    TEST_CHECK(test_arm_v8m_nsacr_fpca(0xc00) == V8M_CONTROL_FPCA);
    TEST_CHECK(test_arm_v8m_nsacr_fpca(0) == 0);

    uc_common_setup(&uc, UC_ARCH_ARM, UC_MODE_THUMB, code, sizeof(code) - 1,
                    UC_CPU_ARM_CORTEX_M7);
    uc_assert_err(UC_ERR_ARG, uc_reg_read(uc, UC_ARM_REG_NSACR, &ignored));
    uc_assert_err(UC_ERR_ARG, uc_reg_write(uc, UC_ARM_REG_NSACR, &ignored));
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
             {"test_arm_v8m_lookup_on", test_arm_v8m_lookup_on},
             {"test_arm_v8m_security_off", test_arm_v8m_security_off},
             {"test_arm_v8m_security_state", test_arm_v8m_security_state},
             {"test_arm_v8m_sau_attribution", test_arm_v8m_sau_attribution},
             {"test_arm_v8m_tt", test_arm_v8m_tt},
             {"test_arm_v8m_sau_chained_tb", test_arm_v8m_sau_chained_tb},
             {"test_arm_v8m_sau_isb", test_arm_v8m_sau_isb},
             {"test_arm_v8m_sau_subpage", test_arm_v8m_sau_subpage},
             {"test_arm_v8m_fp_context", test_arm_v8m_fp_context},
             {"test_arm_v8m_banked_sp", test_arm_v8m_banked_sp},
             {"test_arm_v8m_banked_special", test_arm_v8m_banked_special},
             {"test_arm_v8m_fault_status", test_arm_v8m_fault_status},
             {"test_arm_v8m_mmio_attrs", test_arm_v8m_mmio_attrs},
             {"test_arm_v8m_mmio_attrs_sau", test_arm_v8m_mmio_attrs_sau},
             {"test_arm_v8m_mmio_attrs_chained",
              test_arm_v8m_mmio_attrs_chained},
             {"test_arm_mmio_plain_unchanged", test_arm_mmio_plain_unchanged},
             {"test_arm_mmio_attrs_split", test_arm_mmio_attrs_split},
             {"test_arm_mmio_attrs_no_security",
              test_arm_mmio_attrs_no_security},
             {"test_arm_v8m_remove_cache_secure_fault",
              test_arm_v8m_remove_cache_secure_fault},
             {"test_arm_v8m_remove_cache_unfetchable",
              test_arm_v8m_remove_cache_unfetchable},
             {"test_arm_v8m_remove_cache_subpage",
              test_arm_v8m_remove_cache_subpage},
             {"test_arm_bus_filter_fetch_first",
              test_arm_bus_filter_fetch_first},
             {"test_arm_bus_filter_fast", test_arm_bus_filter_fast},
             {"test_arm_bus_filter_per_entry", test_arm_bus_filter_per_entry},
             {"test_arm_bus_filter_sau_downgrade",
              test_arm_bus_filter_sau_downgrade},
             {"test_arm_bus_filter_raz_wi", test_arm_bus_filter_raz_wi},
             {"test_arm_bus_filter_fault", test_arm_bus_filter_fault},
             {"test_arm_mmio_refuse", test_arm_mmio_refuse},
             {"test_arm_mmio_refuse_it_block", test_arm_mmio_refuse_it_block},
             {"test_arm_bus_filter_exec_after_data",
              test_arm_bus_filter_exec_after_data},
             {"test_arm_bus_filter_flush", test_arm_bus_filter_flush},
             {"test_arm_bus_filter_chained_tb", test_arm_bus_filter_chained_tb},
             {"test_arm_bus_filter_remove_denied",
              test_arm_bus_filter_remove_denied},
             {"test_arm_bus_filter_remove_new_code",
              test_arm_bus_filter_remove_new_code},
             {"test_arm_bus_filter_unaligned", test_arm_bus_filter_unaligned},
             {"test_arm_bus_filter_chained_data",
              test_arm_bus_filter_chained_data},
             {"test_arm_bus_filter_exit_after_insn",
              test_arm_bus_filter_exit_after_insn},
             {"test_arm_bus_filter_mem_hooks", test_arm_bus_filter_mem_hooks},
             {"test_arm_bus_filter_hook_refill",
              test_arm_bus_filter_hook_refill},
             {"test_arm_bus_filter_prot_drop", test_arm_bus_filter_prot_drop},
             {"test_arm_bus_filter_alias", test_arm_bus_filter_alias},
             {"test_arm_bus_filter_ctl", test_arm_bus_filter_ctl},
             {"test_arm_bus_filter_ram_only", test_arm_bus_filter_ram_only},
             {"test_arm_bus_filter_no_security",
              test_arm_bus_filter_no_security},
             {"test_arm_subpage_boundary_fetch",
              test_arm_subpage_boundary_fetch},
             {"test_arm_bus_filter_fetch_each", test_arm_bus_filter_fetch_each},
             {"test_arm_bus_filter_read_hook_order",
              test_arm_bus_filter_read_hook_order},
             {"test_arm_bus_filter_fetch_each_straddle",
              test_arm_bus_filter_fetch_each_straddle},
             {"test_arm_subpage_straddle", test_arm_subpage_straddle},
             {"test_arm_v8m_nsacr", test_arm_v8m_nsacr},
             {NULL, NULL}};
