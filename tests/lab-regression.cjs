// Execute the actual lab physics without a browser or network dependency.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const html = fs.readFileSync('index.html', 'utf8');
const script = html.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
new vm.Script(script); // Entire UI script must parse, not just extracted physics.
const physics = script.slice(script.indexOf('  function effective('), script.indexOf('  function colorFor('));
const ctx = { Math, ranks: ['Sub-IIA','IIA','II','IIIA+','III','III+','IV'], floors: {} };
vm.createContext(ctx);
vm.runInContext(physics, ctx);
const ammo = { mass: .004, v0: 940, cal: 5.71, base: 45, pm: 1, threat: 'III', bhh: .2, bth: .1, bhs: .45, bts: .35 };
const plate = { hp: 800, k: 1500, t: 24, type: 'Ceramic', areal: 25.9, resistance: 151.6 };
const near = (a,b) => assert.ok(Math.abs(a-b) < 1e-8, `${a} != ${b}`);
// Empty and one-tile configurations must preserve the original armor curve.
for (const v of [0, 250, 550, 940]) {
  const mono = { hp: 800 }, single = { hp: 800 };
  for (let hit = 0; hit < 20; hit++) {
    const a = ctx.one(ammo, plate, v, mono, 0, 'Torso');
    const b = ctx.one(ammo, {...plate, tiles: [800]}, v, single, 0, 'Torso', () => 0);
    near(mono.hp, single.hp);
    near(a.damage, b.damage);
    assert.equal(a.penetrated, b.penetrated);
  }
}
// Force full severity: one intact tile retains the original 100→75→25→0 curve.
const tiled = {...plate, tiles: Array(16).fill(800)};
const state = {hp: 800};
const strong = {...ammo, base: 1000};
for (const expected of [.75, .25, 0]) {
  ctx.one(strong, tiled, 940, state, 0, 'Torso', () => 0);
  near(state.tiles[0], expected);
  near(state.tiles[1] ?? 1, 1);
}
assert.ok(state.hp > 0, 'One of sixteen defeated tiles must not ruin the vest');
const beforeHole = state.hp;
const hole = ctx.one(strong, tiled, 940, state, 0, 'Torso', () => 0);
assert.equal(hole.penetrated, true);
near(hole.exit, 940);
near(state.hp, beforeHole);
for (let tile = 1; tile < 4; tile++) {
  for (let hit = 0; hit < 3; hit++) ctx.one(strong, tiled, 940, state, 0, 'Torso', () => (tile + .5) / 16);
  if (tile < 3) assert.ok(state.hp > 0);
}
assert.equal(state.hp, 0, 'Four of sixteen defeated tiles must ruin the vest');
assert.equal(ctx.tileCondition([0,1,1,1,1]), .8);
assert.equal(ctx.tileCondition([0,0,1,1,1]), 0, 'Round the failure count upward');
// A fifth shot through whole destroyed armor must be native-result passthrough.
assert.equal(ctx.one(ammo, plate, 940, {hp: 0}, 0, 'Torso').penetrated, true);
// Transfers, not independently editable shock controls, determine torso/head shock.
for (const zone of ['Torso', 'Head']) {
  const a = ctx.one(ammo, plate, 550, {hp: 800}, 0, zone);
  const b = ctx.one({...ammo, bhs: 999, bts: 999}, plate, 550, {hp: 800}, 0, zone);
  assert.equal(a.penetrated, false);
  near(a.playerShockDamage, b.playerShockDamage);
  near(a.playerShockDamage, a.playerHealthDamage * (zone === 'Head' ? 1.5 : 1));
}
const shot3 = ctx.series(ammo, tiled, 3, 0, 'Torso');
const repeat3 = ctx.series(ammo, tiled, 3, 0, 'Torso');
assert.equal(JSON.stringify(shot3), JSON.stringify(repeat3));
const random = ctx.tileRandom(0x41525045), counts = Array(16).fill(0);
for (let i=0; i<160000; i++) counts[Math.floor(random()*16)]++;
assert.ok(counts.every(n => n > 9500 && n < 10500), 'Sampling should cover every tile uniformly');
console.log('Lab regressions passed: single-tile parity, tile independence, holes, failure threshold, transfers, and deterministic sampling.');
