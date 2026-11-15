#ifndef GEN_ICOUNT_H
#define GEN_ICOUNT_H

#include "qemu/timer.h"

/* Helpers for instruction counting code generation.  */

static inline void gen_io_start(TCGContext *tcg_ctx)
{
    TCGv_i32 tmp = tcg_const_i32(tcg_ctx, 1);
    tcg_gen_st_i32(tcg_ctx, tmp, tcg_ctx->cpu_env,
                   offsetof(ArchCPU, parent_obj.can_do_io) -
                   offsetof(ArchCPU, env));
    tcg_temp_free_i32(tcg_ctx, tmp);
}

/*
 * cpu->can_do_io is cleared automatically at the beginning of
 * each translation block.  The cost is minimal and only paid
 * for -icount, plus it would be very easy to forget doing it
 * in the translator.  Therefore, backends only need to call
 * gen_io_start.
 */
static inline void gen_io_end(TCGContext *tcg_ctx)
{
    TCGv_i32 tmp = tcg_const_i32(tcg_ctx, 0);
    tcg_gen_st_i32(tcg_ctx, tmp, tcg_ctx->cpu_env,
                   offsetof(ArchCPU, parent_obj.can_do_io) -
                   offsetof(ArchCPU, env));
    tcg_temp_free_i32(tcg_ctx, tmp);
}

static inline void gen_tb_start(TCGContext *tcg_ctx, TranslationBlock *tb)
{
    TCGv_ptr puc = tcg_const_ptr(tcg_ctx, tcg_ctx->uc);
    TCGv_i32 tmp = tcg_const_i32(tcg_ctx, 0);
    // Unicorn:
    //    We CANT'T use brcondi_i32 here or we will fail liveness analysis
    //    because it marks the end of BB
    if (tcg_ctx->delay_slot_flag != NULL) {
        tcg_gen_mov_i32(tcg_ctx, tmp, tcg_ctx->delay_slot_flag);
    }
    gen_helper_check_exit_request(tcg_ctx, puc, tmp);
    tcg_temp_free_i32(tcg_ctx, tmp);
    tcg_temp_free_ptr(tcg_ctx, puc);
}

#define UC_TB_BUDGET_OFS(field)                                                \
    ((int)offsetof(ArchCPU, neg.uc_budget.field) - (int)offsetof(ArchCPU, env))

static inline void gen_tb_start_budget(TCGContext *tcg_ctx,
                                       TranslationBlock *tb)
{
    TCGv_i32 exit32 = tcg_temp_new_i32(tcg_ctx);
    TCGv_i32 async = tcg_temp_new_i32(tcg_ctx);
    TCGv_i64 exit64 = tcg_temp_new_i64(tcg_ctx);
    TCGv_i64 budget = tcg_temp_new_i64(tcg_ctx);
    TCGv_i32 count32;
    TCGv_i64 count;
    TCGv_i64 block;

    tcg_ctx->uc_budget_exit = gen_new_label(tcg_ctx);
    tcg_gen_ld_i32(tcg_ctx, exit32, tcg_ctx->cpu_env,
                   offsetof(ArchCPU, neg.icount_decr.u32) -
                       offsetof(ArchCPU, env));
    tcg_gen_ld_i32(tcg_ctx, async, tcg_ctx->cpu_env,
                   UC_TB_BUDGET_OFS(async_exit));
    tcg_gen_neg_i32(tcg_ctx, async, async);
    tcg_gen_or_i32(tcg_ctx, exit32, exit32, async);
    tcg_gen_ext_i32_i64(tcg_ctx, exit64, exit32);
    tcg_gen_ld_i64(tcg_ctx, budget, tcg_ctx->cpu_env, UC_TB_BUDGET_OFS(budget));
    tcg_gen_subi_i64(tcg_ctx, budget, budget, 1);
    tcg_gen_or_i64(tcg_ctx, exit64, exit64, budget);
    tcg_gen_brcondi_i64(tcg_ctx, TCG_COND_LT, exit64, 0,
                        tcg_ctx->uc_budget_exit);
    tcg_temp_free_i64(tcg_ctx, budget);
    tcg_temp_free_i64(tcg_ctx, exit64);
    tcg_temp_free_i32(tcg_ctx, async);
    tcg_temp_free_i32(tcg_ctx, exit32);

    budget = tcg_temp_new_i64(tcg_ctx);
    count = tcg_temp_new_i64(tcg_ctx);
    block = tcg_temp_new_i64(tcg_ctx);
    count32 = tcg_const_i32(tcg_ctx, 0);
    tcg_ctx->uc_budget_count_op = tcg_last_op(tcg_ctx);
    tcg_gen_extu_i32_i64(tcg_ctx, count, count32);
    tcg_gen_ld_i64(tcg_ctx, budget, tcg_ctx->cpu_env, UC_TB_BUDGET_OFS(budget));
    tcg_gen_sub_i64(tcg_ctx, budget, budget, count);
    tcg_gen_st_i64(tcg_ctx, budget, tcg_ctx->cpu_env, UC_TB_BUDGET_OFS(budget));
    tcg_gen_shli_i64(tcg_ctx, block, count, 32);
    tcg_gen_ori_i64(tcg_ctx, block, block, (uint32_t)tb->pc);
    tcg_gen_st_i64(tcg_ctx, block, tcg_ctx->cpu_env,
                   UC_TB_BUDGET_OFS(cur_block));
    tcg_temp_free_i32(tcg_ctx, count32);
    tcg_temp_free_i64(tcg_ctx, block);
    tcg_temp_free_i64(tcg_ctx, count);
    tcg_temp_free_i64(tcg_ctx, budget);
}

