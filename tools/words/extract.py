#!/usr/bin/env python3
"""Every string CATACOMB shows a player, gathered for the words editor (docs/PLAN-words.md).

    python3 tools/words/extract.py [outdir]        # default build/words

Writes catalogue.json (one entry per distinct string: where it is drawn, what fills its blanks, how
wide it may be, which of my habits it leans on) and profiles.json (the creatures, bosses, classes,
spells, runes and gifts, with what the game does with each now). Debug output, the console, pixel
art and save tags are left out. Nothing here changes the game.
"""
import hashlib
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FILES = ["src/scene_crawl.c", "src/crawl_gear.c"]
SCREEN = 40                                   # columns of the 8-pixel font across 320 pixels

# ---- a small C tokeniser: strings, identifiers, numbers, punctuation; comments and the preprocessor dropped ----

def tokens(src):
    """Yield (kind, text, pos, line, debug). debug: inside an #ifdef CRAWL_DEBUG (or HAUNT_DEBUG) branch."""
    i, n, line = 0, len(src), 1
    stack = []                                # per #if: 'dbg', 'nodbg' or 'other'
    bol = True                                # at the beginning of a line, but for blanks
    while i < n:
        c = src[i]
        if c == '\n':
            line += 1; i += 1; bol = True; continue
        if c in ' \t\r':
            i += 1; continue
        if bol and c == '#':                  # a preprocessor line, with its continuations
            j = i
            while j < n and src[j] != '\n':
                if src[j] == '\\' and j + 1 < n and src[j + 1] == '\n':
                    j += 2; line += 1; continue
                if src.startswith('/*', j):    # a comment on the line may run on
                    k = src.find('*/', j + 2); line += src.count('\n', j, k); j = k + 2; continue
                j += 1
            d = src[i:j]
            m = re.match(r'#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', d)
            if m:
                kw, rest = m.group(1), m.group(2)
                dbg = 'CRAWL_DEBUG' in rest or 'HAUNT_DEBUG' in rest
                if kw == 'ifdef' or (kw == 'if' and dbg and '!' not in rest):
                    stack.append('dbg' if dbg else 'other')
                elif kw == 'ifndef' or (kw == 'if' and dbg):
                    stack.append('nodbg' if dbg else 'other')
                elif kw == 'if':
                    stack.append('other')
                elif kw == 'else' and stack:
                    stack[-1] = {'dbg': 'nodbg', 'nodbg': 'dbg'}.get(stack[-1], 'other')
                elif kw == 'endif' and stack:
                    stack.pop()
            i = j; continue
        bol = False
        if src.startswith('//', i):
            while i < n and src[i] != '\n': i += 1
            continue
        if src.startswith('/*', i):
            k = src.find('*/', i + 2); line += src.count('\n', i, k); i = k + 2; continue
        debug = 'dbg' in stack
        if c == '"':
            j, buf = i + 1, []
            while src[j] != '"':
                if src[j] == '\\':
                    e = src[j + 1]
                    if e == 'n': buf.append('\n'); j += 2
                    elif e == 't': buf.append('\t'); j += 2
                    elif e == 'x':
                        m = re.match(r'[0-9a-fA-F]{1,2}', src[j + 2:]); buf.append(chr(int(m.group(0), 16))); j += 2 + len(m.group(0))
                    elif e in '01234567':
                        m = re.match(r'[0-7]{1,3}', src[j + 1:]); buf.append(chr(int(m.group(0), 8))); j += 1 + len(m.group(0))
                    else: buf.append(e); j += 2
                else:
                    buf.append(src[j]); j += 1
            yield ('str', ''.join(buf), i, line, debug); i = j + 1; continue
        if c == "'":
            j = i + 1
            while src[j] != "'": j += 2 if src[j] == '\\' else 1
            yield ('chr', src[i:j + 1], i, line, debug); i = j + 1; continue
        if c.isalpha() or c == '_':
            j = i
            while j < n and (src[j].isalnum() or src[j] == '_'): j += 1
            yield ('id', src[i:j], i, line, debug); i = j; continue
        if c.isdigit() or (c == '.' and i + 1 < n and src[i + 1].isdigit()):
            m = re.match(r'(0[xX][0-9a-fA-F]+|[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)[uUlLfF]*', src[i:])
            yield ('num', m.group(0), i, line, debug); i += len(m.group(0)); continue
        m = re.match(r'->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^!~<>=?:;,.(){}\[\]]', src[i:])
        yield ('p', m.group(0), i, line, debug); i += len(m.group(0))


