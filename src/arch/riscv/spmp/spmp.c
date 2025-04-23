#include <arch/spmp.h>

#include <mem.h>
#include <arch/csrs.h>
#include <cpu.h>
#include <vm.h>

// #define SPMPCFG_PERMS_MASK (SPMPCFG_W_BIT | SPMPCFG_X_BIT | SPMPCFG_R_BIT | SPMPCFG_S_BIT)

// This assumes all harts have the same number of SPMP entries
static size_t SPMP_NUM_ENTRIES;



static void spmp_set_icfg(mpid_t i, spmp_cfg_t cfg)
{
    size_t cfgi = i / 4;
    size_t cfgi_off = (i % 4) * 8;
    if (RV64 && ((cfgi & 1) != 0)) {
        cfgi -= 1;
        cfgi_off += 32;
    }
    unsigned long mask = SPMPCFG_ICFG_MSK << cfgi_off;
    unsigned long val = ((unsigned long)cfg.raw) << cfgi_off;

    switch (cfgi) {
        case 0:
            csrs_spmpcfg0_clear(mask);
            csrs_spmpcfg0_set(val);
            break;
        case 1:
            csrs_spmpcfg1_clear(mask);
            csrs_spmpcfg1_set(val);
            break;
        case 2:
            csrs_spmpcfg2_clear(mask);
            csrs_spmpcfg2_set(val);
            break;
        case 3:
            csrs_spmpcfg3_clear(mask);
            csrs_spmpcfg3_set(val);
            break;
        case 4:
            csrs_spmpcfg4_clear(mask);
            csrs_spmpcfg4_set(val);
            break;
        case 5:
            csrs_spmpcfg5_clear(mask);
            csrs_spmpcfg5_set(val);
            break;
        case 6:
            csrs_spmpcfg6_clear(mask);
            csrs_spmpcfg6_set(val);
            break;
        case 7:
            csrs_spmpcfg7_clear(mask);
            csrs_spmpcfg7_set(val);
            break;
        case 8:
            csrs_spmpcfg8_clear(mask);
            csrs_spmpcfg8_set(val);
            break;
        case 9:
            csrs_spmpcfg9_clear(mask);
            csrs_spmpcfg9_set(val);
            break;
        case 10:
            csrs_spmpcfg10_clear(mask);
            csrs_spmpcfg10_set(val);
            break;
        case 11:
            csrs_spmpcfg11_clear(mask);
            csrs_spmpcfg11_set(val);
            break;
        case 12:
            csrs_spmpcfg12_clear(mask);
            csrs_spmpcfg12_set(val);
            break;
        case 13:
            csrs_spmpcfg13_clear(mask);
            csrs_spmpcfg13_set(val);
            break;
        case 14:
            csrs_spmpcfg14_clear(mask);
            csrs_spmpcfg14_set(val);
            break;
        case 15:
            csrs_spmpcfg15_clear(mask);
            csrs_spmpcfg15_set(val);
            break;
        default:
            ERROR("undefined spmp cfgi");
            break;
    }
}

