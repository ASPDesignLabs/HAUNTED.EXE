// The browser build's smoke test (docs/PLAN-web.md): serves a site folder, opens /play/ in Playwright's
// Chromium, boots it with a key, lets the dream run, holds H through the takeover, walks and casts with Space,
// presses Esc, and checks the picture is not black, nothing was thrown, the frame rate held and a slot or the
// settings reached localStorage.
//   node tools/web_smoke.js <site-dir> [seconds-of-dream] [shots-dir]
// The site folder is tools/build_site.sh's, with build/web copied into play/.
const http = require("http"), fs = require("fs"), path = require("path");
const { chromium } = require("playwright");

const root = path.resolve(process.argv[2] || "_site");
const dream = Number(process.argv[3] || 20);
const shots = process.argv[4] || "";
const TYPES = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm", ".png": "image/png",
                ".ttf": "font/ttf", ".json": "application/json", ".txt": "text/plain" };

const server = http.createServer((req, res) => {
  let p = decodeURIComponent(new URL(req.url, "http://x").pathname);
  if (p.endsWith("/")) p += "index.html";
  const f = path.join(root, p);
  if (!f.startsWith(root) || !fs.existsSync(f) || fs.statSync(f).isDirectory()) { res.writeHead(404); res.end(); return; }
  res.writeHead(200, { "Content-Type": TYPES[path.extname(f)] || "application/octet-stream" });
  fs.createReadStream(f).pipe(res);
});

function fail(msg) { console.error("FAIL " + msg); process.exitCode = 1; }