def strip_comments(s):
    return re.sub(r'/\*.*?\*/', ' ', s, flags=re.S).strip()

# ---- walking the tokens: each literal with its function or table, the call it is in, and that call's arguments ----

PRINTF = {'snprintf': 2, 'sprintf': 1, 'printf': 0, 'fprintf': 1, 'vsnprintf': 2, 'LOST': 0}
SKIP_CALLS = {'fprintf', 'printf', 'getenv', 'strcmp', 'strncmp', 'strstr', 'strchr', 'strrchr', 'fopen', 'setenv', 'memcmp',
              'memcpy', 'perror', 'puts', 'DBG_COUNT', 'con_print', 'con_printf', 'con_line', 'sscanf', 'atoi', 'atof', 'strtol'}
SKIP_FUNCS = re.compile(r'^(con_|dbg_|debug_|crawl_debug|log_)')
SKIP_TABLES = re.compile(r'^(MAP_|CON_|PK_KEY|PK_PAD|SKULL|BONE$|SOUL$|RUNE$|ROMAN$|RUN_ROMAN|Q_TAG)')


def walk(path):
    src = open(os.path.join(ROOT, path), encoding='latin-1').read()
    toks = list(tokens(src))
    printf_like = dict(PRINTF)
    for m in re.finditer(r'\b([a-z_0-9]+)\(([^()]*?)const char \*fmt, \.\.\.\)', src):
        printf_like[m.group(1)] = m.group(2).count(',')
    out = []
    stack = []            # open brackets: dict(ch, callee, arg, starts, lits, pos)
    func = None           # the function or table being read, at depth 0
    last_top_id = None    # the last identifier at depth 0 (a function's or table's name)
    pending_table = None
    prev = [None, None, None]
    i = 0
    while i < len(toks):
        kind, text, pos, line, debug = toks[i]
        if kind == 'str':                     # adjacent literals join
            j, s = i + 1, text
            while j < len(toks) and toks[j][0] == 'str':
                s += toks[j][1]; j += 1
            inner = stack[-1] if stack else None
            lit = dict(text=s, line=line, debug=debug, func=func, pos=pos,
                       callee=inner['callee'] if inner and inner['ch'] == '(' else None,
                       arg=inner['arg'] if inner else 0,
                       table=None, index=None, args=None)
            tabs = [b for b in stack if b.get('tname')]
            if inner and inner['ch'] == '{' and tabs:  # inside an initialiser: which table, which element of it
                t0 = tabs[0]
                lit['table'] = t0['tname']
                lit['index'] = t0['arg']
                lit['depth'] = sum(1 for b in stack[stack.index(t0):] if b['ch'] == '{')
                # the call it is inside, if any (a table inside a function, or a compound literal)
                calls = [b for b in stack if b['ch'] == '(']
                lit['callee'] = calls[-1]['callee'] if calls else None
            for b in stack: b['lits'].append(lit)
            out.append(lit)
            prev = [prev[1], prev[2], toks[j - 1]]
            i = j; continue
        if kind == 'p':
            if text in '({[':
                callee = prev[2][1] if prev[2] and prev[2][0] == 'id' and text == '(' else None
                b = dict(ch=text, callee=callee, arg=0, starts=[toks[i + 1][2] if i + 1 < len(toks) else pos], lits=[], pos=pos)
                if text == '{' and i > 0 and toks[i - 1][1] == '=':   # NAME[...] = {, at any depth
                    k = i - 2
                    while k > 0 and toks[k][1] == ']':
                        d = 0
                        while k > 0:
                            d += {']': 1, '[': -1}.get(toks[k][1], 0); k -= 1
                            if d == 0: break
                    if toks[k][0] == 'id': b['tname'] = toks[k][1]
                if text == '{' and not [x for x in stack if x['ch'] in '({']:
                    if prev[2] and prev[2][1] == ')':
                        func = last_fn; b['table'] = None
                    else:
                        b['table'] = pending_table or last_top_id
                        func = b['table']
                stack.append(b)
            elif text in ')}]':
                b = stack.pop() if stack else None
                if b and b['ch'] == '(':
                    ends = b['starts'] + [pos + 1]
                    args = [strip_comments(src[ends[k]:ends[k + 1] - 1]).rstrip(',').strip() for k in range(len(b['starts']))]
                    for lit in b['lits']:
                        if lit.get('args') is None and lit['callee'] == b['callee'] and lit['pos'] >= b['pos']:
                            lit['args'] = args
                if text == '}' and not [x for x in stack if x['ch'] in '({']:
                    pending_table = None
            elif text == ',' and stack:
                stack[-1]['arg'] += 1
                stack[-1]['starts'].append(toks[i + 1][2])
            elif text == ';' and not stack:
                pending_table = None
            if text == '(' and not [x for x in stack[:-1] if x['ch'] in '({'] and prev[2] and prev[2][0] == 'id':
                last_fn = prev[2][1]
            if text == '[' and not [x for x in stack[:-1] if x['ch'] in '({'] and prev[2] and prev[2][0] == 'id':
                pending_table = prev[2][1]
            if text == '=' and not stack and prev[2] and prev[2][0] == 'id':
                pending_table = prev[2][1]
        elif kind == 'id' and not stack:
            last_top_id = text
        prev = [prev[1], prev[2], toks[i]]
        i += 1
    for lit in out:
        lit['printf'] = printf_like
    return out


