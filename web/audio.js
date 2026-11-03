// The sound's last step (docs/PLAN-web.md): the game renders stereo at its own rate on the main thread and
// posts it here in chunks; this plays them out at the device's rate, stretching linearly if the two differ,
// and says now and then how much is waiting so the page can keep about 80 ms queued. Run dry, it is silent.
class HauntedSound extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.step = options.processorOptions.rate / sampleRate;   // source frames per output frame
    this.queue = [];                                            // Float32Arrays, interleaved L R
    this.at = 0;                                                // position in the first, in frames, fractional
    this.waiting = 0;                                           // source frames queued
    this.tick = 0;
    this.port.onmessage = (e) => {
      if (e.data === "flush") { this.queue = []; this.waiting = 0; this.at = 0; return; }
      this.queue.push(e.data);
      this.waiting += e.data.length / 2;
    };
  }
  process(inputs, outputs) {
    const out = outputs[0], L = out[0], R = out[1] || out[0], n = L.length;
    for (let i = 0; i < n; i++) {
      const q = this.queue[0];
      if (!q) { L[i] = R[i] = 0; continue; }
      const f = this.at | 0, t = this.at - f, len = q.length / 2;
      const nx = f + 1 < len ? q : this.queue[1], g = f + 1 < len ? f + 1 : 0;
      const l0 = q[f * 2], r0 = q[f * 2 + 1];
      const l1 = nx ? nx[g * 2] : l0, r1 = nx ? nx[g * 2 + 1] : r0;
      L[i] = l0 + (l1 - l0) * t; R[i] = r0 + (r1 - r0) * t;
      this.at += this.step;
      while (this.queue.length && this.at >= this.queue[0].length / 2) {
        this.at -= this.queue[0].length / 2;
        this.waiting -= this.queue[0].length / 2;
        this.queue.shift();
      }
    }
    if (++this.tick % 8 === 0) this.port.postMessage(this.waiting - this.at);
    return true;
  }
}
registerProcessor("haunted-sound", HauntedSound);