static void spmp_set_addr(mpid_t i, paddr_t addr)
{
    switch (i) {
        case 0:
            csrs_spmpaddr0_write(addr);
            break;
        case 1:
            csrs_spmpaddr1_write(addr);
            break;
        case 2:
            csrs_spmpaddr2_write(addr);
            break;
        case 3:
            csrs_spmpaddr3_write(addr);
            break;
        case 4:
            csrs_spmpaddr4_write(addr);
            break;
        case 5:
            csrs_spmpaddr5_write(addr);
            break;
        case 6:
            csrs_spmpaddr6_write(addr);
            break;
        case 7:
            csrs_spmpaddr7_write(addr);
            break;
        case 8:
            csrs_spmpaddr8_write(addr);
            break;
        case 9:
            csrs_spmpaddr9_write(addr);
            break;
        case 10:
            csrs_spmpaddr10_write(addr);
            break;
        case 11:
            csrs_spmpaddr11_write(addr);
            break;
        case 12:
            csrs_spmpaddr12_write(addr);
            break;
        case 13:
            csrs_spmpaddr13_write(addr);
            break;
        case 14:
            csrs_spmpaddr14_write(addr);
            break;
        case 15:
            csrs_spmpaddr15_write(addr);
            break;
        case 16:
            csrs_spmpaddr16_write(addr);
            break;
        case 17:
            csrs_spmpaddr17_write(addr);
            break;
        case 18:
            csrs_spmpaddr18_write(addr);
            break;
        case 19:
            csrs_spmpaddr19_write(addr);
            break;
        case 20:
            csrs_spmpaddr20_write(addr);
            break;
        case 21:
            csrs_spmpaddr21_write(addr);
            break;
        case 22:
            csrs_spmpaddr22_write(addr);
            break;
        case 23:
            csrs_spmpaddr23_write(addr);
            break;
        case 24:
            csrs_spmpaddr24_write(addr);
            break;
        case 25:
            csrs_spmpaddr25_write(addr);
            break;
        case 26:
            csrs_spmpaddr26_write(addr);
            break;
        case 27:
            csrs_spmpaddr27_write(addr);
            break;
        case 28:
            csrs_spmpaddr28_write(addr);
            break;
        case 29:
            csrs_spmpaddr29_write(addr);
            break;
        case 30:
            csrs_spmpaddr30_write(addr);
            break;
        case 31:
            csrs_spmpaddr31_write(addr);
            break;
        case 32:
            csrs_spmpaddr32_write(addr);
            break;
        case 33:
            csrs_spmpaddr33_write(addr);
            break;
        case 34:
            csrs_spmpaddr34_write(addr);
            break;
        case 35:
            csrs_spmpaddr35_write(addr);
            break;
        case 36:
            csrs_spmpaddr36_write(addr);
            break;
        case 37:
            csrs_spmpaddr37_write(addr);
            break;
        case 38:
            csrs_spmpaddr38_write(addr);
            break;
        case 39:
            csrs_spmpaddr39_write(addr);
            break;
        case 40:
            csrs_spmpaddr40_write(addr);
            break;
        case 41:
            csrs_spmpaddr41_write(addr);
            break;
        case 42:
            csrs_spmpaddr42_write(addr);
            break;
        case 43:
            csrs_spmpaddr43_write(addr);
            break;
        case 44:
            csrs_spmpaddr44_write(addr);
            break;
        case 45:
            csrs_spmpaddr45_write(addr);
            break;
        case 46:
            csrs_spmpaddr46_write(addr);
            break;
        case 47:
            csrs_spmpaddr47_write(addr);
            break;
        case 48:
            csrs_spmpaddr48_write(addr);
            break;
        case 49:
            csrs_spmpaddr49_write(addr);
            break;
        case 50:
            csrs_spmpaddr50_write(addr);
            break;
        case 51:
            csrs_spmpaddr51_write(addr);
            break;
        case 52:
            csrs_spmpaddr52_write(addr);
            break;
        case 53:
            csrs_spmpaddr53_write(addr);
            break;
        case 54:
            csrs_spmpaddr54_write(addr);
            break;
        case 55:
            csrs_spmpaddr55_write(addr);
            break;
        case 56:
            csrs_spmpaddr56_write(addr);
            break;
        case 57:
            csrs_spmpaddr57_write(addr);
            break;
        case 58:
            csrs_spmpaddr58_write(addr);
            break;
        case 59:
            csrs_spmpaddr59_write(addr);
            break;
        case 60:
            csrs_spmpaddr60_write(addr);
            break;
        case 61:
            csrs_spmpaddr61_write(addr);
            break;
        case 62:
            csrs_spmpaddr62_write(addr);
            break;
        case 63:
            csrs_spmpaddr63_write(addr);
            break;
        default:
            ERROR("spmp no entry to set");
            break;
    }
}

static inline bool spmp_reg_is_napot(struct mp_region* mem)
{
    return ((mem->size & (mem->size - 1)) == 0) && (((mem->size - 1) & mem->base) == 0);
}

static struct spmp* spmp_get_local(struct addr_space* as)
{
    struct spmp* spmp = NULL;

    if (as == &cpu()->as) {
        spmp = &cpu()->arch.spmp_hyp;
    } else {
        list_foreach (cpu()->vcpu_lst, struct vcpu, vcpu) {
            if (as == &vcpu->vm->as) {
                spmp = &vcpu->arch.spmp;
                break;
            }
        }
    }

    return spmp;
}

static void spmp_lock_entry(struct spmp* spmp, mpid_t mpid)
{
    spmp->locked = bit64_set(spmp->locked, mpid);
}

static bool spmp_entry_locked(struct spmp* spmp, mpid_t mpid)
{
    return !!bit64_get(spmp->locked, mpid);
}

