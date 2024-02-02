from __future__ import annotations

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, Symbol, SymbolTableSection
from pathlib import Path
from typing import Dict, Tuple, Optional


class Elf():
    """
    Internal representation of Elf file.
    """

    def __init__(self,
                 elf: ELFFile,
                 kernel_start: int,
                 pc2fun: Dict[int, str],
                 fun2pc: Dict[str, int]):
        self._elf = elf
        self.kernel_start = kernel_start
        self.pc2fun = pc2fun
        self.fun2pc = fun2pc

    def __repr__(self) -> str:
        return f"<Elf kernel_start {self.kernel_start:#x}>"

    @staticmethod
    def inspect_elf_file(elf_file: Path) -> Optional[Elf]:
        """
        Read elf file and get info about functions adresses.

        Arguments:
            elf_file

        Returns:
            Elf representation used by KFT
        """
        def get_symbol_table_section(elf: ELFFile) -> Section:
            for section in elf.iter_sections():
                if isinstance(section, SymbolTableSection):
                    return section

        def is_function(s: Symbol) -> bool:
            return s.entry['st_info']['type'] == 'STT_FUNC'

        def read_symbol(s: Symbol) -> Tuple[int, str]:
            return (s.entry['st_value'], s.name)

        with open(elf_file, 'rb') as f:
            elf = ELFFile(f)

            sym_table = get_symbol_table_section(elf)
            if sym_table is None:
                return None

            kern_sym = sym_table.get_symbol_by_name('__kernel_start')
            if kern_sym is None:
                return None

            kern_start = read_symbol(kern_sym[0])[0]

            pc_to_fun = [read_symbol(s)
                         for s in sym_table.iter_symbols() if is_function(s)]
        pcs, fns = zip(*pc_to_fun)
        fun_to_pc = zip(fns, pcs)
        pc2fun = dict(pc_to_fun)
        fun2pc = dict(fun_to_pc)

        return Elf(elf, kern_start, pc2fun, fun2pc)