static inline TCGLabel *gen_tb_budget_hook_begin(TCGContext *tcg_ctx)
{
    TCGLabel *skip = gen_new_label(tcg_ctx);
    TCGv_i32 live = tcg_temp_new_i32(tcg_ctx);

    tcg_gen_ld_i32(tcg_ctx, live, tcg_ctx->cpu_env,
                   UC_TB_BUDGET_OFS(hook_live));
    tcg_gen_brcondi_i32(tcg_ctx, TCG_COND_EQ, live, 0, skip);
    tcg_temp_free_i32(tcg_ctx, live);
    return skip;
}

static inline void gen_tb_budget_hook_end(TCGContext *tcg_ctx, TCGLabel *skip)
{
    TCGv_i32 decr = tcg_temp_new_i32(tcg_ctx);

    tcg_gen_ld_i32(tcg_ctx, decr, tcg_ctx->cpu_env,
                   offsetof(ArchCPU, neg.icount_decr.u32) -
                       offsetof(ArchCPU, env));
    tcg_gen_brcondi_i32(tcg_ctx, TCG_COND_LT, decr, 0, tcg_ctx->uc_budget_exit);
    tcg_temp_free_i32(tcg_ctx, decr);
    gen_set_label(tcg_ctx, skip);
}

static inline void gen_tb_budget_consume(TCGContext *tcg_ctx, bool hooked)
{
    TCGv_i64 budget = tcg_temp_new_i64(tcg_ctx);

    if (hooked) {
        TCGv_i32 live32 = tcg_temp_new_i32(tcg_ctx);
        TCGv_i64 mask = tcg_temp_new_i64(tcg_ctx);

        tcg_gen_ld_i32(tcg_ctx, live32, tcg_ctx->cpu_env,
                       UC_TB_BUDGET_OFS(hook_live));
        tcg_gen_setcondi_i32(tcg_ctx, TCG_COND_NE, live32, live32, 0);
        tcg_gen_extu_i32_i64(tcg_ctx, mask, live32);
        tcg_gen_neg_i64(tcg_ctx, mask, mask);
        tcg_gen_ld_i64(tcg_ctx, budget, tcg_ctx->cpu_env,
                       UC_TB_BUDGET_OFS(budget));
        tcg_gen_and_i64(tcg_ctx, budget, budget, mask);
        tcg_temp_free_i64(tcg_ctx, mask);
        tcg_temp_free_i32(tcg_ctx, live32);
    } else {
        tcg_gen_movi_i64(tcg_ctx, budget, 0);
    }
    tcg_gen_st_i64(tcg_ctx, budget, tcg_ctx->cpu_env, UC_TB_BUDGET_OFS(budget));
    tcg_temp_free_i64(tcg_ctx, budget);
}

static inline void gen_tb_end(TCGContext *tcg_ctx, TranslationBlock *tb, int num_insns)
{
    if (tcg_ctx->delay_slot_flag != NULL){
        tcg_temp_free_i32(tcg_ctx, tcg_ctx->delay_slot_flag);
    }
    tcg_ctx->delay_slot_flag = NULL;
    if (tb_cflags(tb) & CF_USE_ICOUNT) {
        /* Update the num_insn immediate parameter now that we know
         * the actual insn count.  */
        tcg_set_insn_param(tcg_ctx->icount_start_insn, 1, num_insns);
    }

    if (tcg_ctx->uc_budget_exit) {
        gen_set_label(tcg_ctx, tcg_ctx->uc_budget_exit);
        tcg_ctx->uc_budget_exit = NULL;
    }
    tcg_gen_exit_tb(tcg_ctx, tb, TB_EXIT_REQUESTED);
}

#endif
