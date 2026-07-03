var assert = require('assert');
var fs = require('fs');
var path = require('path');
var parser = require('../src/pkjs/parser');
var teams = require('../src/pkjs/teams');

function utc(y, mo, d, h, mi) { return Date.UTC(y, mo - 1, d, h || 0, mi || 0) / 1000; }

// 48 distinct FIFA codes in the generated map (some names alias to the same code)
var codes = {};
Object.keys(teams).forEach(function(name) { codes[teams[name]] = true; });
assert.strictEqual(Object.keys(codes).length, 48);
assert.strictEqual(teams['Mexico'], 'MEX');
assert.strictEqual(teams['Korea Republic'], 'KOR'); // name_normalised alias

// End-to-end against the pinned fixture at a fixed instant: 2026-07-03T00:00Z.
// At that instant match 83 (Portugal v Croatia, kicked off 2026-07-02 19:00 UTC-4
// = epoch 1783033200) is still inside the 3h window, and 21 matches remain.
var fixture = JSON.parse(fs.readFileSync(path.join(__dirname, 'fixtures', 'worldcup.json'), 'utf8'));
var NOW = utc(2026, 7, 3, 0, 0); // 1783036800
var list = parser.buildMatchList(fixture, teams, NOW);
assert.strictEqual(list.length, 21);
assert.deepStrictEqual(list[0], { kickoff: 1783033200, code1: 'POR', code2: 'CRO' });
assert.deepStrictEqual(list[list.length - 1],
    { kickoff: 1784487600, code1: 'TBD', code2: 'TBD' }); // the Final: W101 v W102 (4 chars -> TBD)
assert.strictEqual(parser.encodeMatches(list).length, 1 + 21 * 10);

console.log('fixture tests OK');
