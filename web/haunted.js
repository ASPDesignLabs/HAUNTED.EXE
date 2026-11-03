// HAUNTED.EXE in the browser: the page's side of the shell (docs/PLAN-web.md). haunted-core.js is the game,
// compiled by Emscripten from the same sources as the Windows build, with src/web_main.c as its door. This
// file is what win_main.c is on Windows: the loop, the picture, the sound, the keys, the mouse, the pad, the
// slots (in localStorage, as the registry keeps them there) and the settings.
(() => {
  "use strict";

  const PREFIX = "haunted.exe/";                     // the origin is shared with the organisation's other pages
  const VERSION = document.documentElement.dataset.version || "";
  const AUDIO_AHEAD = 0.1;                           // seconds of sound kept queued
  const $ = (id) => document.getElementById(id);
  const stage = $("stage"), canvas = $("screen"), plate = $("plate");
  const ctx2d = canvas.getContext("2d", { alpha: false });

  // ---- storage --------------------------------------------------------------------------------------
  const store = {
    get(k) { try { return localStorage.getItem(PREFIX + k); } catch (e) { return null; } },
    set(k, v) { try { localStorage.setItem(PREFIX + k, v); return true; } catch (e) { return false; } },
    del(k) { try { localStorage.removeItem(PREFIX + k); } catch (e) { /* nothing to do */ } },
  };
  const session = {
    get(k) { try { return sessionStorage.getItem(PREFIX + k); } catch (e) { return null; } },
    set(k, v) { try { sessionStorage.setItem(PREFIX + k, v); } catch (e) { /* nothing to do */ } },
    del(k) { try { sessionStorage.removeItem(PREFIX + k); } catch (e) { /* nothing to do */ } },
  };
  const toB64 = (u8) => { let s = ""; for (let i = 0; i < u8.length; i += 4096) s += String.fromCharCode.apply(null, u8.subarray(i, i + 4096)); return btoa(s); };
  const fromB64 = (s) => { const b = atob(s), u = new Uint8Array(b.length); for (let i = 0; i < b.length; i++) u[i] = b.charCodeAt(i); return u; };

  // The settings dialog's, as the registry has them on Windows, and the mouse's sensitivity.
  const DEFAULTS = { sound: 1, volume: 60, brightness: 85, haunt: 1, crawl: 1, stay: 4, shift: 1, pad: 1, sens: 1.0 };
  function settings() {
    let s = {};
    try { s = JSON.parse(store.get("settings") || "{}") || {}; } catch (e) { s = {}; }
    return Object.assign({}, DEFAULTS, s);
  }

  // The slots: the ledger's three pages, then the progress, the mark and the prefs (the pad's sprint button).
  const SLOT_NAMES = ["slot1", "slot2", "slot3", "progress", "mark", "prefs"];

  // ---- the game ------------------------------------------------------------------------------------
  let M = null;                                      // the module
  let KEY = {}, PADB = {};                           // name -> number, from the game's own lists
  let booted = false, fleeing = false, importing = false;
  let state = 0;                                     // APP_AUTO, APP_HOLDING, APP_PLAYER
  const APP_PLAYER = 2;
  let cfg = settings();

  const coreReady = HauntedCore({
    print: (s) => console.log(s),
    printErr: (s) => console.log(s),
  }).then((mod) => {
    M = mod;
    const keys = M.UTF8ToString(M._web_key_names()).split(" ");
    keys.forEach((n, i) => { KEY[n] = i; });
    M.UTF8ToString(M._web_pad_names()).split(" ").forEach((n, i) => { PADB[n] = i; });
    return mod;
  });

  function loadSlots() {
    const buf = M._web_slot_buf();
    for (let i = 0; i < SLOT_NAMES.length && i < M._web_slots(); i++) {
      const v = store.get(SLOT_NAMES[i]);
      if (!v) continue;
      let u;
      try { u = fromB64(v); } catch (e) { continue; }
      if (!u.length || u.length > 16384) continue;
      M.HEAPU8.set(u, buf);
      M._web_slot_load(i, u.length);
    }
  }
  function flushSlots() {
    if (!M || !booted || importing) return;
    const buf = M._web_slot_buf();
    for (let i = 0; i < SLOT_NAMES.length && i < M._web_slots(); i++) {
      const n = M._web_slot_take(i);
      if (n > 0 && !store.set(SLOT_NAMES[i], toB64(M.HEAPU8.subarray(buf, buf + n))))
        console.warn("HAUNTED.EXE: the browser refused to keep " + SLOT_NAMES[i]);
    }
  }

  // ---- sound ---------------------------------------------------------------------------------------
  const snd = { ctx: null, node: null, rate: 44100, told: 0, toldAt: 0, sent: 0, ok: false };
  async function soundStart() {
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
    try { snd.ctx = new AC({ sampleRate: 44100, latencyHint: "interactive" }); }
    catch (e) { try { snd.ctx = new AC({ latencyHint: "interactive" }); } catch (e2) { return; } }
    try {
      await snd.ctx.audioWorklet.addModule("audio.js");
      snd.node = new AudioWorkletNode(snd.ctx, "haunted-sound", { numberOfInputs: 0, outputChannelCount: [2], processorOptions: { rate: snd.rate } });
      snd.node.port.onmessage = (e) => { snd.told = e.data; snd.toldAt = snd.ctx.currentTime; snd.sent = 0; };
      snd.node.connect(snd.ctx.destination);
      snd.ok = true;
    } catch (e) { console.warn("HAUNTED.EXE: no sound in this browser", e); }
    if (snd.ctx.state !== "running") snd.ctx.resume().catch(() => {});
  }
  // the game's sound clock runs on the samples rendered, as on Windows: so it is always rendered, queued if it can be
  function soundPump(dt) {
    if (!snd.ok || snd.ctx.state !== "running") { M._web_audio(Math.min(8192, Math.round(dt * snd.rate))); return; }
    const gone = Math.max(0, (snd.ctx.currentTime - snd.toldAt) * snd.rate);
    const have = Math.max(0, snd.told + snd.sent - gone);
    let need = Math.round(AUDIO_AHEAD * snd.rate - have);
    while (need > 0) {
      const n = Math.min(need, 4096);
      const p = M._web_audio(n) >> 2;
      const chunk = M.HEAPF32.slice(p, p + n * 2);
      snd.node.port.postMessage(chunk, [chunk.buffer]);
      snd.sent += n; need -= n;
    }
  }
  function soundWake() { if (snd.ctx && snd.ctx.state !== "running") snd.ctx.resume().catch(() => {}); }

  // ---- the picture ---------------------------------------------------------------------------------
  let img = null, scale = 1, shown = { w: 0, h: 0, cw: 0, ch: 0 };
  function fit() {
    const w = M ? M._web_width() : 320, h = M ? M._web_height() : 200;
    const dpr = window.devicePixelRatio || 1;
    const aw = stage.clientWidth * dpr, ah = stage.clientHeight * dpr;
    let s = Math.floor(Math.min(aw / w, ah / h)), cw, ch;
    if (s >= 1) { cw = (w * s) / dpr; ch = (h * s) / dpr; }      // whole device pixels, every one alike
    else { const f = Math.min(aw / w, ah / h); cw = (w * f) / dpr; ch = (h * f) / dpr; s = f; }
    scale = s / dpr;
    if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; img = ctx2d.createImageData(w, h); }
    if (shown.cw !== cw || shown.ch !== ch) { canvas.style.width = cw + "px"; canvas.style.height = ch + "px"; }
    shown = { w, h, cw, ch };
  }
  function draw() {
    const p = M._web_frame(), w = M._web_width(), h = M._web_height();
    if (w !== shown.w || h !== shown.h || !img) fit();
    img.data.set(M.HEAPU8.subarray(p, p + w * h * 4));
    ctx2d.putImageData(img, 0, 0);
    if (cfg.shift) {                                             // the slow drift, kept inside the black margin
      const mx = (stage.clientWidth - shown.cw) / 2, my = (stage.clientHeight - shown.ch) / 2;
      const dx = Math.max(-mx, Math.min(mx, M._web_shift_x() * scale)), dy = Math.max(-my, Math.min(my, M._web_shift_y() * scale));
      canvas.style.transform = "translate(" + dx + "px," + dy + "px)";
    }
  }
  window.addEventListener("resize", () => { if (M) fit(); });

  // ---- keys and the mouse --------------------------------------------------------------------------
  // KeyboardEvent.code is where the key is, not what it prints, so Backquote is the key left of 1 on any layout,
  // as the console wants. Ctrl is the browser's (Ctrl+W, Ctrl+1-9) and never reaches the game: Space casts.
  const CODE = {
    ArrowUp: "UP", ArrowDown: "DOWN", ArrowLeft: "LEFT", ArrowRight: "RIGHT", KeyW: "W", KeyA: "A", KeyS: "S", KeyD: "D",
    KeyE: "E", KeyQ: "Q", Space: "SPACE", ShiftLeft: "SHIFT", ShiftRight: "SHIFT", Tab: "TAB", Escape: "ESC",
    Enter: "ENTER", NumpadEnter: "ENTER", KeyH: "H", KeyY: "Y", KeyN: "N", KeyB: "B", KeyL: "L", KeyC: "C", KeyM: "M",
    Backquote: "CONSOLE", Backspace: "BACK", PageUp: "PGUP", PageDown: "PGDN",
  };
  for (let d = 1; d <= 9; d++) { CODE["Digit" + d] = String(d); CODE["Numpad" + d] = String(d); }
  const locked = () => document.pointerLockElement === stage;
  let escAt = -1e9, escSent = -1e9, freeing = false;

  function key(name, down) { if (M && booted && name in KEY) M._web_key(KEY[name], down ? 1 : 0); }
  function takeMouse() {
    if (locked() || !stage.requestPointerLock) return;
    let r;
    try { r = stage.requestPointerLock({ unadjustedMovement: true }); } catch (e) { r = null; }
    if (r && r.catch) r.catch(() => { try { const q = stage.requestPointerLock(); if (q && q.catch) q.catch(() => {}); } catch (e) { /* not now */ } });
  }
  function freeMouse() { if (locked()) { freeing = true; document.exitPointerLock(); } }

  window.addEventListener("keydown", (e) => {
    if (!booted) return;
    soundWake();
    if (e.ctrlKey || e.metaKey || e.altKey) return;             // the browser's own
    const name = CODE[e.code];
    if (!name) return;
    if ($("panel").open) return;
    e.preventDefault();
    if (name === "ESC") {
      if (performance.now() - escSent < 250) return;            // the press that freed the mouse, already sent
      escAt = performance.now();
    }
    if (state === APP_PLAYER) {
      if (!locked() && !e.repeat) takeMouse();
      if (!e.repeat) key(name, true);
      if (e.key && e.key.length === 1 && /[a-z0-9]/i.test(e.key)) M._web_text(e.key.charCodeAt(0));
    } else if (name === "H") {
      key("H", true);                                            // only H is heard while the dream plays
      if (e.repeat && !locked() && M._web_state() === APP_PLAYER) takeMouse();   // held through the takeover: the mouse with it
    }
  });
  window.addEventListener("keyup", (e) => {
    const name = CODE[e.code];
    if (!name || !booted) return;
    e.preventDefault();
    key(name, false);
  });
  stage.addEventListener("mousedown", (e) => {
    if (!booted) return;
    soundWake();
    if (state !== APP_PLAYER) return;
    if (!locked()) { takeMouse(); return; }                      // the click that takes the mouse casts nothing
    if (e.button === 0) key("MOUSE1", true);
    if (e.button === 2) key("MOUSE2", true);
  });
  window.addEventListener("mouseup", (e) => {
    if (e.button === 0) key("MOUSE1", false);
    if (e.button === 2) key("MOUSE2", false);
  });
  stage.addEventListener("contextmenu", (e) => e.preventDefault());
  document.addEventListener("mousemove", (e) => {
    if (M && booted && locked() && state === APP_PLAYER) M._web_mouse(e.movementX * cfg.sens, e.movementY * cfg.sens);
  });
  let wheelAcc = 0;
  stage.addEventListener("wheel", (e) => {
    if (!booted || state !== APP_PLAYER) return;
    e.preventDefault();
    wheelAcc += e.deltaMode === 1 ? e.deltaY / 3 : e.deltaMode === 2 ? e.deltaY * 3 : e.deltaY / 100;
    while (wheelAcc <= -1) { M._web_wheel(1); wheelAcc += 1; }   // + away from you
    while (wheelAcc >= 1) { M._web_wheel(-1); wheelAcc -= 1; }
  }, { passive: false });

  // Esc frees the mouse before the page hears it: freed while a person plays, and not by the page itself or by
  // going to another window, it counts as Esc (which opens the question in play, and goes back in a menu).
  document.addEventListener("pointerlockchange", () => {
    document.body.classList.toggle("locked", locked());
    if (locked()) return;
    if (freeing) { freeing = false; return; }
    if (!booted || state !== APP_PLAYER) return;
    setTimeout(() => {
      if (document.hidden || !document.hasFocus() || performance.now() - escAt < 300) return;
      escSent = performance.now();
      key("ESC", true);
      setTimeout(() => key("ESC", false), 50);
    }, 60);
  });

  function away() { if (M && booted) M._web_focus(0); wheelAcc = 0; }
  window.addEventListener("blur", away);
  document.addEventListener("visibilitychange", () => { if (document.hidden) away(); else { wakeLock(); soundWake(); } });

  // ---- the controller ------------------------------------------------------------------------------
  // The Gamepad API's standard layout is XInput's; the dead zones are XInput's own, as on Windows.
  const STD = ["A", "B", "X", "Y", "LB", "RB", "LT", "RT", "VIEW", "MENU", "LS", "RS", "UP", "DOWN", "LEFT", "RIGHT"];
  let padTouched = 0, motor = { lo: 0, hi: 0, at: 0 };
  function stick(x, y, dead) {
    const m = Math.hypot(x, y);
    if (m <= dead) return [0, 0];
    const k = (Math.min(m, 1) - dead) / (1 - dead) / m;
    return [Math.max(-1, Math.min(1, x * k)), Math.max(-1, Math.min(1, y * k))];
  }
  function pads() {
    if (!navigator.getGamepads) return [];
    return Array.from(navigator.getGamepads()).filter((g) => g && g.connected);
  }
  function pollPad() {
    const list = cfg.pad ? pads() : [];
    if (!list.length) { M._web_pad(0, 0, 0, 0, 0, 0, 0, 0); return 0; }
    let bits = 0, lx = 0, ly = 0, rx = 0, ry = 0, lt = 0, rt = 0;
    for (const g of list) {
      STD.forEach((n, i) => {
        const b = g.buttons[i];
        if (!b) return;
        const v = typeof b === "object" ? b.value : b, pressed = typeof b === "object" ? b.pressed : b > 0.5;
        if (n === "LT" || n === "RT") {
          if (v > 0.33) bits |= 1 << PADB[n];
          if (n === "LT") lt = Math.max(lt, v > 0.1 ? v : 0); else rt = Math.max(rt, v > 0.1 ? v : 0);
        } else if (pressed) bits |= 1 << PADB[n];
      });
      const a = g.axes;
      const [x1, y1] = stick(a[0] || 0, -(a[1] || 0), 7849 / 32767), [x2, y2] = stick(a[2] || 0, -(a[3] || 0), 8689 / 32767);
      if (Math.abs(x1) + Math.abs(y1) > Math.abs(lx) + Math.abs(ly)) { lx = x1; ly = y1; }
      if (Math.abs(x2) + Math.abs(y2) > Math.abs(rx) + Math.abs(ry)) { rx = x2; ry = y2; }
    }
    M._web_pad(1, bits, lx, ly, rx, ry, lt, rt);
    if (bits || lx || ly || rx || ry) padTouched = performance.now();
    rumble(list);
    return bits;
  }
  function rumble(list) {
    const lo = M._web_motor(0), hi = M._web_motor(1), now = performance.now();
    const same = Math.abs(lo - motor.lo) < 0.02 && Math.abs(hi - motor.hi) < 0.02;
    if (same && (lo + hi === 0 || now - motor.at < 150)) return;
    motor = { lo, hi, at: now };
    for (const g of list) {
      const v = g.vibrationActuator;
      if (!v || !v.playEffect) continue;
      if (lo + hi === 0) { if (v.reset) v.reset().catch(() => {}); continue; }
      v.playEffect("dual-rumble", { duration: 200, strongMagnitude: lo, weakMagnitude: hi }).catch(() => {});
    }
  }

  // ---- the screen kept awake, full screen ----------------------------------------------------------
  let wake = null;
  async function wakeLock() {
    if (!booted || wake || document.hidden || !navigator.wakeLock) return;
    try { wake = await navigator.wakeLock.request("screen"); wake.addEventListener("release", () => { wake = null; }); } catch (e) { wake = null; }
  }
  $("full").addEventListener("click", async () => {
    if (document.fullscreenElement) { document.exitFullscreen().catch(() => {}); return; }
    try {
      await document.documentElement.requestFullscreen({ navigationUI: "hide" });
      if (navigator.keyboard && navigator.keyboard.lock) await navigator.keyboard.lock(["Escape"]).catch(() => {});   // Chrome and Edge: Esc is the game's
    } catch (e) { /* refused */ }
  });
  document.addEventListener("fullscreenchange", () => {
    if (!document.fullscreenElement && navigator.keyboard && navigator.keyboard.unlock) navigator.keyboard.unlock();
    if (M) fit();
  });

  // ---- the loop ------------------------------------------------------------------------------------
  let last = 0;
  const perf = { n: 0, step: 0, sound: 0, draw: 0, worst: 0 };   // ms in each part, for the smoke test and the curious
  window.HAUNTED = {
    perf, reset() { Object.assign(perf, { n: 0, step: 0, sound: 0, draw: 0, worst: 0 }); },
    dark() {                                                     // the share of the last frame near black, from what was drawn, not read back
      if (!img) return { w: 0, h: 0, dark: 1 };
      const d = img.data; let n = 0;
      for (let i = 0; i < d.length; i += 4) if (d[i] + d[i + 1] + d[i + 2] < 24) n++;
      return { w: img.width, h: img.height, dark: n / (d.length / 4) };
    },
    state: () => state,
  };
  function frame(ts) {
    if (fleeing) return;
    const dt = last ? Math.min(0.1, (ts - last) / 1000) : 1 / 60;
    last = ts;
    const t0 = performance.now();
    pollPad();
    const quit = M._web_step(dt);
    const t1 = performance.now();
    soundPump(dt);
    flushSlots();
    const t2 = performance.now();
    const was = state;
    state = M._web_state();
    if (state !== was) document.body.classList.toggle("playing", state === APP_PLAYER);
    if (was === APP_PLAYER && state !== APP_PLAYER) freeMouse();  // handed back to the dream
    $("needmouse").hidden = !(state === APP_PLAYER && !locked() && performance.now() - padTouched > 3000);
    draw();
    const t3 = performance.now();
    perf.n++; perf.step += t1 - t0; perf.sound += t2 - t1; perf.draw += t3 - t2;
    perf.worst = Math.max(perf.worst, t3 - t0);
    if (quit) { flee(); return; }
    requestAnimationFrame(frame);
  }
  // Fled to the surface: what is kept is written; back to the dream, saying so.
  function flee() {
    fleeing = true;
    flushSlots();
    freeMouse();
    session.set("fled", "1");
    location.reload();
  }

  // ---- booting -------------------------------------------------------------------------------------
  async function boot() {
    if (booted || !plate.classList.contains("ready")) return;
    plate.classList.remove("ready");
    $("press").textContent = "Loading...";
    await soundStart();                                           // inside the press: the browser allows sound now
    await coreReady;
    if (navigator.storage && navigator.storage.persist) navigator.storage.persist().catch(() => {});
    session.del("fled");
    loadSlots();
    M._web_config(cfg.sound, cfg.volume, cfg.brightness, cfg.haunt, cfg.crawl, Math.round(cfg.stay * 60), cfg.shift, cfg.pad);
    M._web_start();
    booted = true;
    plate.hidden = true;
    document.body.classList.add("booted");
    fit();
    wakeLock();
    requestAnimationFrame(frame);
  }
  function plateReady() {
    plate.classList.add("ready");
    if (session.get("fled")) $("fled").hidden = false;
    const coarse = matchMedia("(pointer: coarse)").matches && !matchMedia("(any-pointer: fine)").matches;
    if (coarse) $("watch").hidden = false;
    $("press").textContent = coarse ? "Tap to boot" : "Press a key or click to boot";
    const go = (e) => { if (e.target.closest && e.target.closest("#bar, #panel, #plate details")) return; boot(); };
    window.addEventListener("keydown", (e) => {                  // a key boots it, but not Tab and Enter about the controls
      if (e.ctrlKey || e.metaKey || e.altKey || $("panel").open) return;
      if (document.activeElement && document.activeElement.closest && document.activeElement.closest("#plate details") && /^(Tab|Enter|Space)$/.test(e.code)) return;
      go(e);
    });
    window.addEventListener("pointerdown", go);
    const padWait = () => {                                      // a pad can boot it too, though the sound waits for a key or a click
      if (booted) return;
      if (pads().some((g) => g.buttons.some((b) => (typeof b === "object" ? b.pressed : b > 0.5)))) { boot(); return; }
      requestAnimationFrame(padWait);
    };
    requestAnimationFrame(padWait);
  }

  // One tab at a time: two would write over each other's ledger.
  if (navigator.locks && navigator.locks.request) {
    navigator.locks.request(PREFIX + "ledger", { ifAvailable: true }, (lock) => {
      if (!lock) { $("busy").hidden = false; $("press").textContent = ""; return null; }
      plateReady();
      return new Promise(() => {});                              // held while the page is open
    });
  } else plateReady();

  // ---- settings and the ledger ---------------------------------------------------------------------
  const panel = $("panel"), form = $("settings");
  const PAD_ORDER = ["LS", "RS", "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "UP", "DOWN", "LEFT", "RIGHT", "VIEW", "MENU"];
  const PAD_SAY = ["Left stick, clicked", "Right stick, clicked", "A", "B", "X", "Y", "Left bumper", "Right bumper", "Left trigger",
                   "Right trigger", "D-pad up", "D-pad down", "D-pad left", "D-pad right", "View", "Menu"];
  function padSprint() { const v = store.get("prefs"); try { return v ? fromB64(v)[0] : -1; } catch (e) { return -1; } }
  function showValues() {
    $("volume-v").textContent = form.volume.value;
    $("brightness-v").textContent = form.brightness.value + "%";
    $("stay-v").textContent = form.stay.value + (form.stay.value === "1" ? " minute" : " minutes");
    $("sens-v").textContent = Number(form.sens.value).toFixed(2) + "x";
  }
  $("open-settings").addEventListener("click", async () => {
    await coreReady;
    flushSlots();
    const s = settings();
    form.sound.checked = !!s.sound; form.volume.value = s.volume; form.brightness.value = s.brightness;
    form.haunt.checked = !!s.haunt; form.crawl.checked = !!s.crawl; form.stay.value = s.stay;
    form.shift.checked = !!s.shift; form.pad.checked = !!s.pad; form.sens.value = s.sens;
    const sel = form.sprint;
    if (!sel.options.length) PAD_ORDER.forEach((n, i) => sel.add(new Option(PAD_SAY[i], String(PADB[n]))));
    const ps = padSprint();
    sel.value = String(ps >= 0 ? ps : PADB.LS);
    showValues();
    freeMouse();
    panel.showModal();
  });
  form.addEventListener("input", showValues);
  $("cancel").addEventListener("click", () => panel.close());
  form.addEventListener("submit", (e) => {
    e.preventDefault();
    const s = {
      sound: form.sound.checked ? 1 : 0, volume: +form.volume.value, brightness: +form.brightness.value,
      haunt: form.haunt.checked ? 1 : 0, crawl: form.crawl.checked ? 1 : 0, stay: +form.stay.value,
      shift: form.shift.checked ? 1 : 0, pad: form.pad.checked ? 1 : 0, sens: +form.sens.value,
    };
    if (!s.haunt && !s.crawl) s.haunt = 1;
    store.set("settings", JSON.stringify(s));
    const ps = +form.sprint.value;
    flushSlots();
    if (ps !== padSprint()) store.set("prefs", toB64(new Uint8Array([ps])));
    importing = true;                                            // nothing more from this run's memory: it boots again
    location.reload();
  });

  // The ledger as a file: everything the game keeps, to carry to another browser or keep safe.
  $("export").addEventListener("click", () => {
    flushSlots();
    const slots = {};
    SLOT_NAMES.forEach((n) => { const v = store.get(n); if (v) slots[n] = v; });
    const blob = new Blob([JSON.stringify({ game: "HAUNTED.EXE", version: VERSION, written: new Date().toISOString(), slots }, null, 1)], { type: "application/json" });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = "haunted-ledger-" + new Date().toISOString().slice(0, 10) + ".json";
    document.body.appendChild(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
    $("ledger-note").textContent = "The ledger was written to your downloads.";
  });
  $("import").addEventListener("click", () => $("import-file").click());
  $("import-file").addEventListener("change", async (e) => {
    const f = e.target.files && e.target.files[0];
    e.target.value = "";
    if (!f) return;
    let data;
    try { data = JSON.parse(await f.text()); } catch (err) { data = null; }
    const ok = data && data.game === "HAUNTED.EXE" && data.slots && typeof data.slots === "object" &&
      Object.keys(data.slots).every((k) => SLOT_NAMES.includes(k) && typeof data.slots[k] === "string") &&
      Object.values(data.slots).every((v) => { try { const u = fromB64(v); return u.length > 0 && u.length <= 16384; } catch (err) { return false; } });
    if (!ok) { $("ledger-note").textContent = "That file isn't a HAUNTED.EXE ledger."; return; }
    if (!confirm("Read this ledger into this browser? It takes the place of the ledger here, and the game boots again.")) return;
    importing = true;
    SLOT_NAMES.forEach((n) => { if (data.slots[n]) store.set(n, data.slots[n]); else store.del(n); });
    location.reload();
  });
})();
