// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 TRAPSTATION; based on BBL JAM BOX (Blair) and Felucca (Leo Kuroshita, Hügelton Instruments)

import { Updater, pack7, unpack7 } from "./fm1ota.js";

const BK = { BEGIN: 0x50, STAT: 0x51, READ: 0x52, PUT: 0x53, COMMIT: 0x54, END: 0x55, SMP: 0x40 };
const BK_SMP_CMD = { BEGIN: 0x60, SUM: 0x61, WRITE: 0x62, END: 0x63 };

export const BK_IDS = [0, 10, 11, 12, 13, 14, 15, 16, 9, 17];
export const BK_KIND = (id) => (id === 0 ? "settings" : id === 9 || id === 17 ? "jam" : id <= 12 ? "sounds" : "kits");
const BK_OBJ_MAX = 3840, BK_SECTOR = 4096, BK_TABLE = 4096, BK_REGION = 0x3A000, BK_SMP_MAGIC = 0x324D5346, BK_BLOCK = 512;
export const BK_FORMAT = "trapstation-backup";

function bkErr(code, detail = "") { const e = new Error(code + (detail !== "" ? " " + detail : "")); e.code = code; e.detail = String(detail); return e; }
const bkU32 = (v) => [v & 255, (v >>> 8) & 255, (v >>> 16) & 255, (v >>> 24) & 255];
const bkRd32 = (d, i) => (d[i] | d[i + 1] << 8 | d[i + 2] << 16 | d[i + 3] << 24) >>> 0;
const BK_CRC = new Uint32Array(256).map((_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
export function bkCrc32(b) { let c = 0xFFFFFFFF; for (let i = 0; i < b.length; i++) c = BK_CRC[(c ^ b[i]) & 255] ^ (c >>> 8); return (~c) >>> 0; }
function bkB64(u8) { let s = ""; for (let i = 0; i < u8.length; i += 4096) s += String.fromCharCode.apply(null, u8.subarray(i, i + 4096)); return btoa(s); }
function bkUnB64(s) { return Uint8Array.from(atob(s), (c) => c.charCodeAt(0)); }

function bkFrame(cmd, body) {
  const n = body.length, m = [0x00, 0x59, cmd, n & 255, (n >> 8) & 255, (n >> 16) & 255, ...body];
  let s = 0;
  for (const b of body) s += b;
  m.push(~s & 255);
  return [0xF0, ...pack7(m), 0xF7];
}

async function bkAsk(link, cmd, body, ms = 3000, tries = 3) {
  for (let t = 0; t < tries; t++) {
    link.drain();
    if (!link.send(bkFrame(cmd, body))) throw bkErr("lost");
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      const d = unpack7(p.slice(1, -1));
      if (d.length >= 7 && d[0] === 0 && d[1] === 0x59 && d[2] === cmd && d.length === (d[3] | d[4] << 8 | d[5] << 16) + 7)
        return Uint8Array.from(d.slice(6, d.length - 1));
    }
  }
  return null;
}

export const bkSupported = (id) => id.model === "FM-1" && (id.version === 500 || (id.version >= 509 && id.version <= 599));
async function bkFind(access) {
  const dev = await new Updater(access).find((id) => !/^ota-/i.test(id.model));
  if (!dev) throw bkErr("notfound");
  return dev;
}
async function bkReadRange(link, id, from, len, chunk, tick) {
  const out = new Uint8Array(len);
  for (let off = 0; off < len; off += chunk) {
    const k = Math.min(chunk, len - off);
    const r = await bkAsk(link, BK.READ, [id, ...bkU32(from + off), k & 255, k >> 8]);
    if (!r) throw bkErr("bkLost");
    if (r[0] || r.length !== 6 + k || bkRd32(r, 2) !== from + off) throw bkErr("bkRefused", r[0]);
    out.set(r.subarray(6), off);
    if (tick) tick(off + k);
  }
  return out;
}

function bkSampleLen(img) {
  if (img.length < BK_TABLE) return -1;
  const v = new DataView(img.buffer, img.byteOffset, img.byteLength), used = v.getUint32(8, true);
  if (v.getUint32(0, true) !== BK_SMP_MAGIC || v.getUint16(4, true) !== 1 || used > BK_REGION - BK_TABLE) return -1;
  return used;
}

export function bkFactorySet() {
  const objects = BK_IDS.filter((id) => id !== 0).map((id) => [id, new Uint8Array(4)]);
  const samples = new Uint8Array(BK_TABLE), v = new DataView(samples.buffer);
  v.setUint32(0, BK_SMP_MAGIC, true);
  v.setUint16(4, 1, true);
  v.setUint32(28, bkCrc32(samples.subarray(0, 28)), true);
  return { objects, samples, count: 0, firmware: "factory", created: "" };
}

export async function bkBackup(access, step = () => {}) {
  step("find", 0, 1);
  const dev = await bkFind(access), link = dev.link;
  let open = false;
  try {
    if (!bkSupported(dev.id)) throw bkErr("bkOld", dev.id.text);
    const b = await bkAsk(link, BK.BEGIN, [0]);
    if (!b) throw bkErr("bkOld", dev.id.text);
    if (b[0]) throw bkErr("bkRefused", b[0]);
    open = true;
    const chunk = b[2] | b[3] << 8, region = bkRd32(b, 4), objects = {};
    for (let i = 0; i < BK_IDS.length; i++) {
      const id = BK_IDS[i];
      step("obj", i, BK_IDS.length);
      const s = await bkAsk(link, BK.STAT, [id]);
      if (!s) throw bkErr("bkLost");
      if (s[0] === 2) continue;
      if (s[0]) throw bkErr("bkRefused", s[0]);
      const len = bkRd32(s, 2), crc = bkRd32(s, 6), data = await bkReadRange(link, id, 0, len, chunk);
      if (bkCrc32(data) !== crc) throw bkErr("bkCrc");
      objects[id] = bkB64(data);
    }
    let samples = null, count = 0;
    const tab = await bkReadRange(link, BK.SMP, 0, BK_TABLE, chunk), used = bkSampleLen(tab);
    if (used >= 0 && BK_TABLE + used <= region) {
      const all = new Uint8Array(BK_TABLE + used);
      all.set(tab);
      if (used) all.set(await bkReadRange(link, BK.SMP, BK_TABLE, used, chunk, (n) => step("smp", n, used)), BK_TABLE);
      if (bkCrc32(all.subarray(BK_TABLE)) !== new DataView(all.buffer).getUint32(12, true)) throw bkErr("bkCrc");
      samples = bkB64(all);
      count = new DataView(all.buffer).getUint16(6, true);
    }
    await bkAsk(link, BK.END, []);
    open = false;
    return { format: BK_FORMAT, version: 1, firmware: dev.id.text, created: new Date().toISOString(),
             content: { objects: Object.keys(objects).length, samples: count }, objects, samples };
  } finally {
    if (open) link.send(bkFrame(BK.END, []));
    link.close();
  }
}

export function bkCheck(a) {
  if (!a || a.format !== BK_FORMAT || a.version !== 1 || typeof a.objects !== "object") throw bkErr("bkBadFile");
  const objects = [];
  for (const id of BK_IDS) {
    if (!(id in a.objects)) continue;
    const d = bkUnB64(a.objects[id]);
    if (!d.length || d.length > BK_OBJ_MAX) throw bkErr("bkBadFile", id);
    objects.push([id, d]);
  }
  for (const k of Object.keys(a.objects)) if (!BK_IDS.includes(+k)) throw bkErr("bkBadFile", k);
  let samples = null, count = 0;
  if (a.samples) {
    samples = bkUnB64(a.samples);
    const used = bkSampleLen(samples);
    if (used < 0 || samples.length !== BK_TABLE + used ||
        bkCrc32(samples.subarray(BK_TABLE)) !== new DataView(samples.buffer).getUint32(12, true)) throw bkErr("bkBadFile", "samples");
    count = new DataView(samples.buffer).getUint16(6, true);
  }
  if (!objects.length && !samples) throw bkErr("bkBadFile", "empty");
  return { objects, samples, count, firmware: String(a.firmware || ""), created: String(a.created || "") };
}

async function bkWriteSamples(link, img, step) {
  const b = await bkAsk(link, BK_SMP_CMD.BEGIN, []);
  if (!b) throw bkErr("bkOld");
  if (b[0]) throw bkErr("bkRefused", b[0]);
  if (img.length - BK_TABLE > bkRd32(b, 1)) throw bkErr("bkSmpFull");
  const pad = new Uint8Array(Math.ceil(img.length / BK_SECTOR) * BK_SECTOR).fill(0xFF);
  pad.set(img);
  const n = pad.length / BK_SECTOR, order = [];
  for (let s = 1; s < n; s++) order.push(s);
  order.push(0);
  for (let i = 0; i < order.length; i++) {
    const off = order[i] * BK_SECTOR, sec = pad.subarray(off, off + BK_SECTOR);
    const r = await bkAsk(link, BK_SMP_CMD.SUM, bkU32(off));
    if (!r || r[0]) throw bkErr("bkLost");
    if (bkRd32(r, 5) !== bkCrc32(sec)) {
      let ok = false;
      for (let t = 0; t < 3 && !ok; t++) {
        ok = true;
        for (let o = 0; o < BK_SECTOR && ok; o += BK_BLOCK) {
          const w = await bkAsk(link, BK_SMP_CMD.WRITE, [...bkU32(off + o), ...sec.subarray(o, o + BK_BLOCK)], 4000, 1);
          ok = !!w && w[0] === 0;
        }
      }
      if (!ok) throw bkErr("bkWrite");
    }
    step("smp", i + 1, order.length);
  }
  const e = await bkAsk(link, BK_SMP_CMD.END, []);
  if (!e || e[0]) throw bkErr("bkWrite");
}

export async function bkRestore(access, r, withSamples, step = () => {}) {
  step("find", 0, 1);
  const up = new Updater(access), dev = await bkFind(access), link = dev.link;
  let open = false, commits = 0;
  try {
    if (!bkSupported(dev.id)) throw bkErr("bkOld", dev.id.text);
    if (withSamples && r.samples) await bkWriteSamples(link, r.samples, step);
    if (r.objects.length) {
      const b = await bkAsk(link, BK.BEGIN, [1]);
      if (!b) throw bkErr("bkOld", dev.id.text);
      if (b[0]) throw bkErr("bkRefused", b[0]);
      open = true;
      const chunk = b[2] | b[3] << 8;
      for (let i = 0; i < r.objects.length; i++) {
        const [id, d] = r.objects[i];
        step("obj", i, r.objects.length);
        for (let off = 0; off < d.length; off += chunk) {
          const p = await bkAsk(link, BK.PUT, [id, ...bkU32(off), ...d.subarray(off, off + chunk)]);
          if (!p) throw bkErr("bkLost");
          if (p[0]) throw bkErr("bkRefused", p[0]);
        }
        const c = await bkAsk(link, BK.COMMIT, [id, ...bkU32(d.length), ...bkU32(bkCrc32(d))], 6000);
        if (!c) throw bkErr("bkLost");
        if (c[0] === 4) throw bkErr("bkWrite");
        if (c[0]) throw bkErr("bkRefused", c[0]);
        commits++;
      }
      await bkAsk(link, BK.END, [], 3000, 1);
      open = false;
    }
  } finally {
    if (open) link.send(bkFrame(BK.END, []));
    link.close();
  }
  if (commits) {
    step("reboot", 0, 1);
    await new Promise((res) => setTimeout(res, 2500));
    const back = await up.waitFor((id) => !/^ota-/i.test(id.model), 30000);
    if (!back) throw bkErr("noreturn");
    back.link.close();
  }
  return commits;
}

export function bkSave(archive) {
  const url = URL.createObjectURL(new Blob([JSON.stringify(archive)], { type: "application/json" }));
  const a = document.createElement("a");
  a.href = url;
  a.download = `TRAPSTATION-Sicherung-${new Date().toISOString().slice(0, 16).replace(/[T:]/g, "-")}.json`;
  document.body.append(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
