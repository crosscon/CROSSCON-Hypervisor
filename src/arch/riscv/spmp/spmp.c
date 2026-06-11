#include <arch/spmp.h>

#include <mem.h>
#include <arch/csrs.h>
#include <cpu.h>
#include <vm.h>
#include <bit.h>
#include <arch/instructions.h>

size_t SPMP_NUM_ENTRIES  = 0;
size_t VSPMP_NUM_ENTRIES = 0;

static inline void spmp_write_entry(mpid_t i, struct spmp_entry* entry)
{
    csrs_siselect_write(SPMP_SISELECT_BASE_ADDR + (unsigned long)i);
    csrs_sireg_write(entry->addr);
    csrs_sireg2_write((unsigned long)entry->cfg.raw);
}

static inline bool spmp_reg_is_napot(struct mp_region* mem)
{
    return ((mem->size & (mem->size - 1)) == 0) &&
           (((mem->size - 1) & mem->base) == 0);
}

static inline void spmp_fence(bool guest_entry)
{
    if (guest_entry) {
        hfence_gvma();
    } else {
        sfence_vma();
    }
}

static inline void spmp_set_entry(struct spmp* spmp, mpid_t i,
                                  struct mp_region* mem, bool locked)
{
    UNUSED_ARG(locked);
    spmp_cfg_t cfg = mem->mem_flags;
    //cfg.l = locked ? 1 : 0;
    cfg.l=0;

    if (spmp_reg_is_napot(mem)) {
        unsigned long addr = (mem->base >> 2) | ((mem->size - 1) >> 3);
        cfg.a = (mem->size == 4) ? SPMPCFG_A_NA4 : SPMPCFG_A_NAPOT;
        spmp->entry[i].cfg  = cfg;
        spmp->entry[i].addr = addr;

        if (spmp->active) {
            spmp_write_entry(i, &spmp->entry[i]);
        }
    } else {
        /* TOR: entry[i-1] is the base (OFF), entry[i] is the top. */
        unsigned long addr_base = mem->base >> 2;
        unsigned long addr_top  = (mem->base + mem->size) >> 2;
        cfg.a = SPMPCFG_A_TOR;

        spmp->entry[i].cfg       = cfg;
        spmp->entry[i].addr      = addr_top;
        spmp->entry[i - 1].cfg.a = SPMPCFG_A_OFF;
        spmp->entry[i - 1].addr  = addr_base;

        if (spmp->active) {
            spmp_write_entry(i,     &spmp->entry[i]);
            spmp_write_entry(i - 1, &spmp->entry[i - 1]);
        }
    }

    if (spmp->active) {
        spmp_fence(cfg.u != 0);
    }
}

static void spmp_clear_entry(struct spmp* spmp, mpid_t mpid)
{
    spmp_cfg_t cfg = spmp->entry[mpid].cfg;
    bool guest_entry = (cfg.u != 0);

    if ((spmp->entry[mpid].cfg.a == SPMPCFG_A_TOR) && (mpid > 0)) {
        spmp->entry[mpid - 1].cfg.a = SPMPCFG_A_OFF;
        spmp->entry[mpid - 1].addr  = 0;
        if (spmp->active) {
            spmp_write_entry(mpid - 1, &spmp->entry[mpid - 1]);
        }
    }

    spmp->entry[mpid].cfg.a = SPMPCFG_A_OFF;
    spmp->entry[mpid].addr  = 0;
    spmp->spmpen &= ~(1ULL << (unsigned)mpid);

    if (spmp->active) {
        spmp_write_entry(mpid, &spmp->entry[mpid]);
        spmp_fence(guest_entry);
    }
}

static struct spmp* spmp_get_local(struct addr_space* as)
{
    if (as == &cpu()->as) {
        return &cpu()->arch.spmp;
    }

    list_foreach (cpu()->vcpu_lst, node_t, node) {
        struct vcpu* vcpu = CONTAINER_OF(struct vcpu, list_node, node);
        if (as == &vcpu->vm->as) {
            return &vcpu->arch.spmp;
        }
    }

    return NULL;
}

