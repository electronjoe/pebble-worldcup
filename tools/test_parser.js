var assert = require('assert');
var parser = require('../src/pkjs/parser');

function utc(y, mo, d, h, mi) { return Date.UTC(y, mo - 1, d, h || 0, mi || 0) / 1000; }

// --- parseKickoff: "13:00 UTC-6" style strings -> UTC epoch seconds ---
assert.strictEqual(parser.parseKickoff('2026-06-11', '13:00 UTC-6'), utc(2026, 6, 11, 19, 0));
assert.strictEqual(parser.parseKickoff('2026-07-19', '15:00 UTC-4'), utc(2026, 7, 19, 19, 0));
assert.strictEqual(parser.parseKickoff('2026-06-11', '9:30 UTC+5:30'), utc(2026, 6, 11, 4, 0));
assert.strictEqual(parser.parseKickoff('2026-06-11', null), utc(2026, 6, 11, 12, 0));
assert.strictEqual(parser.parseKickoff('2026-06-11', 'afternoon'), utc(2026, 6, 11, 12, 0));
assert.strictEqual(parser.parseKickoff('bogus', '13:00 UTC-6'), null);

// --- buildTeamsMap / teamCode ---
var tinyMap = parser.buildTeamsMap([
  { name: 'Mexico', fifa_code: 'MEX' },
  { name: 'South Korea', name_normalised: 'Korea Republic', fifa_code: 'KOR' }
]);
assert.strictEqual(parser.teamCode('Mexico', tinyMap), 'MEX');
assert.strictEqual(parser.teamCode('South Korea', tinyMap), 'KOR');
assert.strictEqual(parser.teamCode('Korea Republic', tinyMap), 'KOR');
assert.strictEqual(parser.teamCode('W83', tinyMap), 'W83');
assert.strictEqual(parser.teamCode('W101', tinyMap), 'TBD');
assert.strictEqual(parser.teamCode('Winner of group A', tinyMap), 'TBD');

// --- buildMatchList: filter boundary is kickoff + 3h > now (strict) ---
var NOW = utc(2026, 7, 3, 0, 0);
var json = { matches: [
  { date: '2026-07-10', time: '12:00 UTC', team1: 'W101', team2: 'South Korea' },
  { date: '2026-07-02', time: '21:00 UTC', team1: 'Mexico', team2: 'W83' },
  { date: '2026-07-02', time: '21:01 UTC', team1: 'Mexico', team2: 'W83' }
] };
var list = parser.buildMatchList(json, tinyMap, NOW);
assert.strictEqual(list.length, 2);                               // 21:00 match: kickoff+3h == NOW, excluded
assert.strictEqual(list[0].kickoff, utc(2026, 7, 2, 21, 1));      // sorted ascending
assert.deepStrictEqual(list[1], { kickoff: utc(2026, 7, 10, 12, 0), code1: 'TBD', code2: 'KOR' });

// --- encodeMatches: count byte, little-endian epoch, NUL-padded ASCII codes ---
var bytes = parser.encodeMatches([{ kickoff: 0x0102030A, code1: 'USA', code2: 'W9' }]);
assert.strictEqual(bytes.length, 1 + 10);
assert.strictEqual(bytes[0], 1);
assert.deepStrictEqual(bytes.slice(1, 5), [0x0A, 0x03, 0x02, 0x01]);
assert.deepStrictEqual(bytes.slice(5, 8), [85, 83, 65]);   // 'U','S','A'
assert.deepStrictEqual(bytes.slice(8, 11), [87, 57, 0]);   // 'W','9',NUL

console.log('parser tests OK');
