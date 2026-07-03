// Pure parse/filter/encode logic shared by the PebbleKit JS app (index.js)
// and the Node test suite (tools/test_parser.js). No Pebble APIs here.

var MAX_MATCHES = 128;
var JS_FUTURE_WINDOW_SEC = 3 * 3600; // wider than the watch's 2.5h display window

function parseKickoff(dateStr, timeStr) {
  var d = /^(\d{4})-(\d{2})-(\d{2})$/.exec(dateStr || '');
  if (!d) return null;
  var hour = 12, min = 0, offMin = 0; // missing/unparseable time -> 12:00 UTC
  var t = /^(\d{1,2}):(\d{2})(?:\s*UTC([+-])(\d{1,2})(?::(\d{2}))?)?/.exec(timeStr || '');
  if (t) {
    hour = +t[1];
    min = +t[2];
    if (t[3]) {
      offMin = (+t[4]) * 60 + (t[5] ? +t[5] : 0);
      if (t[3] === '-') offMin = -offMin;
    }
  }
  return Date.UTC(+d[1], +d[2] - 1, +d[3], hour, min) / 1000 - offMin * 60;
}

function buildTeamsMap(teamsArray) {
  var map = {};
  teamsArray.forEach(function(team) {
    map[team.name] = team.fifa_code;
    if (team.name_normalised) map[team.name_normalised] = team.fifa_code;
  });
  return map;
}

function teamCode(name, teamsMap) {
  if (teamsMap[name]) return teamsMap[name];
  if (name && name.length <= 3) return name.toUpperCase();
  return 'TBD';
}

function buildMatchList(json, teamsMap, nowSec) {
  var out = [];
  (json.matches || []).forEach(function(m) {
    var kickoff = parseKickoff(m.date, m.time);
    if (kickoff === null) return;
    if (kickoff + JS_FUTURE_WINDOW_SEC <= nowSec) return;
    out.push({
      kickoff: kickoff,
      code1: teamCode(m.team1, teamsMap),
      code2: teamCode(m.team2, teamsMap)
    });
  });
  out.sort(function(a, b) { return a.kickoff - b.kickoff; });
  return out.slice(0, MAX_MATCHES);
}

function encodeMatches(list) {
  var bytes = [list.length & 0xff];
  list.forEach(function(m) {
    bytes.push(m.kickoff & 0xff, (m.kickoff >>> 8) & 0xff,
               (m.kickoff >>> 16) & 0xff, (m.kickoff >>> 24) & 0xff);
    var i;
    for (i = 0; i < 3; i++) bytes.push(i < m.code1.length ? m.code1.charCodeAt(i) : 0);
    for (i = 0; i < 3; i++) bytes.push(i < m.code2.length ? m.code2.charCodeAt(i) : 0);
  });
  return bytes;
}

module.exports = {
  MAX_MATCHES: MAX_MATCHES,
  parseKickoff: parseKickoff,
  buildTeamsMap: buildTeamsMap,
  teamCode: teamCode,
  buildMatchList: buildMatchList,
  encodeMatches: encodeMatches
};