static void spmp_set_entry(struct spmp* spmp, mpid_t i, struct mp_region* mem)
{
    spmp_cfg_t cfg = mem->mem_flags;

    if (spmp_reg_is_napot(mem)) {
        unsigned long addr = (mem->base >> 2) | ((mem->size - 1) >> 3);
        if (mem->size == 4) {
            cfg.a = SPMPCFG_A_NA4;
        } else {
            cfg.a = SPMPCFG_A_NAPOT;
        }
        spmp->entry[i].cfg = cfg;
        spmp->entry[i].addr = addr;
        if (spmp->active) {
            spmp_set_addr(i, addr);
            spmp_set_icfg(i, cfg);
        }
    } else {
        cfg.a = SPMPCFG_A_TOR;
        spmp->entry[i - 1].cfg = (spmp_cfg_t){ .a = SPMPCFG_A_OFF };
        spmp->entry[i - 1].addr = mem->base >> 2;
        spmp->entry[i].addr = (mem->base + mem->size) >> 2;
        spmp->entry[i].cfg = cfg;
        if (spmp->active) {
            spmp_set_icfg(i - 1, spmp->entry[i - 1].cfg);
            spmp_set_addr(i - 1, spmp->entry[i - 1].addr);
            spmp_set_addr(i, spmp->entry[i].addr);
            spmp_set_icfg(i, spmp->entry[i].cfg);
        }
    }
}

static void spmp_clear_entry(struct spmp* spmp, mpid_t i)
{
    spmp->entry[i].cfg = (spmp_cfg_t){ .a = SPMPCFG_A_OFF };
    if (spmp->active) {
        spmp_set_icfg(i, spmp->entry[i].cfg);
    }
}

static mpid_t spmp_alloc_entry(struct spmp* spmp, struct mp_region* mem)
{
    mpid_t mpid = INVALID_MPID;
    size_t num_entries;
    if (spmp_reg_is_napot(mem)) {
        num_entries = 1;
    } else {
        num_entries = 2;
    }
    uint64_t free_entries = cpu()->arch.spmp_hyp.alloc_entries | spmp->alloc_entries;
    ssize_t entry =
        bitmap_find_consec((bitmap_t*)&free_entries, SPMP_NUM_ENTRIES, 0, num_entries, false);
    if (entry >= 0) {
        bitmap_set_consecutive((bitmap_t*)&spmp->alloc_entries, (size_t)entry, num_entries);
        mpid = (mpid_t)entry + num_entries - 1;
    }
    return mpid;
}

static void spmp_free_entry(struct spmp* spmp, mpid_t mpid)
{
    spmp->alloc_entries = bit64_clear(spmp->alloc_entries, mpid);

    if (spmp->entry[mpid].cfg.a == SPMPCFG_A_TOR) {
        spmp->alloc_entries = bit64_clear(spmp->alloc_entries, mpid - 1);
    }
}

static void spmp_entry_get_region(struct spmp* spmp, mpid_t mpid, struct mp_region* reg)
{
    unsigned long addr = spmp->entry[mpid].addr;
    reg->mem_flags = spmp->entry[mpid].cfg;
    if (spmp_reg_is_napot(reg)) {
        if (reg->mem_flags.a == SPMPCFG_A_NA4) {
            reg->size = 4;
        } else {
            reg->size = (1UL << (bit_ffs(~addr) + 3));
        }
        reg->base = (addr << 2) & (reg->size - 1);
    } else {
        reg->base = spmp->entry[mpid - 1].addr << 2;
        reg->size = (addr << 2) - reg->base;
    }
    reg->as_sec = SEC_UNKNOWN;
}

static int spmp_node_cmp(void* cookie, node_t* _n1, node_t* _n2)
{
    struct spmp* spmp = (struct spmp*)cookie;
    struct spmpe_node* n1 = (struct spmpe_node*)_n1;
    struct spmpe_node* n2 = (struct spmpe_node*)_n2;
    struct mp_region r1;
    struct mp_region r2;
    spmp_entry_get_region(spmp, n1->mpid, &r1);
    spmp_entry_get_region(spmp, n2->mpid, &r2);
    if (r1.base > r2.base) {
        return 1;
    } else if (r1.base < r2.base) {
        return -1;
    } else {
        return 0;
    }
}

