// The touch build in Playwright's Chromium as a Pixel 7 held sideways, with a 63-pixel camera strip as the owner's phone has
// (docs/PLAN-mobile.md): Touch chosen, a finger held to take over, the title, a new run, her question, a slide that turns the
// view, Esc held on the top left, its TURNING row raised. node tools/touch_smoke.js <folder with play/ or build/web> <shots dir>
const http = require("http"), fs = require("fs"), path = require("path");
const { chromium, devices } = require("playwright");
const root = path.resolve(process.argv[2]), out = process.argv[3];
const T = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm", ".ttf": "font/ttf" };
const server = http.createServer((req, res) => {
  let p = decodeURIComponent(new URL(req.url, "http://x").pathname).replace(/^\/play/, "");
  if (p.endsWith("/")) p += "index.html";
  const f = path.join(root, p);
  if (!f.startsWith(root) || !fs.existsSync(f) || fs.statSync(f).isDirectory()) { res.writeHead(404); res.end(); return; }
  res.writeHead(200, { "Content-Type": T[path.extname(f)] || "application/octet-stream" }); fs.createReadStream(f).pipe(res);
});
(async () => {
  await new Promise((r) => server.listen(0, "127.0.0.1", r));
  const browser = await chromium.launch({ args: ["--autoplay-policy=no-user-gesture-required"] });
  const ctx = await browser.newContext({ ...devices["Pixel 7 landscape"] });
  const page = await ctx.newPage();
  const errors = [];
  page.on("pageerror", (e) => errors.push(String(e)));
  page.on("console", (m) => { if (m.type() === "error") errors.push(m.text()); });
  const cdp = await ctx.newCDPSession(page);
  const shot = (n) => page.screenshot({ path: path.join(out, "t-" + n + ".png") });
  const st = () => page.evaluate(() => window.HAUNTED.state());
  const box = async () => page.evaluate(() => { const r = document.getElementById("screen").getBoundingClientRect(), c = document.getElementById("screen");
    return { x: r.left, y: r.top, w: r.width, h: r.height, sw: c.width, sh: c.height }; });
  const at = (b, sx, sy) => ({ x: b.x + sx * b.w / b.sw, y: b.y + sy * b.h / b.sh });   // stage pixels to the page's
  const down = (pts) => cdp.send("Input.dispatchTouchEvent", { type: "touchStart", touchPoints: pts });
  const move = (pts) => cdp.send("Input.dispatchTouchEvent", { type: "touchMove", touchPoints: pts });
  const up = () => cdp.send("Input.dispatchTouchEvent", { type: "touchEnd", touchPoints: [] });
  const tap = async (p) => { await down([{ ...p, id: 1 }]); await page.waitForTimeout(60); await up(); await page.waitForTimeout(250); };
  await page.goto(`http://127.0.0.1:${server.address().port}/play/?perf`);
  await page.addStyleTag({ content: "#safe { padding-left: 63px !important; }" });   // a camera's strip, as the owner's phone has
  await page.waitForFunction(() => document.getElementById("plate").classList.contains("ready"), null, { timeout: 30000 });
  await page.locator('#how button[data-how="touch"]').tap();
  await page.waitForFunction(() => document.body.classList.contains("booted"), null, { timeout: 30000 });
  await page.waitForTimeout(3000);
  let b = await box();
  const mid = at(b, b.sw / 2, b.sh / 2);
  await down([{ ...mid, id: 1 }]); await page.waitForTimeout(3700); await up(); await page.waitForTimeout(800);
  b = await box();
  console.log(`taken over: state ${await st()}; canvas ${b.sw}x${b.sh}`);
  const L = at(b, 90, b.sh - 40);
  await tap(at(b, b.sw / 2, b.sh / 2)); await page.waitForTimeout(1000);
  const m0 = at(b, b.sw / 2, 100), m1 = at(b, b.sw / 2, 140);
  const drag = async (p0, p1, id, steps = 6, ms = 200) => {
    await down([{ ...p0, id }]);
    for (let i = 1; i <= steps; i++) { await move([{ x: p0.x + (p1.x - p0.x) * i / steps, y: p0.y + (p1.y - p0.y) * i / steps, id }]); await page.waitForTimeout(ms / steps); }
    await up(); await page.waitForTimeout(350);
  };
  await drag(m0, m1, 2);
  await tap(L); await page.waitForTimeout(2500);
  await tap(L); await page.waitForTimeout(800);
  await tap(L); await page.waitForTimeout(1500);
  await shot("a-room");
  await drag(at(b, 420, 100), at(b, 300, 100), 5, 8, 300);          // slid left on the play field: the view turns left
  await page.waitForTimeout(500);
  await shot("b-turned");
  const C = at(b, 90, 25);
  await down([{ ...C, id: 6 }]); await page.waitForTimeout(700); await up(); await page.waitForTimeout(600);
  await shot("c-esc");
  for (let i = 0; i < 4; i++) await drag(at(b, 300, 90), at(b, 300, 130), 7 + i, 4, 120);
  await shot("d-rows");
  await drag(at(b, 260, 120), at(b, 330, 120), 20, 4, 120);
  await shot("e-raised");
  console.log("state at the end " + (await st()));
  console.log("errors: " + (errors.length ? errors.join(" | ") : "none"));
  await browser.close(); server.close();
})();
