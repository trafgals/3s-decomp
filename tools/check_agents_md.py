#!/usr/bin/env python3
"""Check that AGENTS.md and docs/upper3s/remaining.md agree with the source.

Every file, function, table and value the two documents name is looked up in
the tree:

* every file they name exists, and every `file.c:symbol` / `symbol()` they
  name is defined in the file the same line names;
* the character index map of AGENTS.md section 3.1 matches the two tables
  of the game that list the characters in index order;
* the numbering rules of section 3.2 (pass, plpat, cmd tables) hold for
  every character;
* the table locations, meter-gain columns, struct sizes and build facts of
  sections 3.3, 3.4 and 5 hold;
* in remaining.md: the summary counts match the tables, every table value
  quoted as "current" is the retail value, and every per-character table
  reference sits under the heading of the character it indexes.

Usage: python3 tools/check_agents_md.py [--agents PATH] [--remaining PATH]
Exits 0 when every check passes, 1 otherwise.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ANNIV = ROOT / "src" / "anniversary"
GAME = ANNIV / "sf33rd" / "Source" / "Game"

# Paths in remaining.md that only exist on the PR #1 branch. remaining.md cites them to say what that
# branch contains, so their absence here is expected, not an error.
PR_ONLY_PREFIXES = ("upper3s/", "tests/", "tools/upper3s", "tools/verify", "docs/upper3s/spec-map.md")
# The "Kind" column vocabulary of remaining.md (defined in its "How to read this" section).
KINDS = {"table", "logic", "disc", "new", "find", "none"}

failures = []
passed = 0


def check(cond, msg):
    global passed
    if cond:
        passed += 1
    else:
        failures.append(msg)
    return cond


_text_cache = {}


def text(path):
    path = Path(path)
    if path not in _text_cache:
        _text_cache[path] = path.read_text(encoding="utf-8", errors="replace")
    return _text_cache[path]


def resolve(name):
    """A file name as the docs write it -> its path, or None."""
    # The docs write repo paths ("tools/x.py"), paths under src/anniversary ("bin2obj/gauge.c") and bare
    # Game/ file names ("PLCNT.c"), so try the three roots in that order.
    for base in (ROOT, ANNIV, GAME):
        p = base / name
        if p.is_file():
            return p
    return None


def game_file(name):
    """A Game/ file by name, ignoring case (the tree mixes PLPAT09.c and plpat10.c)."""
    for p in GAME.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


# --- C source helpers -------------------------------------------------------


def def_regexes(name):
    n = re.escape(name)
    return [
        # function definition: "type name(args) {"
        re.compile(rf"^[A-Za-z_][\w \t*]*?[\s*]{n}\s*\([^;{{]*\)\s*\{{", re.M),
        # variable or array at file scope: "type name[..] = " / "type name[..];"
        re.compile(rf"^(?!extern\b)(?!return\b)[A-Za-z_][\w \t*]*?\b{n}\s*(\[[^\]]*\]\s*)*(=|;)", re.M),
        # function-pointer table: "void (*const name[])() = {"
        re.compile(rf"^(?!extern\b)[A-Za-z_][^\n;]*\(\s*\*\s*(const\s+)?{n}\s*(\[[^\]]*\]\s*)*\)", re.M),
    ]


def defines(path, name):
    src = text(path)
    return any(r.search(src) for r in def_regexes(name))


def braced(src, start):
    """The text between the first '{' at or after start and its matching '}'."""
    i = src.index("{", start)
    depth = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                return src[i + 1 : j]
    raise ValueError("unbalanced braces")


def initializer(path, name):
    src = text(path)
    m = re.search(rf"\b{re.escape(name)}\b[^;=]*=\s*\{{", src)
    if not m:
        raise KeyError(f"{name} has no initializer in {path.name}")
    return braced(src, m.end() - 1)


def function_body(path, name):
    src = text(path)
    m = def_regexes(name)[0].search(src)
    if not m:
        raise KeyError(f"{name}() is not defined in {path.name}")
    return braced(src, m.end() - 1)


def int_rows(body):
    return [[int(x) for x in r.split(",") if x.strip()] for r in re.findall(r"\{\s*([-\d,\s]+?)\s*\}", body)]


def struct_size(name):
    """The '// total size' comment of 'typedef struct { ... } name;'."""
    for p in list((ROOT / "include").rglob("*.h")) + list(GAME.glob("*.c")):
        src = text(p)
        m = re.search(rf"^\}}\s*{re.escape(name)}\s*;", src, re.M)
        if m:
            start = src.rfind("typedef struct", 0, m.start())
            size = re.search(r"total size:\s*(0x[0-9A-Fa-f]+)", src[start : m.start()])
            return int(size.group(1), 16) if size else None
    return None


# --- Markdown helpers -------------------------------------------------------


def section(md, number):
    """The text of '## N.' up to the next '## '."""
    m = re.search(rf"^## {number}\..*?(?=^## |\Z)", md, re.M | re.S)
    return m.group(0) if m else ""


def cells(line):
    return [c.strip() for c in line.strip().strip("|").split("|")]


def spans(line):
    return re.findall(r"`([^`]+)`", line)


# Spans that name a family rather than one thing (`passNN.c`, `plpatNN.c`, `PLSxx.c`, `nm_*`,
# `add_sp_arts_gauge_*()`, `pX_cmd`) can't be looked up, so they are skipped.
PLACEHOLDER = re.compile(r"NN|xx|\*|…|pX_cmd")


def check_references(md, label, skip_prefixes=(), stop_at=None):
    """Files exist; 'file.c:sym' is defined in file.c; bare 'sym()' is defined
    in a .c file named on the same line (or anywhere in Game/ if none is).
    Checking stops at the heading `stop_at` (a section about another branch)."""
    for lineno, line in enumerate(md.splitlines(), 1):
        if stop_at and line.startswith(stop_at):
            break
        refs = [s for s in spans(line) if not PLACEHOLDER.search(s) and not s.startswith(skip_prefixes)]
        files_on_line = []
        for s in refs:
            m = re.fullmatch(r"([\w./-]+\.(?:c|h|py|md|sh))(?::(\w+).*)?", s)
            if not m:
                continue
            path = resolve(m.group(1))
            if check(path is not None, f"{label}:{lineno}: file `{m.group(1)}` does not exist"):
                if path.suffix == ".c":
                    files_on_line.append(path)
                if m.group(2):
                    check(defines(path, m.group(2)), f"{label}:{lineno}: `{m.group(2)}` is not defined in {m.group(1)}")
        for s in refs:
            m = re.fullmatch(r"(\w+)\(\)", s)
            if not m:
                continue
            name = m.group(1)
            pool = files_on_line or sorted(GAME.glob("*.c"))
            check(any(defines(p, name) for p in pool),
                  f"{label}:{lineno}: `{name}()` is not defined in {', '.join(p.name for p in files_on_line) or 'Game/'}")


# --- AGENTS.md checks -------------------------------------------------------


def name_tokens(display):
    """'Gouki (Akuma)' -> {'GOUKI', 'AKUMA'}; 'Chun-Li' -> {'CHUN_LI', 'CHUN-LI'}."""
    words = re.findall(r"[A-Za-z0-9-]+", display)
    toks = {w.upper() for w in words} | {w.upper().replace("-", "_") for w in words}
    if display == "Twelve":
        toks.add("NO12")
    return toks


def index_map(md):
    pairs = re.findall(r"`(\d\d)`\s*\|\s*([^|]+?)\s*\|", section(md, 3))
    return {int(i): n for i, n in pairs}


def check_index_map(md, chars):
    # The game never lists its characters by name in one place, so the order is checked against two
    # independent tables that happen to be in index order:
    #   Ck_Pass.c:Passive_AS_tbl  CPU tables VS_GILL_AS, VS_ALEX_AS, ... (Yang reuses VS_YUN_AS at 10)
    #   Eff99.c:Letter_Data_99[4] "ALEX STAGE", ... "REMY STAGE" (blank for Gill 0 and Q 17)
    # Every index is covered by at least one of them, and both must agree with the doc where they name someone.
    check(sorted(chars) == list(range(20)), f"AGENTS.md 3.1: the map has {len(chars)} entries, not 0..19")
    ck = [m for m in re.findall(r"VS_(\w+?)_AS\b", initializer(GAME / "Ck_Pass.c", "Passive_AS_tbl"))]
    groups = re.findall(r"\{([^{}]*)\}", initializer(GAME / "Eff99.c", "Letter_Data_99"))
    stages = re.findall(r'"([^"]*)"', groups[4])  # row 4 is the "<character> STAGE" row
    check(len(ck) == 20 and len(stages) >= 20, "Ck_Pass.c / Eff99.c tables did not parse")
    for i in range(20):
        name = chars.get(i, "?")
        toks = name_tokens(name)
        stage = stages[i].replace(" STAGE", "") if i < len(stages) else ""
        if i == 10:  # Yang reuses Yun's CPU table
            check(ck[i] == "YUN" and stage == "YANG" and name == "Yang",
                  f"AGENTS.md 3.1: index 10 should be Yang (VS_YUN_AS, 'YANG STAGE'), doc says {name}")
            continue
        check(ck[i] in toks, f"AGENTS.md 3.1: index {i} is VS_{ck[i]}_AS in Ck_Pass.c, doc says {name}")
        if stage:
            check(stage in toks or stage.replace("-", "_") in toks,
                  f"AGENTS.md 3.1: index {i} is '{stages[i]}' in Eff99.c, doc says {name}")
    check(re.search(r"charset_id = plid_data\[My_char\[", text(GAME / "PLCNT.c")) is not None,
          "PLCNT.c: charset_id is not plid_data[My_char[..]]")


def check_numbering(md, chars):
    idx = {n.split(" (")[0]: i for i, n in chars.items()}
    s32 = section(md, 3)

    pls = sorted(p.name for p in GAME.glob("*") if re.fullmatch(r"PLS\d\d\.c", p.name, re.I))
    check(pls == ["PLS00.c", "PLS01.c", "PLS02.c", "PLS03.c"], f"PLSxx.c files are {pls}, not PLS00-03")

    jmp = re.findall(r"\w+", initializer(GAME / "Com_Pl.c", "Passive_Jmp_Tbl"))
    for i in range(20):
        check(jmp[i] == f"Passive{i:02d}", f"Com_Pl.c: Passive_Jmp_Tbl[{i}] is {jmp[i]}")
        f = game_file(f"pass{i:02d}.c")
        check(f is not None and defines(f, f"Passive{i:02d}"), f"pass{i:02d}.c does not define Passive{i:02d}")
    m = re.search(r"`pass(\d\d)\.c` is (\w+)", s32)
    check(m and int(m.group(1)) == idx.get(m.group(2)), "AGENTS.md 3.2: the passNN.c example does not match the index")

    # plpat files: the dispatch table has no pl15_extra_attack, so from Chun-Li
    # (index 15) on the file number is index + 1. Issue #2 of the repository got this wrong.
    ext = re.findall(r"\w+", initializer(GAME / "PLPAT.c", "plxx_extra_attack_table"))
    for i in range(20):
        n = i if i < 15 else i + 1
        check(ext[i] == f"pl{n:02d}_extra_attack", f"PLPAT.c: plxx_extra_attack_table[{i}] is {ext[i]}, expected pl{n:02d}")
    check(game_file("plpat15.c") is None, "a plpat15.c exists")
    examples = re.findall(r"([A-Z][\w-]*) `plpat(\d\d)\.c`", s32)
    check(len(examples) == 5, f"AGENTS.md 3.2: expected 5 plpat examples, found {len(examples)}")
    for who, nn in examples:
        i = idx.get(who)
        f = game_file(f"plpat{nn}.c")
        check(i is not None and ext[i] == f"pl{nn}_extra_attack" and f and defines(f, f"pl{nn}_extra_attack"),
              f"AGENTS.md 3.2: {who} is not plpat{nn}.c")

    # Command tables are named in hex but skip F: p0..p9, pA..pE (0-14), then p10..p14 (15-19).
    cmd = re.findall(r"\w+", initializer(GAME / "cmd_data.c", "pl_cmd"))
    want = [f"p{i}_cmd" for i in range(10)] + [f"p{c}_cmd" for c in "ABCDE"] + [f"p1{i}_cmd" for i in range(5)]
    check(cmd[:20] == want, f"cmd_data.c: pl_cmd order is {cmd[:20]}")
    m = re.search(r"(\w+) is `(p\w+_cmd)`", s32)
    i = idx.get(m.group(1)) if m else None
    check(i is not None and cmd[i] == m.group(2), "AGENTS.md 3.2: the pX_cmd example does not match pl_cmd")


def sa_table(name="super_arts_data"):
    """The SA_DATA rows of a table, and the positions of gauge_len and store_max in each row.
    The positions are read from the struct in PLCNT.h rather than hard-coded, so a reordered or renamed
    field fails loudly instead of silently reading the wrong column."""
    fields = re.findall(r"\b(\w+);\s*//", re.search(r"typedef struct \{(.*?)\}\s*SA_DATA;",
                        text(ROOT / "include/sf33rd/Source/Game/PLCNT.h"), re.S).group(1))
    rows = int_rows(initializer(GAME / "PLCNT.c", name))
    return rows, fields.index("gauge_len"), fields.index("store_max")


def check_tables(md, chars):
    idx = {n.split(" (")[0]: i for i, n in chars.items()}
    rows, gl, sm = sa_table()
    check(len(rows) == 80, f"super_arts_data has {len(rows)} entries, not 80")
    # Known retail values that only line up under the right index order: Shippu Jinrai Kyaku (Ken SA III)
    # has 3 stocks; Akuma and Remy are the only short stun bars.
    check(rows[idx["Ken"] * 4 + 2][sm] == 3, "super_arts_data: Ken's SA III does not have 3 stocks")
    check(re.search(r"if \(cmd_sel\[ix\] \|\| no_sa\[ix\]\) \{\s*saptr = &super_arts_DATA", text(GAME / "PLCNT.c")),
          "PLCNT.c: super_arts_DATA is not chosen by cmd_sel[ix] || no_sa[ix]")

    piyo = int_rows("{" + initializer(GAME / "PLCNT.c", "pl_piyo_tbl") + "}")[0]
    check(set(piyo) == {56, 64, 72}, f"pl_piyo_tbl values are {sorted(set(piyo))}")
    shorts = [i for i, v in enumerate(piyo) if v == 56]
    check(shorts == [idx["Gouki"], idx["Remy"]], f"pl_piyo_tbl short bars are at {shorts}")
    check(re.search(r"\.recover = pl_nr_piyo_tbl\[", text(GAME / "PLCNT.c")), "pl_nr_piyo_tbl is not the stun recovery")
    check("56 short, 64 regular, 72 long" in md, "AGENTS.md: stun bar sizes not stated")

    check("Com_Vital_Unit_Data" in function_body(GAME / "PLS02.c", "setup_vitality"),
          "setup_vitality() does not read Com_Vital_Unit_Data")
    # The doc's "Column N ... `add_sp_arts_gauge_X()`" claims, against the column each function reads.
    meter_row = next((l for l in md.splitlines() if l.startswith("| Meter gain")), "")
    claims = re.findall(r"(?:Column |, )(\d)\b[^`]*?`(?:PLS02\.c:)?(add_sp_arts_gauge_\w+)\(\)`", meter_row)
    check(sorted(int(c) for c, _ in claims) == [0, 1, 2, 3], f"AGENTS.md 3.3: meter-gain columns not all stated: {claims}")
    for col, fn in claims:
        cols = set(re.findall(r"_add_arts_gauge\[[^\]]+\]\[[^\]]+\]\[(\d)\]", function_body(GAME / "PLS02.c", fn)))
        check(cols == {col}, f"AGENTS.md 3.3: {fn}() reads _add_arts_gauge column(s) {sorted(cols)}, doc says {col}")
    whiff = dict((int(c), fn) for c, fn in claims).get(0, "")
    check(whiff and re.search(rf"^\s+{whiff}\(", text(GAME / "CHARSET.c"), re.M),
          f"AGENTS.md 3.3: the whiff-gain function {whiff}() is not the one CHARSET.c calls as an attack frame loads")
    check(re.search(r"^\s+add_sp_arts_gauge_guard\(", text(GAME / "HITCHECK.c"), re.M), "HITCHECK.c does not call add_sp_arts_gauge_guard()")
    check("_add_piyo_gauge[as->player_number][as->wu.att.piyo]" in function_body(GAME / "HITCHECK.c", "set_damage_and_piyo"),
          "set_damage_and_piyo() does not index _add_piyo_gauge by att.piyo")
    check("Power_Data[" in function_body(GAME / "Pow_Pow.c", "cal_damage_vitality"), "cal_damage_vitality() does not read Power_Data")

    body = function_body(GAME / "CHARID.c", "set_char_base_data")
    check("cdat = &char_init_data[wk->charset_id]" in body, "set_char_base_data() does not use char_init_data[charset_id]")
    for field in ("char_table[", "hit_ix_table", "body_adrs", "hand_adrs", "catch_adrs", "caught_adrs",
                  "attack_adrs", "hosei_adrs", "att_ix_table"):
        check(re.search(rf"wk->{re.escape(field)}[^=]*= cdat->", body), f"set_char_base_data() does not set {field}")
    structs = text(ROOT / "include/structs.h")
    for field in ("cg_hit_ix", "cg_cancel"):
        check(re.search(rf"\b{field};", structs), f"WORK has no {field}")
    check(re.search(r"->add_arts_point = ", text(GAME / "CHARSET.c")), "CHARSET.c does not set add_arts_point")


def check_build(md):
    s1, s5 = section(md, 1), section(md, 5)
    game_src = "\n".join(text(p) for p in (ANNIV / "sf33rd").rglob("*.c"))
    for macro, count_word in (("INCLUDE_ASM", "two game functions"), ("INCLUDE_RODATA", "two effect jump tables")):
        found = sorted(re.findall(rf'{macro}\("[^"]+", (\w+)\)', game_src))
        check(len(found) == 2 and count_word in s1, f"AGENTS.md 1: game {macro} entries are {found}")
        for name in found:
            check(f"`{name}`" in s1, f"AGENTS.md 1: {macro} {name} is not listed")
    cri_open = len(re.findall(r"^- \[ \]", text(ROOT / "cri-progress.md"), re.M))
    check(cri_open == 6 and "six CRI" in s1, f"AGENTS.md 1: {cri_open} CRI functions are open")

    for name, claim in re.findall(r"`(\w+)` (?:is )?(0x[0-9A-Fa-f]+)", s5):
        check(struct_size(name) == int(claim, 16), f"AGENTS.md 5: {name} is {struct_size(name)}, doc says {claim}")
    check(len(re.findall(r"`(\w+)` (?:is )?(0x[0-9A-Fa-f]+)", s5)) == 4, "AGENTS.md 5: expected 4 struct sizes")
    all_src = "\n".join(text(p) for p in list((ROOT / "include").rglob("*.h")) + list(ANNIV.rglob("*.c")))
    check(not re.search(r"\}\s*GAUGE\s*;", all_src), "a GAUGE struct exists")

    # The build facts of 5.1/5.2: the PS2 rule byte-compares, UPPER_3S adds -DUPPER_3S, the host
    # platforms exist, and both test suites and their make targets are there.
    mk = text(ROOT / "Makefile")
    check(re.search(r"\$\(COMPARE_BYTES\) \$\(MAIN\) \$\(MAIN_TARGET\)", mk), "the PS2 build no longer byte-compares")
    check(re.search(r"\$\(CRI_O_FILES\):.*\n.*\n\s*\$\(EEGCC_PATHS\) \$\(EEGCC\)", mk), "CRI is not built with EE GCC")
    check(re.search(r"ifeq \(\$\(UPPER_3S\),1\)\nUPPER_3S_DEFINE := -DUPPER_3S", mk), "UPPER_3S=1 does not add -DUPPER_3S")
    for platform in ("web", "headless"):
        check(f"ifeq ($(PLATFORM),{platform})" in mk, f"the Makefile has no PLATFORM={platform}")
    for target in ("test:", "match-test:", "match-build:"):
        check(re.search(rf"^{target}", mk, re.M), f"the Makefile has no {target} target")
    for d in ("tests/upper3s", "tests/match", "src/anniversary/port/headless", "src/anniversary/sf33rd/Source/Game/upper3s"):
        check((ROOT / d).is_dir(), f"{d} is missing")


def check_identifiers(md):
    """Backticked identifiers in section 3 exist somewhere in the source."""
    words = set()
    for p in list((ROOT / "include").rglob("*.h")) + list(ANNIV.rglob("*.c")):
        words.update(re.findall(r"\w+", text(p)))
    for lineno, line in enumerate(section(md, 3).splitlines(), 1):
        for s in spans(line):
            if PLACEHOLDER.search(s) or re.search(r"\.(c|h|py|md)\b", s):
                continue
            for ident in re.findall(r"[A-Za-z_]\w*", s):
                if "_" in ident:
                    check(ident in words, f"AGENTS.md 3, line {lineno}: `{ident}` does not occur in the source")


# --- remaining.md checks ----------------------------------------------------


def check_remaining(md, chars):
    idx = {n.split(" (")[0]: i for i, n in chars.items()}
    by_index = {i: n.split(" (")[0] for i, n in chars.items()}
    rows, gl, sm = sa_table()
    piyo = int_rows("{" + initializer(GAME / "PLCNT.c", "pl_piyo_tbl") + "}")[0]

    ids, items = set(), []
    # heading_char is the character whose "### Name (index)" section we are in. A table reference
    # (super_arts_data[c], pl_piyo_tbl[c], Com_Vital_Unit_Data[c], plpatNN.c) under a character's heading
    # must index that character: putting one character's change in another's data is exactly the mistake
    # PR #1 made with Remy's Cold Blue Kick.
    heading_char = None
    seen_chars = set()
    for lineno, line in enumerate(md.splitlines(), 1):
        h = re.match(r"^### (.+?) \((\d+)\)\s*$", line)
        if h:
            name, i = h.group(1), int(h.group(2))
            check(i in chars and name_tokens(name) <= name_tokens(chars[i]),
                  f"remaining.md:{lineno}: heading '{line[4:]}' does not match index {i} ({by_index.get(i)})")
            heading_char = i
            seen_chars.add(i)
            continue
        if line.startswith("## "):
            heading_char = None
        c = cells(line) if line.startswith("|") else []
        # Item rows: SM1, B1, CL1 ... (I1.. are infrastructure, not spec items).
        if c and re.fullmatch(r"SM\d+|B\d+|[A-Z]{1,2}\d+", c[0]) and not re.fullmatch(r"I\d+", c[0]):
            check(c[0] not in ids, f"remaining.md:{lineno}: duplicate id {c[0]}")
            ids.add(c[0])
            kinds = {k.strip() for k in c[2].split("+")}
            check(kinds <= KINDS, f"remaining.md:{lineno}: unknown kind {kinds - KINDS}")
            status = c[-1].split()[0] if c[-1] else ""
            check(status in {"verified", "done", "partial", "—", "n/a"}, f"remaining.md:{lineno}: unknown status '{c[-1]}'")
            items.append(("none" in kinds, status))
        # "`super_arts_data[c][s]` 104/2 → 128/1": the value before the arrow is the claimed retail value
        # (length/stocks) and must match the table; the target after it is the spec's and isn't checked.
        for a, s, cur_len, cur_stk in re.findall(r"`super_arts_data\[(\d+)\]\[(\d)\]` (\d+)(?:/(\d+))?", line):
            row = rows[int(a) * 4 + int(s)]
            check(row[gl] == int(cur_len), f"remaining.md:{lineno}: super_arts_data[{a}][{s}] length is {row[gl]}, not {cur_len}")
            if cur_stk:
                check(row[sm] == int(cur_stk), f"remaining.md:{lineno}: super_arts_data[{a}][{s}] stocks are {row[sm]}, not {cur_stk}")
            if heading_char is not None:
                check(int(a) == heading_char, f"remaining.md:{lineno}: super_arts_data[{a}] under {by_index[heading_char]}")
        for a, cur in re.findall(r"`pl_piyo_tbl\[(\d+)\]` (\d+)", line):
            check(piyo[int(a)] == int(cur), f"remaining.md:{lineno}: pl_piyo_tbl[{a}] is {piyo[int(a)]}, not {cur}")
            if heading_char is not None:
                check(int(a) == heading_char, f"remaining.md:{lineno}: pl_piyo_tbl[{a}] under {by_index[heading_char]}")
        for a in re.findall(r"`Com_Vital_Unit_Data\[(\d+)\]`", line):
            if heading_char is not None:
                check(int(a) == heading_char, f"remaining.md:{lineno}: Com_Vital_Unit_Data[{a}] under {by_index[heading_char]}")
        for nn in re.findall(r"`plpat(\d\d)\.c`", line, re.I):
            if heading_char is not None:
                want = heading_char if heading_char < 15 else heading_char + 1
                check(int(nn) == want, f"remaining.md:{lineno}: plpat{nn}.c under {by_index[heading_char]}")

    check(seen_chars == set(range(20)), f"remaining.md: character sections missing: {sorted(set(range(20)) - seen_chars)}")

    work = [s for none, s in items if not none]
    counts = {"all": len(work), "none": sum(1 for none, _ in items if none), "verified": work.count("verified"),
              "done": work.count("done"), "partial": work.count("partial"), "open": work.count("—")}
    m = re.search(r"\*\*(\d+) work items\*\* \(plus (\d+) \"no change\" items\)\. On `main`: all (\d+) open\. "
                  r"On this branch: (\d+) verified in a fight test, (\d+) done but not yet measured, (\d+) partial, "
                  r"(\d+) not done\.", md)
    if check(m is not None, "remaining.md: summary line not found"):
        got = dict(zip(("all", "none", "main", "verified", "done", "partial", "open"), map(int, m.groups())))
        for k in ("all", "none", "verified", "done", "partial", "open"):
            check(got[k] == counts[k], f"remaining.md summary: {k} is {got[k]}, the tables give {counts[k]}")
        check(got["main"] == counts["all"], "remaining.md summary: 'all N open' does not match the item count")
    return counts


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--agents", type=Path, default=ROOT / "AGENTS.md")
    ap.add_argument("--remaining", type=Path, default=ROOT / "docs/upper3s/remaining.md")
    args = ap.parse_args()

    agents = args.agents.read_text(encoding="utf-8")
    remaining = args.remaining.read_text(encoding="utf-8")
    chars = index_map(agents)

    steps = [
        ("AGENTS.md references", lambda: check_references(agents, "AGENTS.md")),
        ("AGENTS.md identifiers", lambda: check_identifiers(agents)),
        ("character index map", lambda: check_index_map(agents, chars)),
        ("file numbering", lambda: check_numbering(agents, chars)),
        ("tables and data flow", lambda: check_tables(agents, chars)),
        ("build and structs", lambda: check_build(agents)),
        ("remaining.md references",
         lambda: check_references(remaining, "remaining.md", PR_ONLY_PREFIXES, stop_at="## Defects in PR #1")),
        ("remaining.md items", lambda: check_remaining(remaining, chars)),
    ]
    for name, step in steps:
        before = len(failures)
        try:
            result = step()
        except (KeyError, ValueError, IndexError, AttributeError) as e:
            failures.append(f"{name}: could not run: {e!r}")
            result = None
        status = "ok" if len(failures) == before else f"FAILED ({len(failures) - before})"
        extra = f"  {result}" if isinstance(result, dict) else ""
        print(f"{status:>12}  {name}{extra}")

    for f in failures:
        print(f"  - {f}")
    print(f"{passed} checks passed, {len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