static bool spmp_add_region(struct spmp* spmp, struct mp_region* reg)
{
    bool failed = true;

    if (reg->size > 0) {
        mpid_t mpid = spmp_alloc_entry(spmp, reg);
        if (mpid != INVALID_MPID) {
            failed = false;

            spmp_set_entry(spmp, mpid, reg);

            spmp->switchmsk |= 1ULL << mpid;
            if (spmp->priv == PRIV_HYP) {
                csrs_spmpswitch_set(1ULL << mpid);
            }

            struct node_cmp spmp_list_node_cmp = {
                .cmp = spmp_node_cmp,
                .cookie = spmp,
            };

            list_insert_ordered(&spmp->order.list, (node_t*)&spmp->order.node[mpid],
                &spmp_list_node_cmp);
        }
    }

    return !failed;
}

static void spmp_remove_entry(struct spmp* spmp, mpid_t mpid)
{
    if (bit64_set(spmp->alloc_entries, mpid)) {
        spmp_clear_entry(spmp, mpid);
        spmp_free_entry(spmp, mpid);

        spmp->switchmsk &= ~(1ULL << mpid);
        if (spmp->priv == PRIV_HYP) {
            csrs_spmpswitch_clear(1ULL << mpid);
        }

        list_rm(&spmp->order.list, (node_t*)&spmp->order.node[mpid]);
    }
}

bool spmp_perms_compatible(mem_flags_t perms1, mem_flags_t perms2)
{
    uint8_t perms_mask = SPMPCFG_S_BIT | SPMPCFG_R_BIT | SPMPCFG_W_BIT | SPMPCFG_X_BIT;
    return (perms1.raw & perms_mask) == (perms2.raw & perms_mask);
}

static void spmp_coalesce_contiguous(struct spmp* spmp)
{
    while (true) {
        bool merge = false;
        mpid_t cur_mpid = INVALID_MPID;
        mpid_t prev_mpid = INVALID_MPID;
        struct mp_region prev_reg;
        struct mp_region cur_reg;
        list_foreach_tail(spmp->order.list, struct spmpe_node, cur, prev)
        {
            if (prev == NULL) {
                continue;
            }

            if (spmp_entry_locked(spmp, cur->mpid) || spmp_entry_locked(spmp, prev->mpid)) {
                continue;
            }

            spmp_entry_get_region(spmp, cur->mpid, &cur_reg);
            spmp_entry_get_region(spmp, prev->mpid, &prev_reg);

            bool contigous = mp_region_top(&prev_reg) == cur_reg.base;
            bool perms_compatible =
                spmp_perms_compatible(prev_reg.mem_flags, cur_reg.mem_flags);
            if (contigous && perms_compatible) {
                cur_mpid = cur->mpid;
                prev_mpid = prev->mpid;
                merge = true;
                break;
            }
        }

        if (merge) {
            spmp_remove_entry(spmp, cur_mpid);
            spmp_remove_entry(spmp, prev_mpid);
            struct mp_region merged_reg = {
                .base = prev_reg.base,
                .size = prev_reg.size + cur_reg.size,
                .mem_flags = cur_reg.mem_flags,
            };
            spmp_add_region(spmp, &merged_reg);
        } else {
            break;
        }
    }
}

static bool spmp_perms_valid(spmp_cfg_t* cfg)
{
    bool valid = true;

    if (cfg->w && !cfg->r) {
        valid = false;
    }

    if (cfg->s && !cfg->r && !cfg->w && !cfg->x) {
        valid = false;
    }

    // We are forcefully removing X permissions when all permissions are given to the hypervisor
    // as this permission combination is not possible. Despite the hypervisor not wanting to
    // use it, it might come up when it tries to copy mappings from the VM.
    // TODO: think of a cleaner solution
    if (cfg->s && cfg->r && cfg->w && cfg->x) {
        cfg->x = 0;
    }

    return valid;
}

bool mpu_map(struct addr_space* as, struct mp_region* mem, bool locked)
{
    bool failed = true;

    struct spmp* spmp = spmp_get_local(as);

    // We need to force the s bit according to the privilege of the spmp instance we are managing
    // However, maybe this should be done a priori.
    mem->mem_flags.s = !!(spmp->priv == PRIV_HYP);

    if (spmp_perms_valid(&mem->mem_flags)) {
        mpid_t mpid = spmp_add_region(spmp, mem);
        if (mpid != INVALID_MPID) {
            failed = false;
            if (locked) {
                spmp_lock_entry(spmp, mpid);
            }
            spmp_coalesce_contiguous(spmp);
        }
    }

    return !failed;
}