# ---- which strings a player sees ----

def is_art(s):
    """Pixel maps, map legends and the like: no spaces, few capital letters."""
    if ' ' in s or len(s) < 4 or re.fullmatch(r"[A-Z][A-Z'\-!?.,:/0-9]*", s):
        return False
    letters = sum(ch.isalpha() and ch.isupper() for ch in s)
    return letters < len(s) * 0.4


def player_facing(lit):
    s = lit['text']
    if lit['debug']: return False
    if lit['callee'] in SKIP_CALLS: return False
    if lit['callee'] in ('snprintf', 'sprintf') and lit['arg'] == 2 and lit['args'] and 'stderr' in ' '.join(lit['args']): return False
    if lit['func'] and SKIP_FUNCS.match(lit['func']): return False
    if lit['table'] and SKIP_TABLES.match(lit['table']): return False
    bare = re.sub(r'%[-+ #0-9.]*[a-zA-Z]', '', re.sub(r'\bx(?=%|\d)', '', s))   # x%.2f: a times sign
    if re.search(r'[a-z]', bare): return False                     # CATACOMB writes in capitals
    if '\n' in s or re.search(r'\bEVENT\b', s) or s.startswith(('HVS', 'LDG', 'HVP', 'HVM')): return False
    words = re.sub(r'%[-+ #0-9.]*[a-zA-Z]', ' ', s)
    if not re.search(r'[A-Z]{2,}', words) and not re.fullmatch(r'\s*[A-Z]\s*', words or ''): return False
    if is_art(s): return False
    return True


# ---- blanks, sizes, sections, habits ----

SPEC = re.compile(r'%%|%[-+#0]*(\d+|\*)?(\.(\d+|\*))?(hh|h|ll|l|z)?([diouxXfFeEgGcsp])')