static mpid_t spmp_allocate_entry(struct spmp* spmp, struct mp_region* mem,
                                  bool locked)
{
    UNUSED_ARG(spmp);

    uint16_t count_min = (uint16_t)~0U;
    mpid_t   mpid      = INVALID_MPID;
    bool     napot     = spmp_reg_is_napot(mem);

    for (size_t i = 0; i < SPMP_NUM_ENTRIES; i++) {
        bool last        = (i == (SPMP_NUM_ENTRIES - 1));
        bool cur_locked  = bitmap_get(cpu()->arch.spmp_mngmnt.entry_locked, i);
        bool next_locked = last ? true
                                : bitmap_get(cpu()->arch.spmp_mngmnt.entry_locked, i + 1);

        if (locked) {
            bool cur_free = (cpu()->arch.spmp_mngmnt.entry_allocation_count[i] == 0);
            if (cur_free) {
                if (napot) { mpid = i; break; }
                else if (!last) {
                    bool next_free =
                        (cpu()->arch.spmp_mngmnt.entry_allocation_count[i + 1] == 0);
                    if (next_free) { mpid = i; break; }
                }
            }
        } else if (!cur_locked) {
            uint16_t count = cpu()->arch.spmp_mngmnt.entry_allocation_count[i];
            if (!napot) {
                if (next_locked) continue;
                count = (uint16_t)(count +
                    cpu()->arch.spmp_mngmnt.entry_allocation_count[i + 1]);
            }
            if (count < count_min) { count_min = count; mpid = i; }
        }
    }

    if (mpid != INVALID_MPID) {
        cpu()->arch.spmp_mngmnt.entry_allocation_count[mpid]++;
        if (locked) bitmap_set(cpu()->arch.spmp_mngmnt.entry_locked, mpid);
        spmp->allocated_entries = bit64_set(spmp->allocated_entries, mpid);

        if (!napot) {
            cpu()->arch.spmp_mngmnt.entry_allocation_count[mpid + 1]++;
            if (locked) bitmap_set(cpu()->arch.spmp_mngmnt.entry_locked, mpid + 1);
            spmp->allocated_entries = bit64_set(spmp->allocated_entries, mpid + 1);
            mpid += 1; /* TOR: return top-entry index */
        }
    }

    return mpid;
}

static void spmp_free_entry(struct spmp* spmp, mpid_t mpid)
{
    cpu()->arch.spmp_mngmnt.entry_allocation_count[mpid]--;
    bitmap_clear(cpu()->arch.spmp_mngmnt.entry_locked, mpid);
    spmp->allocated_entries = bit64_clear(spmp->allocated_entries, mpid);

    if (spmp->entry[mpid].cfg.a == SPMPCFG_A_TOR) {
        cpu()->arch.spmp_mngmnt.entry_allocation_count[mpid - 1]--;
        bitmap_clear(cpu()->arch.spmp_mngmnt.entry_locked, mpid - 1);
        spmp->allocated_entries = bit64_clear(spmp->allocated_entries, mpid - 1);
    }
}

static mpid_t spmp_find_region_entry(struct spmp* spmp, struct mp_region* mem)
{
    for (size_t i = 0; i < SPMP_NUM_ENTRIES; i++) {
        if (spmp->entry[i].cfg.a == SPMPCFG_A_OFF) continue;

        vaddr_t base;
        if (spmp->entry[i].cfg.a == SPMPCFG_A_TOR) {
            base = (i == 0) ? 0UL : (spmp->entry[i - 1].addr << 2);
        } else if (spmp->entry[i].cfg.a == SPMPCFG_A_NA4) {
            base = (spmp->entry[i].addr << 2) & ~(vaddr_t)(4 - 1);
        } else { /* NAPOT */
            size_t reg_size = (1UL << (bit_ffs(~spmp->entry[i].addr) + 3));
            base = (spmp->entry[i].addr << 2) & ~(reg_size - 1);
        }

        if (base == mem->base) return (mpid_t)i;
    }
    return INVALID_MPID;
}


static bool spmp_perms_valid(spmp_cfg_t* cfg)
{
    // if (cfg->w && !cfg->r) return false;        
    // if (cfg->s && !cfg->r && !cfg->w && !cfg->x) return false; 
    // if (cfg->s && cfg->r && cfg->w && cfg->x) cfg->x = 0;
    // return true;

   return !((cfg->w && !cfg->r) || cfg->s);
}


void spmp_init(struct spmp* spmp)
{
    spmp->spmpen            = 0;
    spmp->allocated_entries = 0;
    spmp->first_entry       = INVALID_MPID;
    spmp->resident          = false;
    spmp->active            = false;

    for (size_t i = 0; i < SPMP_MAX_NUM_ENTRIES; i++) {
        spmp->entry[i].cfg.raw = 0;
        spmp->entry[i].addr    = 0;
    }
}

void spmp_restore(struct spmp* spmp)
{
    if ((!spmp->resident) && (spmp->first_entry != INVALID_MPID)) {
        uint64_t allocated = spmp->allocated_entries >> spmp->first_entry;
        for (size_t i = spmp->first_entry;
             (i < SPMP_NUM_ENTRIES) && (allocated != 0); i++) {
            if ((allocated & 1) != 0) {
                spmp_write_entry(i, &spmp->entry[i]);
                if (cpu()->arch.spmp_mngmnt.resident[i] != NULL) {
                    *(cpu()->arch.spmp_mngmnt.resident[i]) = false;
                }
                cpu()->arch.spmp_mngmnt.resident[i] = &spmp->resident;
            }
            allocated >>= 1;
        }
    }

    hfence_gvma();

    /* hspmpen activates SPMP entries for VS/VU accesses when V=1 (Sshspmpen) */
    csrs_hspmpen_write(spmp->spmpen);

    spmp->resident = true;
    spmp->active   = true;

    if (cpu()->arch.spmp_mngmnt.active_guest_spmp != NULL) {
        cpu()->arch.spmp_mngmnt.active_guest_spmp->active = false;
    }
    cpu()->arch.spmp_mngmnt.active_guest_spmp = spmp;
}