bool mpu_unmap(struct addr_space* as, struct mp_region* mem)
{
    struct mp_region target_reg = *mem;
    struct mp_region overlapped_reg;
    mpid_t overlapped_mpid;
    bool failed = false;

    struct spmp* spmp = spmp_get_local(as);

    while (target_reg.size > 0 && !failed) {
        overlapped_mpid = INVALID_MPID;
        list_foreach (spmp->order.list, struct spmpe_node, entry) {
            if (!bit64_get(spmp->alloc_entries, entry->mpid)) {
                continue;
            }

            spmp_entry_get_region(spmp, entry->mpid, &overlapped_reg);

            if (mp_region_top(&target_reg) < overlapped_reg.base) {
                continue;
            }

            if (mem_regions_overlap(&target_reg, &overlapped_reg)) {
                overlapped_mpid = entry->mpid;
                break;
            }
        }

        if (overlapped_mpid == INVALID_MPID) {
            break;
        }

        if (spmp_entry_locked(spmp, overlapped_mpid)) {
            failed = true;
            break;
        }

        vaddr_t target_top = mp_region_top(&target_reg);
        vaddr_t overlapped_top = mp_region_top(&overlapped_reg);
        size_t top_size = target_top <= overlapped_top ? overlapped_top - target_top : 0;
        size_t bottom_size = target_reg.base >= overlapped_top ? overlapped_top - target_top : 0;

        spmp_remove_entry(spmp, overlapped_mpid);
        struct mp_region tmp_reg;
        if (top_size > 0) {
            tmp_reg.base = mp_region_top(&target_reg);
            tmp_reg.size = top_size;
            tmp_reg.mem_flags.raw = overlapped_reg.mem_flags.raw;
            spmp_add_region(spmp, &tmp_reg);
        }

        if (bottom_size > 0) {
            tmp_reg.base = overlapped_reg.base;
            tmp_reg.size = bottom_size;
            tmp_reg.mem_flags.raw = overlapped_reg.mem_flags.raw;
            spmp_add_region(spmp, &tmp_reg);
        }

        target_reg.size = sat_ul_sub(target_top, overlapped_top);
        target_reg.base = overlapped_top;
    }

    if (!failed) {
        spmp_coalesce_contiguous(spmp);
    }

    return !failed && target_reg.size == 0;
}

void mpu_init()
{
    ssize_t nentries = bit64_ffs(~csrs_spmpswitch_read());
    if (nentries < 0) {
        nentries = 64;
    }

    // We count one less entry as we reserve the last entry as a "block anything" entry for
    // the hypervisor, so that other entries can be seen as whitelist
    SPMP_NUM_ENTRIES = (size_t)nentries - 1;

    for (mpid_t i = 0; i < (mpid_t)nentries; i += 1) {
        spmp_set_icfg(i, (spmp_cfg_t){ .a = SPMPCFG_A_OFF });
    }

    // TODO: check for granularity and expand it to the mapping functions
}

void spmp_enable_hyp_whitelist_mode()
{
    // As the spmp does not provide a whitelist mode, we set the last entry to match the whole
    // address space but with no permissions.
    mpid_t last_entry = (mpid_t)SPMP_NUM_ENTRIES;
    spmp_set_addr(last_entry, (paddr_t)-1);
    spmp_set_icfg(last_entry, (spmp_cfg_t){ .a = SPMPCFG_A_NAPOT });
    // spmp_lock_entry(last_entry);
}

void spmp_init(struct spmp* spmp, priv_t priv)
{
    spmp->alloc_entries = 0;
    spmp->priv = priv;

    list_init(&spmp->order.list);

    for (mpid_t i = 0; i < SPMP_MAX_ENTRIES; i++) {
        spmp->order.node[i].mpid = i;
    }
}

void spmp_restore(struct spmp* spmp)
{
    list_foreach (spmp->order.list, struct spmpe_node, entry) {
        mpid_t i = entry->mpid;
        spmp_set_addr(i, spmp->entry[i].addr);
        spmp_set_icfg(i, spmp->entry[i].cfg);
        if (spmp->entry[i].cfg.a == SPMPCFG_A_TOR) {
            spmp_set_icfg(i - 1, spmp->entry[i - 1].cfg);
            spmp_set_addr(i - 1, spmp->entry[i - 1].addr);
        }
    }
}

void spmp_enable(void)
{

}

bool spmp_update(struct addr_space* as, struct mp_region* mpr)
{
    UNUSED_ARG(as);
    UNUSED_ARG(mpr);

    //TODO:CROSSCON - Need to implement this
    ERROR("spmp_update not implemented");

    return false;
}