def blanks(lit):
    """The template with its printf blanks as {1}, {2}..., and what fills each."""
    s = lit['text']
    callee = lit['callee']
    fmt_at = lit['printf'].get(callee)
    fills = []
    k = 0
    def sub(m):
        nonlocal k
        if m.group(0) == '%%': return '%'
        k += 1
        typ = 'number' if m.group(5) in 'diouxXfFeEgG' else 'letter' if m.group(5) == 'c' else 'text'
        src = None
        if fmt_at is not None and lit['arg'] == fmt_at and lit['args'] and len(lit['args']) > fmt_at + k:
            src = lit['args'][fmt_at + k]
        note = None
        if src is None and callee == 'thing_say':
            src = "the thing's name"
        if src:
            pm = re.fullmatch(r'(.+?)\s*\?\s*"([^"]*)"\s*:\s*"([^"]*)"', src)
            if pm:
                a, b = pm.group(2), pm.group(3)
                note = ('plural ending: "%s" or "%s"' if len(a) <= 2 and len(b) <= 2 else 'either "%s" or "%s"') % (a, b)
        fills.append(dict(n=k, type=typ, code=src, note=note))
        return '{%d}' % k
    t = SPEC.sub(sub, s)
    return t, fills


def arg_int(lit, k):
    a = lit['args'][k] if lit['args'] and len(lit['args']) > k else None
    return int(a) if a and re.fullmatch(r'\d+', a) else None


