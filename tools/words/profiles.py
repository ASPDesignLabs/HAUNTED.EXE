"""The things with a name and a nature: creatures, bosses, classes, spells, runes and gifts (docs/PLAN-words.md).

Read from the game's own tables, so the facts are the code's; the line on what each does now is written
by hand from the code and CLAUDE.md, and wants updating when the behaviour changes. The editor shows
these beside the owner's trait fields, so a description and the mechanics can be made to agree.
"""
import os
import re


def keyname(s):
    return re.sub(r'[^a-z0-9_.-]', '', s.lower().replace(' ', '_'))

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ELEM = ['FIRE', 'FROST', 'LIGHTNING', 'POISON', 'SPIRIT', 'PHYSICAL']

# What each creature does now, in a sentence or three (from the code; keep it true)
CREATURE_NOW = {
    'SKEL': 'Throws bones from range; a long throw spreads wider. Hunts round corners once alerted and pulls doors open. Some keep to their room (ossuaries, burial and knights\' vaults) until woken. Drops bones for the spear.',
    'GHOST': 'Drifts through walls and does no harm. Only Spirit Mastery\'s spirit damage can hurt it; slain, it gives souls. How many come depends on Luck.',
    'BAT': 'Fast and erratic; flies at you and bites. From rank II it latches on instead and drains souls until you shake it off by turning and strafing, or throw it with Blink or Swift Dodge.',
    'WSKULL': 'Set in a wall, asleep. Wakes with a scream when you stand within three cells facing it, gathers fire for three seconds, then spits a fire bolt. One hit breaks it.',
    'SHIELD': 'A skeleton with a shield that comes in close to strike. Opens doors.',
    'SPIDER': 'Sits in a corner web and does not move; bites with poison when you are close.',
    'SHAMBLER': 'Waits under turned earth before a grave and rises when you come near. Slow; grabs, which slows you.',
    'DROWNED': 'Waits in a stirring puddle and rises when you come near. Grabs and slows you; dies by bursting into a foul pool that poisons.',
    'HOUND': 'Comes in pairs. Fast; bites, then darts back out of reach and comes again.',
    'WRAITH': 'Comes once a level after four minutes, from the far end, down the hunt field, opening doors; torches gutter as it passes. Cannot be killed: fire, a wisp\'s light close by and Dread drive it back for five seconds. Its touch chills and slows. Never on a boss level.',
    'HUNTER': 'Waits upside down on the ceiling and drops when you come close, then leaps at you; one leap in three poisons.',
    'GHOUL': 'Leaps at you. When it loses you it feeds on bones: it mends while it eats, and takes double damage.',
    'KNIGHT': 'Comes with two bone hounds after its first level. A 0.9 s wind-up, then a heavy blow. Sheds plate as it is hurt; each stage is quicker, and fire bites harder once the plate is off.',
    'BROOD': 'Lays egg sacs as she goes; the sacs split into spiderlings. When the last mother dies, her sacs die too.',
    'LING': 'Tiny and fast; bites once and comes in numbers.',
    'SAC': 'Laid by a brood mother; splits into spiderlings. Not counted as a kill.',
    'SHADE': 'Visible only in light: torches, fires, a wisp\'s glow, a spell going by, and for a moment after it touches you. Its touch takes souls, or more health when you have none. The dream cannot aim at it in the dark.',
    'IMP': 'Starts beside magma and keeps its distance, lobbing embers that set the floor alight where they land.',
    'DEACON': 'Walks with two or three skeletons and keeps back. Chants over remains or bone heaps until a skeleton gets up, four at most. The dream aims at it first.',
    'GARGOYLE': 'A statue on a plinth until you pass within three cells or hit it; then it flies over what stands on the floor and swoops.',
}
BOSS_NOW = {
    'KNIGHT': 'Level 10. Sheds plate by waves, quicker each time. Swift Dodge (darts to close in, to flee when hurt or burning, aside from a shot), the Mirror Rune (one missile in seven comes back), Rockfall and the Sundering Arc. Waves of skeletons, shield-bearers, hounds and a knight.',
    'SPIDER': 'Level 20. Climbs to the ceiling every few seconds, out of reach, crosses it and drops to leap. The Mend Rune heals an eighth after heavy harm; the Tangled Web holds you until you burn through it; Egg Shot hatches four small hunters; one more comes out of a burrow every 15 s. Bile outlets in the floor.',
    'DEACON': 'Level 30, twice a deacon\'s size. The Bone Ward takes blows first and grows back; Grave Call raises shamblers from the chapel\'s heaps; the Litany is two seconds of green light, then a fifth of your health if he can see you (timber props hide you). Lines, rings and crosses on the floor.',
    'WRAITH': 'Level 40. Can be hurt only near a lit brazier (four cells) or a wisp (three), and not while in the stone. It gutters braziers as it passes; E or a wisp kindles them. Time Slip slows a missile about to strike it; Wraithwalk takes it into a wall and out beside you. Its touch chills and slows. Voids and grasping hands.',
}
SPELL_NOW = {
    'FIRE BOLT': 'The first line\'s spell: a bolt. Free at the first tier.',
    'FROST SHARD': 'A shard that slows what it hits.',
    'LIGHTNING': 'Strikes what you aim at at once; no flight.',
    'BONE SPEAR': 'Goes through everything in a line; costs a bone, not spirit.',
    'SOUL DRAIN': 'A beam that hurts and heals you for some of it.',
    'HELLFIRE': 'An older spell, kept for old saves: the floor burns where it lands.',
    'SEEKER WISPS': 'An older spell: wisps that home in on what they hunt, round corners.',
    'BLIGHT FLASK': 'Lobbed; bursts and poisons what it splashes.',
    'BFS9000': 'Spends souls; kills everything in sight. A boss loses 8%.',
}
RUNE_NOW = {
    'BONE WARD': 'Takes the next blows. Exquisite: bursts into shards when it breaks.',
    'BLINK': 'Steps you away from harm. Exquisite: leaves a double the dead go for.',
    'MIRROR VEIL': 'Sends missiles back. Exquisite: home to the thrower at twice the strength.',
    'MEND': 'Heals over time. Exquisite: past full it becomes a small ward.',
    'PURIFY': 'Cures poison and slowing. Exquisite: mends 2% a second.',
    'FROST NOVA': 'Freezes everything round you. Exquisite: the frozen take double.',
    'DREAD': 'The undead flee. Exquisite: the living too.',
    'TIME SLIP': 'Slows everything but you. Exquisite: what it slows takes a quarter more.',
    'SOUL REAPING': 'Triple souls for a while. Exquisite: each kill heals a twentieth of your health.',
}
GIFT_NOW = {
    'SWIFT DODGE': 'From the knight. Tap a direction twice: a dart of two to five cells, untouchable for a quarter second.',
    'TANGLED WEB': 'From the spider. Spits a web that holds a creature until it claws free.',
    'GRAVE CALL': 'From the Deacon. Raises the remains of something you slew to fight for you for 12-20 s.',
    'WRAITHWALK': 'From the Wraith. For a while creatures lose you, missiles pass and you walk through them.',
}
CORNER_NOW = {
    'SECOND WIND': 'A blow that would kill leaves you at half health, untouchable two seconds. Once a run of ten, or for a coin.',
    'PREMONITION': 'A second\'s warning, in purple on the floor, before each hazard.',
    'CARTOGRAPHER': 'The map comes drawn whole, but for keys and secrets.',
    'LANTERN OF THE DEAD': 'A faint light round you; shades show in it and the Wraith keeps further off.',
    'GRAVE SENSE': 'Creatures within ten cells show on the map.',
}

