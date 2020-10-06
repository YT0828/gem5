/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Declaration of a Modified Re-Reference Interval
 * Prediction replacement policy to implement SPM migration.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_BRRIP_SPM_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_BRRIP_SPM_HH__

#include "base/sat_counter.hh"
#include "mem/cache/replacement_policies/base.hh"

#define PR_threshold 3
#define PR_bit 2
struct BRRIPSPMParams;

class BRRIPSPM : public BaseReplacementPolicy
{
  protected:
    /** BRRIP-specific implementation of replacement data. */
    struct BRRIPSPMeplData : ReplacementData
    {
        /**
         * Re-Reference Interval Prediction Value.
         * Some values have specific names (according to the paper):
         * 0 -> near-immediate re-rereference interval
         * max_RRPV-1 -> long re-rereference interval
         * max_RRPV -> distant re-rereference interval
         */
        SatCounter rrpv;
        /**
         * Pattern Record bit
         * Over threshold means it is a write-intense line.
         */
        SatCounter pr;
        /** Whether the entry is valid. */
        bool valid;

        /**
         * Default constructor. Invalidate data.
         */
        BRRIPSPMeplData(const int num_bits)
            : rrpv(num_bits), pr(PR_bit), valid(false)
        {
        }
    };

    /**
     * Number of RRPV bits. An entry that saturates its RRPV has the longest
     * possible re-reference interval, that is, it is likely not to be used
     * in the near future, and is among the best eviction candidates.
     * A maximum RRPV of 1 implies in a NRU.
     */
    const unsigned numRRPVBits;

    /**
     * The hit priority (HP) policy replaces entries that do not receive cache
     * hits over any cache entry that receives a hit, while the frequency
     * priority (FP) policy replaces infrequently re-referenced entries.
     */
    const bool hitPriority;

    /**
     * Bimodal throtle parameter. Value in the range [0,100] used to decide
     * if a new entry is inserted with long or distant re-reference.
     */
    const unsigned btp;

  public:
    /** Convenience typedef. */
    typedef BRRIPSPMParams Params;

    /**
     * Construct and initiliaze this replacement policy.
     */
    BRRIPSPM(const Params *p);

    /**
     * Destructor.
     */
    ~BRRIPSPM() {}

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Set RRPV as the the most distant re-reference.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                              const override;

    /**
     * Touch an entry to update its replacement data.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(
        const std::shared_ptr<ReplacementData>& replacement_data,
        int access_type = -1) const override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Set RRPV according to the insertion policy used.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim using pr and rrpv.
     *
     * @param cands Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const
    ReplacementCandidates& candidates) const override;

    ReplaceableEntry* getRRPVVictim(const
    ReplacementCandidates& candidates) const;
    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_BRRIP_RP_HH__
