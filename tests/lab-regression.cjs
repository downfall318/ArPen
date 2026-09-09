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
    const b = ctx.one(ammo, {...plate, tiles: [800], tileDamageMultiplier: 1}, v, single, 0, 'Torso', () => 0);
    near(mono.hp, single.hp);
    near(a.damage, b.damage);
    assert.equal(a.penetrated, b.penetrated);
  }
}
// Force full severity: one intact tile retains the original 100→75→25→0 curve.
const tiled = {...plate, tiles: Array(16).fill(800), tileDamageMultiplier: 1};
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
// Shock tuning must remain independent of health through global transfer.
for (const zone of ['Torso', 'Head']) {
  const a = ctx.one(ammo, plate, 550, {hp: 800}, 0, zone);
  const b = ctx.one({...ammo, bhs: .9, bts: .7}, plate, 550, {hp: 800}, 0, zone);
  const c = ctx.one({...ammo, bhh: 0, bth: 0}, plate, 550, {hp: 800}, 0, zone);
  const d = ctx.one({...ammo, bhs: 0, bts: 0}, plate, 550, {hp: 800}, 0, zone);
  assert.equal(a.penetrated, false);
  assert.ok(a.playerShockDamage > 0);
  near(b.playerHealthDamage, a.playerHealthDamage);
  near(b.playerShockDamage, a.playerShockDamage * 2);
  near(c.playerHealthDamage, 0);
  near(c.playerShockDamage, a.playerShockDamage);
  near(d.playerShockDamage, 0);
  near(d.playerHealthDamage, a.playerHealthDamage);
  const calculatedShock = a.playerBaseDamage * a.bluntSeverity * (zone === 'Head' ? 3 * .45 : .35);
  near(a.playerShockDamage, calculatedShock);
}
// Full severity at initial velocity: preserve local head H=18, S=60.75,
// then transfer independently to global H=36, S=60.75.
const full = ctx.one(ammo, {...plate, k: 15000, resistance: 1}, 940, {hp: 800}, 0, 'Head');
assert.equal(full.penetrated, false);
near(full.bluntSeverity, 1);
near(full.playerHealthDamage, 36);
near(full.playerShockDamage, 60.75);
const shot3 = ctx.series(ammo, tiled, 3, 0, 'Torso');
const repeat3 = ctx.series(ammo, tiled, 3, 0, 'Torso');
assert.equal(JSON.stringify(shot3), JSON.stringify(repeat3));
const random = ctx.tileRandom(0x41525045), counts = Array(16).fill(0);
for (let i=0; i<160000; i++) counts[Math.floor(random()*16)]++;
assert.ok(counts.every(n => n > 9500 && n < 10500), 'Sampling should cover every tile uniformly');
console.log('Lab regressions passed: single-tile parity, tile independence, holes, failure threshold, transfers, and deterministic sampling.');

// Default tiles take 3x damage, but penetration still uses pre-hit condition.
const fragile = {...plate, k: 15000, tiles: Array(16).fill(800)};
const fragileState = {hp: 800};
const first = ctx.one(strong, fragile, 940, fragileState, 0, 'Torso', () => 0);
near(fragileState.tiles[0], .25);
assert.equal(first.penetrated, false);
assert.ok(first.playerHealthDamage > 0, 'A surviving tile retains normal stopped trauma');
const last = ctx.one(strong, fragile, 940, fragileState, 0, 'Torso', () => 0);
assert.equal(fragileState.tiles[0], 0);
assert.equal(last.penetrated, false);
assert.equal(last.stoppedByDestroyedTile, true);
assert.equal(last.playerHealthDamage, 0);
assert.equal(last.playerShockDamage, 0);
const next = ctx.one(strong, fragile, 940, fragileState, 0, 'Torso', () => 0);
assert.equal(next.penetrated, true);
assert.equal(next.stoppedByDestroyedTile, false);
// Penetrating destruction must not trigger the stopped-hit exemption.
const perforated = ctx.one(strong, {...fragile, k: 1, tileDamageMultiplier: 4}, 940, {hp: 800}, 0, 'Head', () => 0);
assert.equal(perforated.tileHealth, 0);
assert.equal(perforated.penetrated, true);
assert.equal(perforated.stoppedByDestroyedTile, false);
// A multiplier on monolithic armor has no effect.
const monoA = ctx.one(strong, {...plate, tileDamageMultiplier: 3}, 940, {hp: 800}, 0, 'Torso');
const monoB = ctx.one(strong, plate, 940, {hp: 800}, 0, 'Torso');
near(monoA.damage, monoB.damage);
console.log('3x tile regressions passed: 100→25→0 health, sacrificial stop, next-hit hole, penetrating destruction, monolithic isolation.');

// Execute the production transfer block with independent channel inputs.
// Both penetrating and stopped packets reach this same block.
const player = fs.readFileSync('Scripts/4_World/ArPen/PlayerBase.c', 'utf8');
const transferStart = player.indexOf('        if (dmgZone == "Torso")');
const transferEnd = player.indexOf('\n        }', player.indexOf('else if (dmgZone == "Head"', transferStart)) + 10;
const transfer = player.slice(transferStart, transferEnd);
for (const dmgZone of ['Torso', 'Head', 'Brain']) {
  for (const [health, shock] of [[20,20], [20,7], [0,20], [20,0]]) {
    const context = {dmgZone, localDamage: {HealthLoss: health}, customShockDamage: shock, packet: {}};
    vm.runInNewContext(transfer, context);
    near(context.packet.GlobalHealthLoss, health * (dmgZone === 'Torso' ? 1 : 2));
    near(context.packet.GlobalShockLoss, shock);
  }
}
console.log('Production transfer regressions passed: independent health/shock, head/brain/torso, zero channels.');

// Run the actual stopped shock expression and transfer together: catch a
// duplicate head factor even when the lab and transfer tests pass separately.
const shockExpression = player.match(/customShockDamage = (stoppedBaseDamage[^;]+);/)[1];
for (const dmgZone of ['Head', 'Brain', 'Torso']) {
  const head = dmgZone !== 'Torso';
  const context = {dmgZone, stoppedBaseDamage: 45,
    shockZoneMultiplier: head ? 3 : 1, bluntShockMultiplier: head ? .45 : .35,
    bluntSeverity: 1, localDamage: {HealthLoss: head ? 18 : 4.5}, packet: {}};
  vm.runInNewContext('customShockDamage = ' + shockExpression + ';' + transfer, context);
  near(context.packet.GlobalShockLoss, head ? 60.75 : 15.75);
}
console.log('Stopped shock integration passed: head/brain 60.75, torso 15.75.');