FIELDS = {
    'creatures': [
        dict(key='harm', label='Damage it deals', type='elements'),
        dict(key='weak', label='Weak to', type='elements'),
        dict(key='resist', label='Resists', type='elements'),
        dict(key='attack', label='How it attacks', type='text', hint='claws, a bite, a thrown bone, a spat bolt, a touch...'),
        dict(key='projectile', label='What it throws or shoots', type='text', hint='nothing, bones, fire, venom...'),
        dict(key='approach', label='How it comes at you', type='text', hint='rushes, keeps its distance, waits in ambush, hunts in pairs...'),
        dict(key='moves', label='How it moves', type='text', hint='walks, flies, through walls, on the ceiling...'),
        dict(key='lives', label='Where it is found', type='text'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'bosses': [
        dict(key='harm', label='Damage it deals', type='elements'),
        dict(key='weak', label='Weak to', type='elements'),
        dict(key='resist', label='Resists', type='elements'),
        dict(key='attacks', label='Attacks and abilities', type='long'),
        dict(key='phases', label='How the fight changes', type='long'),
        dict(key='arena', label='Its arena', type='text'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'classes': [
        dict(key='style', label='How they fight', type='text'),
        dict(key='gifts', label='What their past should give them', type='long'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'lines': [
        dict(key='delivery', label='How the spell is cast', type='text', hint='a bolt, a beam, a lob, a strike, held...'),
        dict(key='projectile', label='What it looks like in flight', type='text'),
        dict(key='effect', label='What it does to what it hits', type='long'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'spells': [
        dict(key='element', label='Element', type='elements'),
        dict(key='delivery', label='How it is cast', type='text'),
        dict(key='effect', label='What it does', type='long'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'runes': [
        dict(key='effect', label='What it should do', type='long'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
    'gifts': [
        dict(key='effect', label='What it should do', type='long'),
        dict(key='notes', label='Notes for the mechanics', type='long'),
    ],
}


def strings_in(chunk):
    """The literals in a piece of C, adjacent ones joined."""
    out, last_end = [], None
    for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', chunk):
        s = m.group(1).replace('\\"', '"')
        if last_end is not None and chunk[last_end:m.start()].strip() == '':
            out[-1] += s
        else:
            out.append(s)
        last_end = m.end()
    return out


def block(src, head):
    i = src.index(head)
    j = src.index('\n};', i)
    return src[i:j]


def mult(x):
    return ('%.2f' % x).rstrip('0').rstrip('.')


def build(entries):
    by_text = {}
    for e in entries:
        by_text.setdefault(e['mine'], e['id'])

    def ids(*texts):
        return [by_text[t] for t in texts if t in by_text]

    src = open(os.path.join(ROOT, 'src/scene_crawl.c'), encoding='latin-1').read()
    gear = open(os.path.join(ROOT, 'src/crawl_gear.c'), encoding='latin-1').read()
    groups = []

    # ---- creatures ----
    items = []
    kb = re.sub(r'/\*.*?\*/', '', block(src, 'static const Kind KINDS[] = {'), flags=re.S)
    flagw = dict(KF_UNDEAD='undead', KF_SOLID='solid', KF_SPIRIT='a spirit: nothing you carry hurts it', KF_UNIQUE='only one',
                 KF_DOORS='opens doors', KF_MELEE='comes right up to you', KF_UNCOUNTED='not counted as a kill')
    for m in re.finditer(r'\[MOB_(\w+)\] = \{(.*?)\},\s*(?=\[MOB_|$)', kb, re.S):
        key, body = m.group(1), m.group(2)
        name, plural, lore = strings_in(body)[:3]
        rest = re.sub(r'"((?:[^"\\]|\\.)*)"', '', body)
        weak = re.search(r'\{([^}]*)\}', rest).group(1)
        weak = [float(x.strip().rstrip('f')) for x in weak.split(',') if x.strip()] if weak.strip() else []
        weak = (weak + [0.0] * 6)[:6] if weak else [0.0] * 6
        flat = [x.strip() for x in re.sub(r'\{[^}]*\}', 'W', rest).split(',') if x.strip()]
        # spr, spr2, side, side2, family, debut, top, gild, flags, sfx, remains, hp, speed, scale, rad, W, harm
        debut, top, gild, flags = flat[5], flat[6], flat[7], flat[8]
        hp, speed, harm = flat[11], flat[12], flat[16] if len(flat) > 16 else '0'
        facts = [['First met', 'level %s' % debut], ['Ranks', 'up to %s' % top.replace('RK_', '')]]
        if gild == '1': facts.append(['Gilded', 'one may be, from level 7'])
        fl = [flagw[f.strip()] for f in flags.split('|') if f.strip() in flagw]
        if fl: facts.append(['Nature', ', '.join(fl)])
        facts.append(['Health', '%s at rank I, level 1' % hp.rstrip('f').rstrip('e9') if 'e9' not in hp else 'cannot be killed'])
        facts.append(['Pace', mult(float(speed.rstrip('f')))])
        if any(weak):
            w = ['%s x%s' % (ELEM[k], mult(v)) for k, v in enumerate(weak) if v > 1.0]
            r = ['%s x%s' % (ELEM[k], mult(v)) for k, v in enumerate(weak) if v < 1.0]
            if w: facts.append(['Weak to', ', '.join(w)])
            if r: facts.append(['Resists', ', '.join(r)])
        hs = [e for e in ELEM if 'H_' + {'LIGHTNING': 'LIGHT', 'PHYSICAL': 'PHYS'}.get(e, e) in harm]
        facts.append(['Damage it deals', ', '.join(hs) if hs else 'none'])
        items.append(dict(key='creature.' + key.lower(), name=name, strings=ids(name, plural, lore), facts=facts,
                          now=CREATURE_NOW.get(key, ''), weak=weak, harm=hs))
    groups.append(dict(key='creatures', title='Creatures', fields=FIELDS['creatures'], items=items))

    # ---- bosses ----
    items = []
    bb = block(src, 'static const BossDef BOSSES[NBOSS] = {')
    for m in re.finditer(r'\[BOSS_(\w+)\] = \{(.*?)(?=\[BOSS_|\Z)', bb, re.S):
        key, body = m.group(1), m.group(2)
        name, lore, scrip, short = strings_in(body)[:4]
        kind = re.search(r'MOB_(\w+)', body).group(1)
        nums = re.findall(r'MOB_\w+, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f', body)[0]
        level = {'KNIGHT': 10, 'SPIDER': 20, 'DEACON': 30, 'WRAITH': 40}.get(key)
        facts = [['Level', str(level)], ['A stronger', kind.lower().replace('_', ' ')],
                 ['Health', '%s rank-I skeleton knights of its level' % mult(float(nums[0]))],
                 ['Size, pace, blow', 'x%s, x%s, x%s its kind\'s' % tuple(mult(float(x)) for x in nums[1:])]]
        items.append(dict(key='boss.' + key.lower(), name=name, strings=ids(name, short, lore, scrip), facts=facts, now=BOSS_NOW.get(key, '')))
    groups.append(dict(key='bosses', title='Bosses', fields=FIELDS['bosses'], items=items))

    # ---- classes ----
    items = []
    cb = block(src, 'static const ClassDef CLASSES[NELEM] = {')
    short = strings_in(re.search(r'CLASS_SHORT\[NELEM\] = \{(.*?)\};', src).group(1))
    parts = re.split(r'\n    \{ "', cb)[1:]
    for k, p in enumerate(parts):
        name, story, gift = strings_in('"' + p)[:3]
        items.append(dict(key='class.' + keyname(short[k]), name=name,
                          strings=ids(name, short[k], story, gift), facts=[['Element', ELEM[k]]],
                          now='Its line is open from the start, and it brings its gifts: ' + gift.capitalize()))
    groups.append(dict(key='classes', title='Classes', fields=FIELDS['classes'], items=items))

    # ---- the elemental lines (the Alignment) ----
    items = []
    ab = block(src, 'static const ALine ALIGN[NELEM] = {')
    for k, part in enumerate(re.split(r'\n    \{ "', ab)[1:]):
        nodes = re.findall(r'\{ "([^"]+)", (\d+), (NK_\w+), [^,]+, [^,]+, "((?:[^"\\]|\\.)*)" \}', part)
        texts = []
        for nm, cost, kind, does in nodes:
            texts += [nm, does]
        opening = nodes[0][3] if nodes else ''
        facts = [['Element', ELEM[k]], ['Nodes', ', '.join(n[0] for n in nodes[1:])]]
        items.append(dict(key='line.' + ELEM[k].lower(), name=ELEM[k], strings=ids(*texts), facts=facts,
                          now=opening.capitalize() + ' Ultimates: ' + ', '.join(n[0].title() for n in nodes if n[2] == 'NK_ULT') + '.'))
    groups.append(dict(key='lines', title='Spell lines', fields=FIELDS['lines'], items=items))

    # ---- spells, runes, gifts, cornerstones: tables of names ----
    def table(text, name):
        return strings_in(re.search(re.escape(name) + r'\[[A-Z_]+\] = \{(.*?)\};', text, re.S).group(1))

    atk_n, atk_b = table(gear, 'ATK_NAME'), table(gear, 'ATK_BLURB')
    elem = re.search(r'ATK_ELEM\[NATK\] = \{(.*?)\};', gear).group(1).split(',')
    items = []
    for k, n in enumerate(atk_n):
        el = elem[k].strip().replace('EL_', '')
        el = {'LIGHT': 'LIGHTNING', 'PHYS': 'PHYSICAL'}.get(el, el)
        items.append(dict(key='spell.' + keyname(n), name=n, strings=ids(n, atk_b[k]),
                          facts=[['Element', el if el != '-1' else 'none']], now=SPELL_NOW.get(n, '')))
    groups.append(dict(key='spells', title='Spells', fields=FIELDS['spells'], items=items))

    def_n, def_b = table(gear, 'DEF_NAME'), table(gear, 'DEF_BLURB')
    def_s = table(src, 'DEF_SHORT')
    items = [dict(key='rune.' + keyname(n), name=n, strings=ids(n, def_s[k], def_b[k]), facts=[],
                  now=RUNE_NOW.get(n, '')) for k, n in enumerate(def_n)]
    groups.append(dict(key='runes', title='Runes', fields=FIELDS['runes'], items=items))

    ba = table(src, 'BA_NAME')
    cs_n, cs_d = table(src, 'CS_NAME'), table(src, 'CS_DOES')
    ab_n = [x for x in table(gear, 'ABIL_NAME') if x]
    items = [dict(key='gift.' + keyname(n), name=n, strings=ids(n), facts=[['From', 'a boss, at the Hexed Totem']],
                  now=GIFT_NOW.get(n, '')) for n in ba]
    items += [dict(key='corner.' + keyname(n), name=n, strings=ids(n, cs_d[k]), facts=[['From', 'a cornerstone, at each tenth Level of Atonement']],
                   now=CORNER_NOW.get(n, '')) for k, n in enumerate(cs_n)]
    items += [dict(key='ability.' + keyname(n), name=n, strings=ids(n), facts=[['From', 'a piece of gear that rolls it']], now='')
              for n in ab_n]
    groups.append(dict(key='gifts', title='Gifts and abilities', fields=FIELDS['gifts'], items=items))
    return dict(elements=ELEM, groups=groups)