bool mpu_map(struct addr_space* as, struct mp_region* mem, bool locked)
{
    struct spmp* spmp = spmp_get_local(as);
    if (spmp == NULL || mem->size == 0 || !spmp_perms_valid(&mem->mem_flags)) {
        return false;
    }
    mem->mem_flags.u = (as->type == AS_VM) ? 1 : 0;

    mpid_t mpid = spmp_allocate_entry(spmp, mem, locked);
    if (mpid == INVALID_MPID) return false;

    spmp_set_entry(spmp, mpid, mem, locked);

    spmp->spmpen |= (1ULL << (unsigned)mpid);

    if (as->type == AS_HYP) {
        csrs_spmpen_set(1ULL << (unsigned)mpid);
    } else if (spmp->active) {
        csrs_hspmpen_set(1ULL << (unsigned)mpid);
    }

    spmp->first_entry = (mpid_t)bit64_ffs(spmp->allocated_entries);
    return true;
}

bool mpu_unmap(struct addr_space* as, struct mp_region* mem)
{
    struct spmp* spmp = spmp_get_local(as);
    if (spmp == NULL) return false;

    mpid_t mpid = spmp_find_region_entry(spmp, mem);
    if (mpid == INVALID_MPID) return false;

    spmp_free_entry(spmp, mpid);
    spmp_clear_entry(spmp, mpid);

    spmp->spmpen &= ~(1ULL << (unsigned)mpid);
    if (as->type == AS_HYP) {
        csrs_spmpen_clear(1ULL << (unsigned)mpid);
    } else if (spmp->active) {
        csrs_hspmpen_clear(1ULL << (unsigned)mpid);
    }
              
    ssize_t first = bit64_ffs(spmp->allocated_entries);
    spmp->first_entry = (first >= 0) ? (mpid_t)first : INVALID_MPID;
    return true;
}

bool mpu_update(struct addr_space* as, struct mp_region* mpr)
{
    bool failed = true;

    if (mpu_unmap(as, mpr)) {
        failed = !mpu_map(as, mpr, false);
    }

    return !failed;
}

bool mpu_perms_compatible(unsigned long perms1, unsigned long perms2)
{
    uint16_t mask = (uint16_t)(SPMPCFG_S_BIT | SPMPCFG_R_BIT | SPMPCFG_W_BIT | SPMPCFG_X_BIT);
    return (perms1 & mask) == (perms2 & mask);
}

static inline ssize_t my_ffs(uint64_t word)
{
    ssize_t pos = 0;
    uint64_t mask = UINT64_C(1);
    while (mask != 0U) {
        if ((mask & word) != 0U) break;
        mask <<= 1U;
        pos++;
    }
    return (mask != 0U) ? pos : (ssize_t)~0L;
}

void mpu_init(void)
{
    if (cpu_is_master()) {
        /*
         * hspmpdeleg.pmpnum: write 32 and read back to discover how many
         * PMP resources the hardware delegates to SPMP (WARL, 8-bit field).
         */
        csrs_hspmpdeleg_write(48);
        SPMP_NUM_ENTRIES = csrs_hspmpdeleg_read() & HSPMPDELEG_PMPNUM_MSK;

        /*
         * Probe vSPMP count: write all-ones to vspmpen; hardware clamps
         * to the implemented count (WARL).  First zero bit = entry count.
         */
        csrs_vspmpen_write((uint64_t)(-1));
        ssize_t tmp = my_ffs(~csrs_vspmpen_read());
        VSPMP_NUM_ENTRIES = (tmp >= 0) ? (size_t)tmp : 0;
        csrs_vspmpen_write(0);
    }

    cpu_sync_barrier(&cpu_glb_sync);

    /* Per-hart management state */
    for (size_t i = 0; i < SPMP_MAX_NUM_ENTRIES; i++) {
        bitmap_clear(cpu()->arch.spmp_mngmnt.entry_locked, i);
        cpu()->arch.spmp_mngmnt.entry_allocation_count[i] = 0;
        cpu()->arch.spmp_mngmnt.resident[i] = NULL;
    }
    cpu()->arch.spmp_mngmnt.active_guest_spmp = NULL;

    spmp_init(&cpu()->arch.spmp);
    spmp_set_active(&cpu()->arch.spmp, true);
}

void spmp_enable_hyp_whitelist_mode(void)
{
    mpid_t last_entry = (mpid_t)SPMP_NUM_ENTRIES;
    struct spmp* spmp = &cpu()->arch.spmp;
    spmp->entry[last_entry].cfg.a = SPMPCFG_A_NAPOT;
    spmp->entry[last_entry].addr  = (paddr_t)-1;
    spmp_write_entry(last_entry, &spmp->entry[last_entry]);
}

void mpu_enable(void)
{
    //spmp_clear_entry(&cpu()->arch.spmp, SPMP_NUM_ENTRIES-1);
    spmp_enable_hyp_whitelist_mode();
}