var parser = require('./parser');
var TEAMS = require('./teams');

var DATA_URL = 'https://raw.githubusercontent.com/openfootball/worldcup.json/master/2026/worldcup.json';
var FETCH_INTERVAL_MS = 6 * 60 * 60 * 1000;
var CACHE_KEY = 'worldcup_json_v1';

function sendMatches(jsonText) {
  var json;
  try {
    json = JSON.parse(jsonText);
  } catch (e) {
    console.log('worldcup: bad JSON, not sending: ' + e);
    return;
  }
  var list = parser.buildMatchList(json, TEAMS, Date.now() / 1000);
  var bytes = parser.encodeMatches(list);
  console.log('worldcup: sending ' + list.length + ' matches (' + bytes.length + ' bytes)');
  Pebble.sendAppMessage({ MATCHES: bytes }, function() {
    console.log('worldcup: send ok');
  }, function(e) {
    console.log('worldcup: send failed: ' + JSON.stringify(e));
  });
}

function fetchData() {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', DATA_URL);
  xhr.timeout = 15000;
  xhr.onload = function() {
    if (xhr.status >= 200 && xhr.status < 300) {
      localStorage.setItem(CACHE_KEY, xhr.responseText);
      sendMatches(xhr.responseText);
    } else {
      console.log('worldcup: fetch HTTP ' + xhr.status + ', using cache');
      sendCached();
    }
  };
  xhr.onerror = xhr.ontimeout = function() {
    console.log('worldcup: fetch failed, using cache');
    sendCached();
  };
  xhr.send();
}

function sendCached() {
  var cached = localStorage.getItem(CACHE_KEY);
  if (cached) sendMatches(cached);
}

Pebble.addEventListener('ready', function() {
  console.log('worldcup: pkjs ready');
  sendCached();   // fast path: last-good data immediately
  fetchData();    // then refresh from the network
  setInterval(fetchData, FETCH_INTERVAL_MS);
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload && e.payload.REQUEST_REFRESH) {
    console.log('worldcup: watch requested refresh');
    fetchData();
  }
});
