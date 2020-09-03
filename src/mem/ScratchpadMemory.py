from m5.params import *
from m5.objects.AbstractMemory import *
from m5.objects.SimpleMemory import *

class ScratchpadMemory(SimpleMemory):
    type = 'ScratchpadMemory'
    cxx_header = "mem/spm_mem.hh"
    port = SlavePort("Slave ports")
    latency_write = Param.Latency('10ns', "Write latency in SPM")
    latency_read = Param.Latency('2ns', "Read latency in SPM")
    latency_write_var =
        Param.Latency('0ns', "Request to response latency variance")

    energy_read = Param.Float('100', "Energy for each reading (pJ)")
    energy_write = Param.Float('600', "Energy for each writting (pJ)")
    energy_overhead = Param.Float('100', "Overhead energy (pJ)")
    # The memory bandwidth limit default is set to 12.8GB/s which is
    # representative of a x64 DDR3-1600 channel.
    bandwidth = Param.MemoryBandwidth('64GB/s',
                                      "Combined read and write bandwidth")


