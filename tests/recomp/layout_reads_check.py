#!/usr/bin/env python3
"""Reads of the original that depend on where memory lies are patched (0.6.2 open item 138b).

Usage: layout_reads_check.py --exe <cm0102.exe>

tools/recomp/layout_reads.py gives each such read a defined value. This check finds them in the original and fails
when one is not in its INSNS:
- An address used as a number, in one form: `mov eax, dword ptr [reg + off]`, then within six instructions `cdq`
  and `idiv` or `div` with eax not loaded again, where the same function also loads [reg + off] into a register and
  reads memory through it (so the field holds a pointer). In the 3.9.68 exe this form occurs once: choose_captain
  0x6e19b9 divides the manager's non-player record by 20. Other forms (a hash, a compare, `[reg]` with no offset) are
  not searched.
- The local [esp + 0x70] of game_daily_staff_update 0x5ce2d0: its loop reads it at 0x5ce830 and 0x5ce84f, and no
  instruction from the entry to the first read writes it. The patch at 0x5ce311 must write it.
- MATCH_PLAYER::MATCH_PLAYER 0x6eada0 sets no byte from +0x1d to +0x22, which match_eng_update_post_match_reputations
  0x6d1cc0 reads for an empty bench slot (+0x1f, +0x20, +0x21). The patch at 0x6eada0 must clear the record. The
  ctest layout_reads runs the constructor itself.
Each INSNS row must name the instruction that the original has at its address.
Exit code 0 when every case passes.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "recomp"))
import layout_reads  # noqa: E402
from pe import Image  # noqa: E402

DAILY, DAILY_READS, DAILY_LOCAL = 0x5CE2D0, (0x5CE830, 0x5CE84F), "dword ptr [esp + 0x70]"
CTOR, CTOR_END, UNSET = 0x6EADA0, 0x6EAE98, range(0x1D, 0x23)


def disassemble(img):
    import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.skipdata = True
    start, end = img.text
    return [(i.address, i.mnemonic, i.op_str) for i in md.disasm(img.read(start, end - start), start)]


def address_dividends(code):
    field = re.compile(r"eax, dword ptr \[(e[a-z]{2}) \+ (0x[0-9a-f]+)\]$")
    out = []
    for k, (a, m, o) in enumerate(code):
        f = field.match(o) if m == "mov" else None
        if not f or f.group(1) == "esp":
            continue
        after = code[k + 1:k + 7]
        div = next((n for n, x in enumerate(after) if x[1] in ("idiv", "div")), None)
        if div is None or not any(x[1] == "cdq" for x in after[:div]) or \
                any(x[2].startswith("eax,") and x[1] != "cdq" for x in after[:div]):
            continue
        load = f"dword ptr [{f.group(1)} + {f.group(2)}]"
        for j in range(max(0, k - 120), min(len(code), k + 120)):
            if j == k or code[j][1] != "mov" or not code[j][2].endswith(load):
                continue
            reg = code[j][2].split(",")[0]
            if any(re.search(rf"\[{reg}( \+ [^\]]+)?\]", y[2]) for y in code[j + 1:j + 6]):
                out.append(a)
                break
    return out


def main():
    exe = sys.argv[sys.argv.index("--exe") + 1]
    img = Image(exe)
    code = disassemble(img)
    at = {a: (m, o) for a, m, o in code}
    fails = []
    for a, (text, _) in sorted(layout_reads.INSNS.items()):
        got = " ".join(x for x in at.get(a, ("?", "")) if x)
        if got != text:
            fails.append(f"{a:#x}: the original has {got!r}, INSNS names {text!r}")
    for a in address_dividends(code):
        if a not in layout_reads.INSNS:
            fails.append(f"{a:#x}: an address used as a number ({' '.join(at[a])}) with no patch")
    daily = [x for x in code if DAILY <= x[0] < DAILY_READS[0]]
    if any(x[2].startswith(DAILY_LOCAL + ",") for x in daily):
        fails.append(f"{DAILY:#x}: the original writes {DAILY_LOCAL} before {DAILY_READS[0]:#x}; read the case again")
    for r in DAILY_READS:
        if not at.get(r, ("", ""))[1].endswith(DAILY_LOCAL):
            fails.append(f"{r:#x}: not a read of {DAILY_LOCAL}")
    if "W32(esp + 0x70u" not in layout_reads.INSNS.get(0x5CE311, ("", ""))[1]:
        fails.append(f"{DAILY:#x}: {DAILY_LOCAL} is read before any write, and no patch sets it")
    ctor = [x for x in code if CTOR <= x[0] < CTOR_END]
    set_bytes = set()
    for _, m, o in ctor:
        s = re.match(r"(byte|word|dword) ptr \[eax \+ (0x[0-9a-f]+)\], ", o) if m == "mov" else None
        if s:
            n = {"byte": 1, "word": 2, "dword": 4}[s.group(1)]
            set_bytes |= set(range(int(s.group(2), 16), int(s.group(2), 16) + n))
    if set_bytes & set(UNSET):
        fails.append(f"{CTOR:#x}: the original sets a byte of +0x1d..+0x22; read the case again")
    if "memset(M + ecx, 0, 0x1beu)" not in layout_reads.INSNS.get(CTOR, ("", ""))[1]:
        fails.append(f"{CTOR:#x}: the match player's bytes +0x1d..+0x22 stay unset, and no patch clears them")
    for f in fails:
        print("FAIL  " + f)
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
