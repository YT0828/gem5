#include "mem/spm_mem.hh"

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/Drain.hh"
#include "sim/system.hh"

using namespace std;

void
ScratchpadMemory::regStats()
{
    using namespace Stats;

    AbstractMemory::regStats();

    System *system = (AbstractMemory::system());

    readEnergy
        .name(name() + ".energy_read")
        .desc("Total energy reading (pJ)")
        .precision(0)
        .prereq(AbstractMemory::stats.numReads)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        readEnergy.subname(i, system->getMasterName(i));
    }

    writeEnergy
        .name(name() + ".energy_write")
        .desc("Total energy writting (pJ)")
        .precision(0)
        .prereq(AbstractMemory::stats.numWrites)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        writeEnergy.subname(i, system->getMasterName(i));
    }

    overheadEnergy
        .name(name() + ".energy_overhead")
        .desc("Other energy (pJ)")
        .precision(0)
        .prereq(AbstractMemory::stats.numOther)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        overheadEnergy.subname(i, system->getMasterName(i));
    }

    totalEnergy
        .name(name() + ".energy_total")
        .desc("Total energy (pJ)")
        .precision(0)
        .prereq(overheadEnergy)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        totalEnergy.subname(i, system->getMasterName(i));
    }

    averageEnergy
        .name(name() + ".energy_average")
        .desc("Average energy (pJ)")
        .precision(0)
        .prereq(totalEnergy)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        averageEnergy.subname(i, system->getMasterName(i));
    }

    // Trying to implement a energy model...
    readEnergy = AbstractMemory::stats.numReads * energy_read;
    writeEnergy = AbstractMemory::stats.numWrites * energy_write;
    overheadEnergy = AbstractMemory::stats.numOther * energy_overhead;
    totalEnergy = readEnergy + writeEnergy + overheadEnergy;
    averageEnergy = (energy_overhead==0) ? totalEnergy / 2 : totalEnergy / 3 ;

}

ScratchpadMemory::ScratchpadMemory(const ScratchpadMemoryParams* p) :
    SimpleMemory(p),
    latency_read(p->latency_read),
    latency_write(p->latency_write), latency_write_var(p->latency_write_var),
    energy_read(p->energy_read), energy_write(p->energy_write),
    energy_overhead(p->energy_overhead)
{
}
Tick
ScratchpadMemory::getWriteLatency() const
{
    return latency_write +
        (latency_write_var ? random_mt.random<Tick>(0, latency_write_var) : 0);
}

Tick
ScratchpadMemory::getReadLatency() const
{
    return latency_read +
        (latency_write_var ? random_mt.random<Tick>(0, latency_write_var) : 0);
}

Tick
ScratchpadMemory::recvAtomic(PacketPtr pkt)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    access(pkt);
    return getLatency();
}

bool
ScratchpadMemory::recvTimingReq(PacketPtr pkt)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller, "
             "saw %s to %#llx\n", pkt->cmdString(), pkt->getAddr());

    // we should never get a new request after committing to retry the
    // current one, the bus violates the rule as it simply sends a
    // retry to the next one waiting on the retry list, so simply
    // ignore it
    if (retryReq)
        return false;

    // if we are busy with a read or write, remember that we have to
    // retry
    if (isBusy) {
        retryReq = true;
        return false;
    }


    // technically the packet only reaches us after the header delay,
    // and since this is a memory controller we also need to
    // deserialise the payload before performing any write operation
    Tick receive_delay = pkt->headerDelay + pkt->payloadDelay;
    pkt->headerDelay = pkt->payloadDelay = 0;

    // update the release time according to the bandwidth limit, and
    // do so with respect to the time it takes to finish this request
    // rather than long term as it is the short term data rate that is
    // limited for any real memory

    // calculate an appropriate tick to release to not exceed
    // the bandwidth limit
    Tick duration = pkt->getSize() * bandwidth;

    // only consider ourselves busy if there is any need to wait
    // to avoid extra events being scheduled for (infinitely) fast
    // memories
    if (duration != 0) {
        schedule(releaseEvent, curTick() + duration);
        isBusy = true;
    }

    // go ahead and deal with the packet and put the response in the
    // queue if there is one
    bool needsResponse = pkt->needsResponse();
    recvAtomic(pkt);
    // turn packet around to go back to requester if response expected
    if (needsResponse) {
        // recvAtomic() should already have turned packet into
        // atomic response
        assert(pkt->isResponse());
        // to keep things simple (and in order), we put the packet at
        // the end even if the latency suggests it should be sent
        // before the packet(s) before it.
        Tick when_to_send = curTick() + receive_delay;
        if (pkt->isRead())
            when_to_send += getReadLatency();
        else if (pkt->isWrite())
            when_to_send += getWriteLatency();

        // typically this should be added at the end, so start the
        // insertion sort with the last element, also make sure not to
        // re-order in front of some existing packet with the same
        // address, the latter is important as this memory effectively
        // hands out exclusive copies (shared is not asserted)
        auto i = packetQueue.end();
        --i;
        while (i != packetQueue.begin() && when_to_send < i->tick &&
               !i->pkt->matchAddr(pkt))
            --i;

        // emplace inserts the element before the position pointed to by
        // the iterator, so advance it one step
        packetQueue.emplace(++i, pkt, when_to_send);

        if (!retryResp && !dequeueEvent.scheduled())
            schedule(dequeueEvent, packetQueue.back().tick);
    } else {
        pendingDelete.reset(pkt);
    }

    return true;
}

ScratchpadMemory*
ScratchpadMemoryParams::create()
{
    return new ScratchpadMemory(this);
}