def drawn(lit):
    """How it is drawn, and how many columns it has: (label, fits or None)."""
    c, a = lit['callee'], lit['arg']
    if c == 'say' or c == 'thing_say': return 'caption', SCREEN
    if c == 'shrine_note': return 'note on a screen', SCREEN - 2
    if c == 'toast' and a in (4, 5): return 'pop-up ' + ('title' if a == 4 else 'line'), 33
    if c == 'ach_earn' and a in (4, 5): return 'achievement ' + ('title' if a == 4 else 'line'), 33
    if c == 'hint8' and a == 3:
        x = arg_int(lit, 1); return 'hint (keys become buttons on a pad)', (320 - x) // 8 if x is not None else SCREEN
    if c in ('gfx_text8', 'gfx_text16') and a == 3:
        x, sc = arg_int(lit, 1), arg_int(lit, 5) or 1
        lab = 'heading' if c == 'gfx_text16' or sc > 1 else 'text'
        return lab, ((320 - x) // 8 if x is not None else SCREEN) // sc
    if c in ('text_wrap', 'text_wrap_ex') and a == 3:
        cols, lines = arg_int(lit, 4), arg_int(lit, 6)
        return 'wrapped text', ([cols, lines] if cols and lines else None)
    if c == 'text_centred' and a == 2:
        cols, lines = arg_int(lit, 3), arg_int(lit, 5)
        return 'wrapped text, centred', ([cols, lines] if cols and lines else None)
    if c == 'card_text': return 'card line', SCREEN
    if c in ('pz_reading',) and a == 3: return 'clue (read with E)', None
    if c in ('pz_reading', 'pz_standing', 'pz_wallthing', 'tri_thing') : return 'name of a thing', SCREEN - 4
    if c in ('snprintf', 'sprintf', 'pz_append', 'LOST'): return 'assembled text', None
    if c == 'map_text': return 'map label', None
    if c == 'gain_coin': return 'pop-up line', 33
    if lit['table']: return 'name in a table', None
    if c: return 'passed to ' + c, None
    return 'text', None


# The sections the editor shows, by the function or table a string is in (first match wins)
SECTIONS = [
    ('Creatures', r'^(KINDS|RANK_WORD|rank_word|name_newcomers|render_bestiary|bestiary)'),
    ('Bosses and their gifts', r'^(BOSSES|BA_NAME|boss_|knight_|spider_|deacon_|wraith_|render_bosscard|render_bossbar|BOSS|ab_|grave_call|web_|use_readied|ready_next|swift_dodge|wraithwalk|tether_hold|latch_)'),
    ('Classes and the Alignment', r'^(CLASSES|CLASS_|ALIGN|class_|render_class|update_class|align_|render_align|update_align|LINE_)'),
    ('Spells and runes', r'^(ATK_|DEF_|ELEM_|cast_|spell_|human_cast|rune_|DEF_SHORT|gear_blurb|pick_spell|old_spells)'),
    ('Gear and stats', r'^(stat_short|ABIL_|KIND_NAME|QUAL_|piece_|bag_|gear_|SLOT_NAME|take_piece|CP_|render_wardrobe|render_inv|render_elements|render_piece|piece)'),
    ('The caster: Atonement and cornerstones', r'^(wind_offer|render_caster|LINES|CS_|corner|render_stones|update_stones|ascend|line_|render_ascend|update_ascend|xp_|render_xp|over_|stat_over)'),
    ('Codex', r'^(CX_|codex|render_codex|stat_does|PZ_DOES|cx_)'),
    ('Levels, rooms and runs', r'^(LEVEL_NAMES|level_name|SET_NAME|RTYPES|ORDINAL|render_tally|TALLY|tally|LABEL|render_card|start_level|numeral|roman|RANK_ROMAN)'),
    ('Puzzles and documents', r'^(PZ_|pz_|cis_|pipes_|cclue_|pilgrim_|MARK_WORD|NTH|HOURS|BELL_WORD|KNIGHT_NAME|SAINT_|MINER|COURT|WORDS|DOC_|doc_|hide_|msg_|render_documents|search_|bsh_|render_bshrine|update_bshrine|HIDES)'),
    ('Trials', r'^(TRI_|tri_|render_trial|trial_)'),
    ('Hazards', r'^(HZ_|hz_|HAZ|hazard|omen|update_sites|place_sites|update_arising)'),
    ('The merchant and her hall', r'^(step_text|replay_|challenge_|render_letgo|update_letgo|auto_take|render_shop|merchant|render_merchant|update_merchant|ped_|offer_|render_offer|totem|render_totem|update_totem|portal|render_portal|update_portal|comp_|render_comp|update_comp|repo_|render_repo|update_repo|hall2|render_hall2|update_hall2|bell_|wfont_|font_|MM_|mm_|map_buy|render_map_ware|lamp)'),
    ('The Rune Shrine', r'^(shrine_|render_shrine|update_shrine|SHRINE)'),
    ('Altars and wisps', r'^(altar_|render_altar|update_altar|wisp|render_wisp|update_wisp|turn_undead)'),
    ('Secrets and pockets', r'^(open_secret|place_secrets|secret|pocket_|visportal|gain_coin|chain_)'),
    ('Death, the ledger and her claim', r'^(death_|render_card|blame_|claim_|render_claim|ledger|led_|render_ledger|update_ledger|stone|render_stone|update_stone|mark_|dead_eye|run_new|human_end|quit|render_quit|update_quit)'),
    ('The map', r'^(map_|render_map|MAP)'),
    ('Achievements', r'^(rec_|ach_|SECRET_NAME|SECRET_STEP|slayer|TRI_ACH|PZ_ACH|TRI_RUN_ACH|toast)'),
    ('Reckoning', r'^(reckon|render_reckon|update_reckon)'),
    ('Status bar, readout and controls', r'^(render_messages|render_statusbar|render_hud|render_bar|render_readout|pop_readout|render_target|look_|weak_word|render_sockets|render_charge|render_latch|render_toasts|hint|pk)'),
]


def section(lit):
    keys = [lit['table'] or '', lit['func'] or '']
    for name, rx in SECTIONS:
        if any(re.match(rx, k) for k in keys if k):
            return name
    return 'In play'


# My habits, as the owner asked them marked: patterns, not judgements
HABITS = [
    ('paired', 'two clauses balanced on a comma or semicolon', re.compile(r'(, (AND|BUT|OR|THEN|YET|NOR) |; )')),
    ('aphorism', 'a short, sayable line', re.compile(r'^(NOTHING|NONE|NO ONE|ALL |EVERY|WHAT |WHO |NEVER|ONLY )|\b(ALREADY|FOR LONG|AGAIN|YET|STILL)[.!]?$')),
    ('colon', 'a label, then the point after a colon', re.compile(r'^[A-Z\' ]{4,}: [A-Z]+ [A-Z]+')),
    ('the-heavy', 'THE three times or more, or a name that begins with it', None),
    ('as-clause', '"as it / as you / as they": a clause hung on AS', re.compile(r'\bAS (IT|YOU|THEY|HE|SHE|ONE|THE|A|IF|ONCE|ALWAYS)\b')),
    ('personified', 'the place, the dead or "it" as the one who acts', re.compile(r'\b(THE (DEAD|STONE|WALLS?|VOID|CRYPT|DARK|WATER|EARTH|FLOOR|PLACE)|IT|SHE) (WANTS|TAKES|KNOWS|WATCH|IS WATCHING|ARE WATCHING|GIVES|LISTEN|KEEPS|REMEMBERS|WAITS|ASKS|HEARS|FORGETS)')),
    ('of-the', 'an "X OF THE Y" name', re.compile(r'\b[A-Z\']+ OF THE [A-Z\']+')),
    ('inverted', 'old-fashioned order or words: lest, whoever, a while, till, round', re.compile(r'\b(LEST|WHOEVER|A WHILE|TILL|ROUND (YOU|IT|HIM|HER|THEM|CORNERS)|NOT ONE MORE|ONCE A|NONE BUT|THAT IS ALL)\b')),
    ('dense', 'more than 25 words in one place', None),
]


def habits(text, is_name):
    plain = re.sub(r'\{\d+\}', 'X', text)
    out = []
    for key, _, rx in HABITS:
        if key == 'the-heavy':
            if len(re.findall(r'\bTHE\b', plain)) >= 3 or (is_name and plain.startswith('THE ')): out.append(key)
        elif key == 'dense':
            if len(plain.split()) > 25: out.append(key)
        elif rx.search(plain):
            if key == 'aphorism' and len(plain.split()) > 8: continue
            out.append(key)
    return out


def slug(s):
    w = re.sub(r'\{\d+\}', 'n', s)
    w = re.sub(r"[^A-Z0-9]+", '_', w.upper()).strip('_').lower()
    return ('_'.join(w.split('_')[:6]) or 'x')[:40]


def where_note(lit):
    if lit['table']:
        return '%s[%s]' % (lit['table'], lit['index'])
    return '%s()' % (lit['func'] or '?')


def font_rows(name, h):
    """The game's own glyphs, as hex, 256 of them: the editor draws with these."""
    src = open(os.path.join(ROOT, 'src/font_vga.c'), encoding='latin-1').read()
    body = src[src.index(name):]
    body = body[:body.index('};')]
    rows = re.findall(r'\{([^{}]*)\}', body)
    return ''.join(''.join('%02x' % int(v, 16) for v in r.split(',')) for r in rows[:256])


FIELD_POS = {('KINDS', 'lore'): 2, ('BOSSES', 'lore'): 1, ('CLASSES', 'story'): 1, ('CLASSES', 'gift'): 2, ('ALIGN', 'does'): 5}


def table_wraps():
    """Where a table's text is wrapped into a box of known columns and lines: {(table, field or None): [cols, lines]}."""
    src = open(os.path.join(ROOT, 'src/scene_crawl.c'), encoding='latin-1').read()
    out = {}
    for m in re.finditer(r'text_wrap(?:_ex)?\(f, [^,]+, [^,]+, ([^,]+), (\d+), [^,]+, (\d+)', src):
        arg, cols, lines = m.group(1).strip(), int(m.group(2)), int(m.group(3))
        t = re.fullmatch(r'([A-Z_]+)\[[^\]]*\](?:\.(\w+))?', arg)
        if arg == 'B->lore': key = ('BOSSES', 1)
        elif t and t.group(2): key = (t.group(1), FIELD_POS.get((t.group(1), t.group(2))))
        elif t: key = (t.group(1), None)
        else: continue
        if key in out: out[key] = [min(out[key][0], cols), min(out[key][1], lines)]
        else: out[key] = [cols, lines]
    return out


def build(outdir):
    entries = {}
    wraps = table_wraps()
    for path in FILES:
        for lit in walk(path):
            if not player_facing(lit):
                continue
            tmpl, fills = blanks(lit)
            label, fits = drawn(lit)
            wrap = fits if isinstance(fits, list) else None
            if lit['table'] and not wrap:
                wrap = wraps.get((lit['table'], lit['arg'] if lit.get('depth', 1) > 1 else None))
                if wrap: label = 'wrapped text'
            if wrap: fits = wrap[0]
            key = tmpl
            e = entries.get(key)
            loc = dict(file=path, line=lit['line'], where=where_note(lit), drawn=label)
            if e is None:
                is_name = bool(lit['table']) and len(tmpl.split()) <= 6
                h = hashlib.sha1(tmpl.encode('latin-1')).hexdigest()[:4]
                e = entries[key] = dict(id='%s-%s' % (slug(tmpl), h), mine=tmpl, blanks=fills, section=section(lit),
                                        drawn=label, fits=fits, wrap=wrap, centre=label in ('caption', 'card line', 'wrapped text, centred'), locs=[], habits=habits(tmpl, is_name),
                                        keys=bool(re.search(r'\b(ENTER|ESC|TAB|SPACE|CLICK|W/S|A/D|WASD|1-9|[EQBCLM])\b', tmpl)) and label.startswith('hint'),
                                        words=len(re.sub(r'\{\d+\}', '', tmpl).split()))
            else:
                if fits is not None and (e['fits'] is None or fits < e['fits']): e['fits'] = fits
                if wrap and not e['wrap']: e['wrap'] = wrap
                if not e['blanks'] and fills: e['blanks'] = fills
                if e['drawn'].startswith('passed to') and not label.startswith('passed to'): e['drawn'] = label
                if e['section'] == 'In play': e['section'] = section(lit)
            e['locs'].append(loc)
    out = sorted(entries.values(), key=lambda e: (e['section'], e['locs'][0]['file'], e['locs'][0]['line']))
    seen = {}
    for e in out:                                # ids unique, however the slugs fall
        if e['id'] in seen:
            seen[e['id']] += 1; e['id'] += '-%d' % seen[e['id']]
        else:
            seen[e['id']] = 1
    os.makedirs(outdir, exist_ok=True)
    meta = dict(font8=font_rows('font_bios8', 8), font16=font_rows('font_vga16', 16), version=open(os.path.join(ROOT, 'VERSION')).read().strip(), files=FILES, screen=SCREEN,
                habits=[dict(key=k, means=m) for k, m, _ in HABITS])
    with open(os.path.join(outdir, 'catalogue.json'), 'w') as f:
        json.dump(dict(meta=meta, entries=out), f, separators=(',', ':'))
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import profiles
    with open(os.path.join(outdir, 'profiles.json'), 'w') as f:
        json.dump(profiles.build(out), f, separators=(',', ':'))
    return out


if __name__ == '__main__':
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'build', 'words')
    out = build(outdir)
    by = {}
    for e in out: by[e['section']] = by.get(e['section'], 0) + 1
    print('%d strings, %d words' % (len(out), sum(e['words'] for e in out)))
    for k, v in sorted(by.items(), key=lambda kv: -kv[1]): print('  %5d  %s' % (v, k))
