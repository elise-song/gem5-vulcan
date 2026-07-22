from typing import (
    Iterator,
    List,
)

from m5.objects import (
    BaseTrafficGen,
    PyTrafficGen,
)
from m5.params import Port

from gem5.utils.override import overrides
from gem5.components.processors.abstract_core import AbstractCore
from gem5.components.processors.abstract_generator_core import AbstractGeneratorCore
from gem5.components.processors.abstract_generator import (
    AbstractGenerator,
)


class PrimeProbeGeneratorCore(AbstractGeneratorCore):
    def __init__(self, victim_accesses: List[int], num_sets: int, assoc: int):
        super().__init__()
        """ The prime and probe core interface.

        This class defines the interface for a generator core that will create
        prime and probe traffic. This core uses PyTrafficGen to create and
        inject the synthetic traffic.

        """
        self._victim_accesses = victim_accesses
        self._num_sets = num_sets
        self._assoc = assoc
        self.generator = PyTrafficGen()

    @overrides(AbstractCore)
    def connect_dcache(self, port: Port) -> None:
        self.generator.port = port

    def _set_traffic(self) -> None:
        """
        This private function will set the traffic to be generated.
        """
        self._traffic = self._create_traffic()

    def _create_traffic(self) -> Iterator[BaseTrafficGen]:
        """
        A python generator that yields (creates) prime and probe traffic in the
        generator core and then yields (creates) an exit traffic.

        :rtype: Iterator[BaseTrafficGen]
        """
        duration = 60000 #ticks
        accessSize = 4
        period = 30000
        dataLimit = 0
        startAddr = 0
        read = 100
        write = 0
        stride = self._num_sets * 64

        #bring all victims into cache
        for secret in self._victim_accesses:
            yield self.generator.createLinear(
                duration,
                secret,
                secret + accessSize,
                accessSize,
                period,
                period,
                read,
                dataLimit,
            )
        yield self.generator.createIdle(20 * duration)
        yield self.generator.createExit(0)
        #lock all victim lines

        #prime
        for i in range(self._num_sets):
            for k in range(self._assoc + 1):
                startAddr = i * 64 + k * stride
                endAddr = startAddr + accessSize
                yield self.generator.createLinear(
                    duration,
                    startAddr,
                    endAddr,
                    accessSize,
                    period,
                    period,
                    write,
                    dataLimit,
                )
        yield self.generator.createIdle(10 * duration)
        yield self.generator.createExit(0)

        #probe
        for secret in self._victim_accesses:
            yield self.generator.createLinear(
                duration,
                secret,
                secret + accessSize,
                accessSize,
                period,
                period,
                read,
                dataLimit,
            )
        yield self.generator.createIdle(10 * duration)

        for j in range(self._num_sets):
            startAddr = j * 64
            endAddr = startAddr + accessSize
            yield self.generator.createLinear(
                duration,
                startAddr,
                endAddr,
                accessSize,
                period,
                period,
                read,
                dataLimit,
            )
        yield self.generator.createIdle(10 * duration)
        yield self.generator.createExit(0)
        #unlock all victim lines

        yield self.generator.createExit(0)

    @overrides(AbstractGeneratorCore)
    def start_traffic(self):
        self._set_traffic()
        self.generator.start(self._traffic)



class PrimeProbeGenerator(AbstractGenerator):
    def __init__(self, victim_accesses: List[int], num_sets: int, assoc: int):
        """The prime and probe generator

        This class defines an external interface to create a list containing one
        PrimeProbeGeneratorCore that can replace the processing core on the board.

        """
        super().__init__(cores=[PrimeProbeGeneratorCore(victim_accesses, num_sets, assoc)])



    @overrides(AbstractGenerator)
    def start_traffic(self) -> None:
        for core in self.cores:
            core.start_traffic()
