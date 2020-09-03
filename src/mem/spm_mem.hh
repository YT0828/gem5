#ifndef __SCRATCHPAD_MEMORY_HH__
#define __SCRATCHPAD_MEMORY_HH__

#include <deque>

#include "base/statistics.hh"
#include "mem/port.hh"
#include "mem/simple_mem.hh"
#include "params/ScratchpadMemory.hh"
#include "params/SimpleMemory.hh"

class System;

/**
 * This definition of scratchpad just adds stats to output
 *
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 */
class ScratchpadMemory : public SimpleMemory
{
  private:

    const Tick latency_read;
    /**
     * Latency if is a write request. If it is a read request,
     * latency from SimpleMemory is used (see implementation)
     */
    const Tick latency_write;

    /**
     * Fudge factor added to the write latency.
     */
    const Tick latency_write_var;



    /**
     * Fudge factor added to the write latency.
     */
    const double energy_read;

    /**
     * Fudge factor added to the write latency.
     */
    const double energy_write;

    /**
     * Fudge factor added to the write latency.
     */
    const double energy_overhead;

    /*
     * Command energies
    */
    Stats::Formula readEnergy;
    Stats::Formula writeEnergy;
    Stats::Formula overheadEnergy;
    Stats::Formula averageEnergy;
    Stats::Formula totalEnergy;

  public:

    ScratchpadMemory(const ScratchpadMemoryParams *p);

    void regStats();

  protected:

    Tick recvAtomic(PacketPtr pkt);

    bool recvTimingReq(PacketPtr pkt);

    Tick getWriteLatency() const;
    Tick getReadLatency() const;
};

#endif //__SCRATCHPAD_MEMORY_HH__