(async () => {
  await new Promise((r) => server.listen(0, "127.0.0.1", r));
  const url = "http://127.0.0.1:" + server.address().port + "/play/";
  const browser = await chromium.launch({ args: ["--autoplay-policy=no-user-gesture-required"] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
  const errors = [];
  page.on("pageerror", (e) => errors.push(String(e)));
  page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });
  const shot = async (name) => { if (shots) await page.locator("#screen").screenshot({ path: path.join(shots, name + ".png") }); };
  const dark = () => page.evaluate(() => window.HAUNTED.dark());   // no read-back: it would move the canvas off the GPU
  const fps = async (ms) => page.evaluate((ms) => new Promise((r) => {
    let n = 0, t0 = performance.now(), worst = 0, last = t0;
    const f = (t) => { n++; worst = Math.max(worst, t - last); last = t; if (t - t0 < ms) requestAnimationFrame(f); else r({ fps: n / ((t - t0) / 1000), worst }); };
    requestAnimationFrame(f);
  }), ms);

  await page.goto(url);
  await page.waitForFunction(() => document.getElementById("press").textContent.includes("boot"), null, { timeout: 30000 });
  await page.keyboard.press("Enter");
  await page.waitForFunction(() => document.body.classList.contains("booted"), null, { timeout: 30000 });
  console.log("booted");
  let d, lit = 0, looks = 0;                                        // HAUNT.EXE clears its screen now and then: look every two seconds
  for (let t = 0; t < dream; t += 2) { await page.waitForTimeout(2000); d = await dark(); looks++; if (d.dark < 0.98) lit++; }
  console.log(`dream: ${d.w}x${d.h}, something on the screen in ${lit} of ${looks} looks`);
  if (!lit) fail("the dream's picture stayed black");
  await shot("dream");
  const f1 = await fps(3000);
  console.log(`dream: ${f1.fps.toFixed(1)} frames a second, worst gap ${f1.worst.toFixed(0)} ms`);

  // hold H for the takeover (the key repeats, as a held key does)
  await page.keyboard.down("KeyH");
  for (let i = 0; i < 40; i++) { await page.waitForTimeout(100); await page.keyboard.down("KeyH"); }
  await page.keyboard.up("KeyH");
  await page.waitForTimeout(1500);
  const playing = await page.evaluate(() => document.body.classList.contains("playing"));
  console.log("taken over: " + playing);
  if (!playing) fail("holding H didn't take over");
  d = await dark();
  console.log(`play: ${d.w}x${d.h}, ${(d.dark * 100).toFixed(0)}% near black`);
  if (d.w !== 320 || d.h !== 200) fail("not in the crypt's mode after the takeover");
  await shot("takeover");
  await page.keyboard.down("KeyW"); await page.waitForTimeout(1500); await page.keyboard.up("KeyW");
  await page.keyboard.down("Space"); await page.waitForTimeout(300); await page.keyboard.up("Space");
  await page.waitForTimeout(400);
  await shot("cast");
  // a fresh run's takeover stands before the merchant: Space (as a click) opens her menu; sign her ledger on page 1
  await page.keyboard.press("Enter"); await page.waitForTimeout(400);
  await page.keyboard.press("Enter"); await page.waitForTimeout(400);
  await page.keyboard.press("Enter"); await page.waitForTimeout(600);
  await shot("signed");
  await page.keyboard.press("Escape"); await page.waitForTimeout(800);   // in play, Esc asks: flee, stay, sleepwalk
  await shot("question");
  await page.keyboard.press("Escape");
  await page.waitForTimeout(500);
  await page.evaluate(() => window.HAUNTED.reset());
  const f2 = await fps(3000);
  const pf = await page.evaluate(() => window.HAUNTED.perf);
  console.log(`play: a frame's own work ${((pf.step + pf.sound + pf.draw) / pf.n).toFixed(2)} ms (the game ${(pf.step / pf.n).toFixed(2)}, ` +
              `sound ${(pf.sound / pf.n).toFixed(2)}, the picture ${(pf.draw / pf.n).toFixed(2)}), worst ${pf.worst.toFixed(1)} ms`);
  console.log(`play: ${f2.fps.toFixed(1)} frames a second, worst gap ${f2.worst.toFixed(0)} ms`);
  // Headless Chromium on a machine without a GPU composites in software and misses frames that a real browser
  // wouldn't (a trace shows the time in the compositor's commit, not the page), so the page's own work is what is judged.
  const own = (pf.step + pf.sound + pf.draw) / pf.n;
  if (own > 12) fail(`the page's own work is ${own.toFixed(1)} ms a frame`);
  if (f2.fps < 15) fail("under 15 frames a second");
  const kept = await page.evaluate(() => Object.keys(localStorage).filter((k) => k.startsWith("haunted.exe/")));
  console.log("localStorage: " + (kept.join(", ") || "nothing yet"));
  if (!kept.includes("haunted.exe/slot1")) fail("signing the ledger wrote no page to localStorage");
  // the page survives a reload: boot again and it is still there, read back by the game
  const before = await page.evaluate(() => localStorage.getItem("haunted.exe/slot1"));
  await page.reload();
  await page.waitForFunction(() => document.getElementById("press").textContent.includes("boot"), null, { timeout: 30000 });
  await page.keyboard.press("Enter");
  await page.waitForFunction(() => document.body.classList.contains("booted"), null, { timeout: 30000 });
  await page.waitForTimeout(3000);
  const after = await page.evaluate(() => localStorage.getItem("haunted.exe/slot1"));
  console.log("after a reload, page 1 is " + (after === before ? "as it was" : after ? "rewritten" : "gone"));
  if (!after) fail("the ledger's page didn't survive a reload");
  const sound = await page.evaluate(() => { try { return new AudioContext().state; } catch (e) { return "none"; } });
  console.log("an AudioContext here starts " + sound);
  if (errors.length) fail("errors on the page:\n  " + errors.join("\n  "));

  // a controller, scripted: navigator.getGamepads stands in for one in the standard (XInput) layout
  const ctx = await browser.newContext({ viewport: { width: 960, height: 600 } });
  await ctx.addInitScript(() => {
    const pad = { id: "scripted pad", index: 0, connected: true, mapping: "standard", axes: [0, 0, 0, 0],
                  buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })) };
    window.SCRIPTED_PAD = pad;
    navigator.getGamepads = () => [pad];
  });
  const pp = await ctx.newPage();
  pp.on("pageerror", (e) => errors.push(String(e)));
  await pp.goto(url);
  await pp.waitForFunction(() => document.getElementById("press").textContent.includes("boot"), null, { timeout: 30000 });
  await pp.keyboard.press("Enter");
  await pp.waitForFunction(() => document.body.classList.contains("booted"), null, { timeout: 30000 });
  await pp.waitForTimeout(3000);
  await pp.evaluate(() => { const b = window.SCRIPTED_PAD.buttons[0]; b.pressed = true; b.value = 1; });
  await pp.waitForTimeout(3600);
  await pp.evaluate(() => { const b = window.SCRIPTED_PAD.buttons[0]; b.pressed = false; b.value = 0; });
  await pp.waitForTimeout(800);
  const padPlays = await pp.evaluate(() => window.HAUNTED.state() === 2);
  console.log("holding the pad's A took over: " + padPlays);
  if (!padPlays) fail("holding a pad's button didn't take over");
  await pp.evaluate(() => { window.SCRIPTED_PAD.axes[1] = -1; });   // the left stick forward: a walk
  await pp.waitForTimeout(1000);
  await pp.evaluate(() => { window.SCRIPTED_PAD.axes[1] = 0; });
  if (shots) await pp.locator("#screen").screenshot({ path: path.join(shots, "pad.png") });
  if (errors.length) fail("errors on the page:\n  " + errors.join("\n  "));
  await browser.close();
  server.close();
  console.log(process.exitCode ? "web smoke: FAILED" : "web smoke: ok");
})().catch((e) => { console.error(e); process.exit(1); });
