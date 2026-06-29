var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var HEY_BASE_URL = 'https://app.hey.com';
var TIMELINE_API_BASE = 'https://timeline-api.getpebble.com/v1/user/pins/';
var MAX_HABITS = 4;
var MAX_TIMELINE_PINS = 30;
var TIMELINE_PIN_IDS_KEY = 'HeyTimelinePinIds';
var HABIT_CATALOG_KEY = 'HeyHabitCatalog';
var TODO_FOOTER_TEXT_MAX = 80;
var todoRotateIndex = 0;
var s_fetchId = 0;

var settings = {
  accessToken: '',
  refreshToken: '',
  tokenEndpoint: ''
};

function isSettingEnabled(value) {
  return value === true || value === 'true' || value === 1 || value === '1';
}

function readClaySettings() {
  try {
    return JSON.parse(localStorage.getItem('clay-settings') || '{}');
  } catch (e) {
    return {};
  }
}

function writeClaySettings(claySettings) {
  localStorage.setItem('clay-settings', JSON.stringify(claySettings));
}

function clayValue(value) {
  if (value && typeof value === 'object' && value.value !== undefined) {
    return value.value;
  }
  return value;
}

function readClaySetting(key, defaultValue) {
  var claySettings = readClaySettings();
  if (claySettings[key] !== undefined && claySettings[key] !== null && claySettings[key] !== '') {
    return clayValue(claySettings[key]);
  }
  var stored = localStorage.getItem(key);
  if (stored !== null && stored !== '' && stored !== 'undefined' && stored !== 'null') {
    return stored;
  }
  return defaultValue;
}

function persistPreferenceSettings(config) {
  var claySettings = readClaySettings();

  if (config.AppearanceMode !== undefined) {
    var appearance = clayValue(config.AppearanceMode) || 'light';
    claySettings.AppearanceMode = appearance;
    localStorage.setItem('AppearanceMode', appearance);
  }
  if (config.FooterContent !== undefined) {
    var footer = clayValue(config.FooterContent) || 'todos';
    claySettings.FooterContent = footer;
    localStorage.setItem('FooterContent', footer);
  }
  if (config.SyncTimeline !== undefined) {
    var timeline = isSettingEnabled(config.SyncTimeline);
    claySettings.SyncTimeline = timeline;
    localStorage.setItem('SyncTimeline', timeline ? 'true' : 'false');
  }
  if (config.HabitSlot1 !== undefined) {
    claySettings.HabitSlot1 = config.HabitSlot1 || '';
    localStorage.setItem('HabitSlot1', claySettings.HabitSlot1);
  }
  if (config.HabitSlot2 !== undefined) {
    claySettings.HabitSlot2 = config.HabitSlot2 || '';
    localStorage.setItem('HabitSlot2', claySettings.HabitSlot2);
  }
  if (config.HabitSlot3 !== undefined) {
    claySettings.HabitSlot3 = config.HabitSlot3 || '';
    localStorage.setItem('HabitSlot3', claySettings.HabitSlot3);
  }
  if (config.HabitSlot4 !== undefined) {
    claySettings.HabitSlot4 = config.HabitSlot4 || '';
    localStorage.setItem('HabitSlot4', claySettings.HabitSlot4);
  }

  writeClaySettings(claySettings);
}

function syncAuthStorage(access, refresh, endpoint) {
  access = normalizeToken(access || '');
  refresh = normalizeToken(refresh || '');
  endpoint = (endpoint || '').trim();

  if (access) {
    localStorage.setItem('HeyAccessToken', access);
  } else {
    localStorage.removeItem('HeyAccessToken');
  }
  if (refresh) {
    localStorage.setItem('HeyRefreshToken', refresh);
  } else {
    localStorage.removeItem('HeyRefreshToken');
  }
  if (endpoint) {
    localStorage.setItem('HeyTokenEndpoint', endpoint);
  } else {
    localStorage.removeItem('HeyTokenEndpoint');
  }

  var claySettings = readClaySettings();
  if (access) {
    claySettings.HeyAccessToken = access;
  } else {
    delete claySettings.HeyAccessToken;
  }
  if (refresh) {
    claySettings.HeyRefreshToken = refresh;
  } else {
    delete claySettings.HeyRefreshToken;
  }
  if (endpoint) {
    claySettings.HeyTokenEndpoint = endpoint;
  } else {
    delete claySettings.HeyTokenEndpoint;
  }
  claySettings.DisconnectHey = false;
  writeClaySettings(claySettings);

  settings.accessToken = access;
  settings.refreshToken = refresh;
  settings.tokenEndpoint = endpoint;
}

function xhrRequest(url, method, headers, body, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(null, xhr.status, xhr.responseText);
  };
  xhr.onerror = function() {
    callback(new Error('network error'), 0, '');
  };
  xhr.open(method, url);
  if (headers) {
    for (var key in headers) {
      if (headers.hasOwnProperty(key)) {
        xhr.setRequestHeader(key, headers[key]);
      }
    }
  }
  xhr.send(body || null);
}

function authHeaders() {
  return {
  'Authorization': 'Bearer ' + settings.accessToken,
  'Accept': 'application/json'
  };
}

function normalizeToken(token) {
  if (!token) {
    return '';
  }
  token = token.trim();
  if (token.indexOf('Bearer ') === 0) {
    token = token.substring(7).trim();
  }
  while (token.length > 0 && token.charAt(token.length - 1) === '%') {
    token = token.substring(0, token.length - 1);
  }
  return token;
}

function saveSettings() {
  var claySettings = readClaySettings();
  settings.accessToken = normalizeToken(
    claySettings.HeyAccessToken || localStorage.getItem('HeyAccessToken') || ''
  );
  settings.refreshToken = normalizeToken(
    claySettings.HeyRefreshToken || localStorage.getItem('HeyRefreshToken') || ''
  );
  settings.tokenEndpoint = (
    claySettings.HeyTokenEndpoint || localStorage.getItem('HeyTokenEndpoint') || ''
  ).trim();
}

function clearHeyCredentials() {
  s_fetchId += 1;
  localStorage.removeItem('HeyAccessToken');
  localStorage.removeItem('HeyRefreshToken');
  localStorage.removeItem('HeyTokenEndpoint');
  localStorage.removeItem(HABIT_CATALOG_KEY);
  localStorage.removeItem(TIMELINE_PIN_IDS_KEY);

  var claySettings = readClaySettings();
  delete claySettings.HeyAccessToken;
  delete claySettings.HeyRefreshToken;
  delete claySettings.HeyTokenEndpoint;
  claySettings.DisconnectHey = false;
  writeClaySettings(claySettings);

  settings.accessToken = '';
  settings.refreshToken = '';
  settings.tokenEndpoint = '';
}

function saveHeyTokenSettings(config) {
  var claySettings = readClaySettings();
  var previousAccess = normalizeToken(
    claySettings.HeyAccessToken || localStorage.getItem('HeyAccessToken') || ''
  );
  var accessProvided = config.HeyAccessToken !== undefined;
  var newAccess = accessProvided
    ? normalizeToken(config.HeyAccessToken || '')
    : previousAccess;

  if (!newAccess) {
    syncAuthStorage('', '', '');
    return;
  }

  if (accessProvided && newAccess !== previousAccess) {
    // New access token invalidates any saved refresh credentials.
    syncAuthStorage(newAccess, '', '');
    return;
  }

  var refresh = config.HeyRefreshToken !== undefined
    ? normalizeToken(config.HeyRefreshToken || '')
    : normalizeToken(
      claySettings.HeyRefreshToken || localStorage.getItem('HeyRefreshToken') || ''
    );
  var endpoint = config.HeyTokenEndpoint !== undefined
    ? (config.HeyTokenEndpoint || '').trim()
    : (claySettings.HeyTokenEndpoint || localStorage.getItem('HeyTokenEndpoint') || '').trim();
  syncAuthStorage(newAccess, refresh, endpoint);
}

function refreshAccessToken(fetchId, callback) {
  if (!settings.refreshToken || !settings.tokenEndpoint) {
    callback(new Error('no refresh credentials'));
    return;
  }

  var body = 'grant_type=refresh_token&refresh_token=' +
    encodeURIComponent(settings.refreshToken);
  xhrRequest(settings.tokenEndpoint, 'POST', {
    'Content-Type': 'application/x-www-form-urlencoded'
  }, body, function(err, status, responseText) {
    if (err || status < 200 || status >= 300) {
      callback(new Error('refresh failed'));
      return;
    }
    try {
      var json = JSON.parse(responseText);
      if (json.access_token) {
        if (fetchId !== s_fetchId) {
          callback(new Error('stale fetch'));
          return;
        }
        syncAuthStorage(json.access_token, json.refresh_token || settings.refreshToken, settings.tokenEndpoint);
        callback(null);
        return;
      }
    } catch (e) {
      callback(e);
      return;
    }
    callback(new Error('invalid refresh response'));
  });
}

function apiGet(path, fetchId, callback) {
  xhrRequest(HEY_BASE_URL + path, 'GET', authHeaders(), null, function(err, status, responseText) {
    if (fetchId !== s_fetchId) {
      return;
    }
    if (status === 401 && settings.refreshToken && settings.tokenEndpoint) {
      refreshAccessToken(fetchId, function(refreshErr) {
        if (fetchId !== s_fetchId) {
          return;
        }
        if (refreshErr) {
          callback(refreshErr, status, responseText);
          return;
        }
        xhrRequest(HEY_BASE_URL + path, 'GET', authHeaders(), null, function(err2, status2, text2) {
          if (fetchId !== s_fetchId) {
            return;
          }
          callback(err2, status2, text2);
        });
      });
      return;
    }
    callback(err, status, responseText);
  });
}

function todayString() {
  var now = new Date();
  var month = ('0' + (now.getMonth() + 1)).slice(-2);
  var day = ('0' + now.getDate()).slice(-2);
  return now.getFullYear() + '-' + month + '-' + day;
}

function addDaysString(dateString, days) {
  var parts = dateString.split('-');
  var date = new Date(
    parseInt(parts[0], 10),
    parseInt(parts[1], 10) - 1,
    parseInt(parts[2], 10)
  );
  date.setDate(date.getDate() + days);
  var month = ('0' + (date.getMonth() + 1)).slice(-2);
  var day = ('0' + date.getDate()).slice(-2);
  return date.getFullYear() + '-' + month + '-' + day;
}

function footerContentMode() {
  var mode = readClaySetting('FooterContent', 'todos');
  if (mode !== 'todos' && mode !== 'event' && mode !== 'off') {
    return 'todos';
  }
  return mode;
}

function syncTimelineEnabled() {
  return isSettingEnabled(readClaySetting('SyncTimeline', false));
}

function isSameDayAs(isoString, dateString) {
  if (!isoString || !dateString) {
    return false;
  }
  return isoString.indexOf(dateString) === 0;
}

function isZeroDate(isoString) {
  return !isoString || isoString.indexOf('0001-01-01') === 0;
}

function unwrapCalendars(payload) {
  var wrappers;
  if (!payload) {
    return [];
  }
  if (Array.isArray(payload)) {
    return payload;
  }
  if (payload.calendars) {
    wrappers = payload.calendars;
  } else if (payload.data) {
    if (Array.isArray(payload.data)) {
      return payload.data;
    }
    wrappers = payload.data.calendars || [];
  } else {
    return [];
  }
  var calendars = [];
  var i;
  for (i = 0; i < wrappers.length; i++) {
    calendars.push(wrappers[i].calendar || wrappers[i]);
  }
  return calendars;
}

function findPersonalCalendarId(calendarsPayload) {
  var calendars = unwrapCalendars(calendarsPayload);
  var i;
  for (i = 0; i < calendars.length; i++) {
    if (calendars[i].personal) {
      return calendars[i].id;
    }
  }
  for (i = 0; i < calendars.length; i++) {
    if (calendars[i].name && calendars[i].name.toLowerCase() === 'personal') {
      return calendars[i].id;
    }
  }
  return null;
}

function unwrapRecordings(payload) {
  if (!payload) {
    return {};
  }
  if (payload.data && !payload['Calendar::Habit']) {
    return payload.data;
  }
  return payload;
}

function recordingsForType(recordingsPayload, typeName) {
  var payload = unwrapRecordings(recordingsPayload);
  if (!payload) return [];
  if (payload[typeName]) return payload[typeName];
  return [];
}

function habitCompletionMap(recordingsPayload, queryDate) {
  var completions = recordingsForType(recordingsPayload, 'Calendar::Habit::Completion');
  var done = {};
  var i;
  for (i = 0; i < completions.length; i++) {
    var completion = completions[i];
    if (!isSameDayAs(completion.starts_at, queryDate)) {
      continue;
    }
    var habitId = completion.parent_id;
    if (!habitId && completion.parent) {
      habitId = completion.parent.id;
    }
    if (habitId) {
      done[habitId] = true;
    }
  }
  return done;
}

function heyDayIndex(dateString) {
  var parts = dateString.split('-');
  var date = new Date(
    parseInt(parts[0], 10),
    parseInt(parts[1], 10) - 1,
    parseInt(parts[2], 10)
  );
  var jsDay = date.getDay();
  return jsDay === 0 ? 6 : jsDay - 1;
}

function habitScheduledForDate(habit, dateString) {
  var days = habit.days;
  if (!days || days.length === 0) {
    return true;
  }
  var parts = dateString.split('-');
  var date = new Date(
    parseInt(parts[0], 10),
    parseInt(parts[1], 10) - 1,
    parseInt(parts[2], 10)
  );
  var heyDow = heyDayIndex(dateString);
  var jsDow = date.getDay();
  var i;
  for (i = 0; i < days.length; i++) {
    var day = days[i];
    if (day === heyDow || day === jsDow) {
      return true;
    }
  }
  return false;
}

function normalizeHabitName(name) {
  return (name || '').trim().toLowerCase();
}

function habitMatchesSlot(habit, slotName) {
  var slot = normalizeHabitName(slotName);
  if (!slot) {
    return false;
  }
  var title = normalizeHabitName(habit.title);
  if (!title) {
    return false;
  }
  if (title === slot || title.indexOf(slot) !== -1 || slot.indexOf(title) !== -1) {
    return true;
  }
  if ((slot.indexOf('fitness') !== -1 || slot.indexOf('kettlebell') !== -1 ||
       slot.indexOf('weight') !== -1) &&
      (title.indexOf('fitness') !== -1 || title.indexOf('kettlebell') !== -1 ||
       title.indexOf('weight') !== -1)) {
    return true;
  }
  return false;
}

function getHabitSlotNames() {
  return [
    readClaySetting('HabitSlot1', ''),
    readClaySetting('HabitSlot2', ''),
    readClaySetting('HabitSlot3', ''),
    readClaySetting('HabitSlot4', '')
  ];
}

function habitDetailScore(habit) {
  if (!habit) {
    return 0;
  }
  var score = 0;
  if (habit.title) {
    score += 2;
  }
  if (habit.icon || habit.icon_url) {
    score += 2;
  }
  if (habit.color) {
    score += 1;
  }
  if (habit.days && habit.days.length) {
    score += 3;
  }
  return score;
}

function loadHabitCatalog() {
  try {
    var raw = localStorage.getItem(HABIT_CATALOG_KEY);
    if (raw) {
      return JSON.parse(raw);
    }
  } catch (e) {
    // ignore corrupt cache
  }
  return {};
}

function saveHabitCatalog(catalog) {
  localStorage.setItem(HABIT_CATALOG_KEY, JSON.stringify(catalog));
}

function catalogEntryFromHabit(habit) {
  return {
    id: habit.id,
    title: habit.title || '',
    icon: habit.icon || '',
    icon_url: habit.icon_url || '',
    color: habit.color || 'blue',
    days: habit.days || []
  };
}

function mergeHabitsIntoCatalog(catalog, habits, preferToday) {
  var i;
  var habit;
  var entry;
  var current;
  for (i = 0; i < habits.length; i++) {
    habit = habits[i];
    if (!habit || !habit.id) {
      continue;
    }
    entry = catalogEntryFromHabit(habit);
    current = catalog[habit.id];
    if (preferToday || !current || habitDetailScore(habit) > habitDetailScore(current)) {
      catalog[habit.id] = entry;
    }
  }
}

function extractHabitsFromPayload(payload) {
  var habits = [];
  var seen = {};
  var i;
  var key;
  var list;
  if (!payload) {
    return habits;
  }

  function add(habit) {
    if (habit && habit.id && !seen[habit.id]) {
      seen[habit.id] = true;
      habits.push(habit);
    }
  }

  var fromType = recordingsForType(payload, 'Calendar::Habit');
  for (i = 0; i < fromType.length; i++) {
    add(fromType[i]);
  }

  var payloadData = unwrapRecordings(payload);
  for (key in payloadData) {
    if (!payloadData.hasOwnProperty(key)) {
      continue;
    }
    list = payloadData[key];
    for (i = 0; i < list.length; i++) {
      if (list[i].type === 'CalendarHabit') {
        add(list[i]);
      }
    }
  }
  return habits;
}

function syncHabitCatalog(todayPayload, weekPayload) {
  var catalog = loadHabitCatalog();
  mergeHabitsIntoCatalog(catalog, extractHabitsFromPayload(weekPayload), false);
  mergeHabitsIntoCatalog(catalog, extractHabitsFromPayload(todayPayload), true);
  saveHabitCatalog(catalog);
  var list = [];
  for (var id in catalog) {
    if (catalog.hasOwnProperty(id)) {
      list.push(catalog[id]);
    }
  }
  return list;
}

function scheduledHabitsFromList(habits, queryDate) {
  var scheduled = [];
  var i;
  for (i = 0; i < habits.length; i++) {
    if (habitScheduledForDate(habits[i], queryDate)) {
      scheduled.push(habits[i]);
    }
  }
  return scheduled;
}

function catalogMapFromList(catalogList) {
  var map = {};
  var i;
  for (i = 0; i < catalogList.length; i++) {
    if (catalogList[i] && catalogList[i].id) {
      map[catalogList[i].id] = catalogList[i];
    }
  }
  return map;
}

function enrichHabitFromCatalog(habit, catalogMap) {
  var cached = catalogMap[habit.id];
  if (!cached) {
    return habit;
  }
  return {
    id: habit.id,
    title: habit.title || cached.title || '',
    icon: habit.icon || cached.icon || '',
    icon_url: habit.icon_url || cached.icon_url || '',
    color: habit.color || cached.color || 'blue',
    days: (habit.days && habit.days.length) ? habit.days : (cached.days || [])
  };
}

function habitsForToday(catalogList, todayPayload, queryDate) {
  var catalogMap = catalogMapFromList(catalogList);
  var todayHabits = extractHabitsFromPayload(todayPayload);
  var byId = {};
  var scheduled = [];
  var doneMap = habitCompletionMap(todayPayload, queryDate);
  var i;
  var id;
  var habit;

  for (i = 0; i < todayHabits.length; i++) {
    habit = enrichHabitFromCatalog(todayHabits[i], catalogMap);
    byId[habit.id] = habit;
    scheduled.push(habit);
  }

  for (id in doneMap) {
    if (!doneMap.hasOwnProperty(id) || byId[id] || !catalogMap[id]) {
      continue;
    }
    habit = catalogMap[id];
    byId[id] = habit;
    scheduled.push(habit);
  }

  for (i = 0; i < catalogList.length; i++) {
    habit = catalogList[i];
    if (!habit || !habit.id || byId[habit.id]) {
      continue;
    }
    if (habitScheduledForDate(habit, queryDate)) {
      byId[habit.id] = habit;
      scheduled.push(habit);
    }
  }

  if (scheduled.length === 0) {
    return scheduledHabitsFromList(catalogList, queryDate);
  }
  return scheduled;
}

function habitIconSlug(habit) {
  var slug = habit.icon || '';
  if (!slug && habit.icon_url) {
    var parts = habit.icon_url.split('/');
    var file = parts[parts.length - 1] || '';
    slug = file.replace(/-[a-f0-9]+\.svg$/i, '');
  }
  return slug || 'star';
}

function pickHabits(catalog, todayPayload, queryDate) {
  var todayHabits = extractHabitsFromPayload(todayPayload);
  var scheduled = habitsForToday(catalog, todayPayload, queryDate);
  var doneMap = habitCompletionMap(todayPayload, queryDate);
  var slots = getHabitSlotNames();
  var hasPinnedSlots = false;
  var picked = [];
  var slotHabits = [null, null, null, null];
  var usedIds = {};
  var icons = [];
  var colors = [];
  var titles = [];
  var mask = 0;
  var s;
  var h;
  var i;
  var habit;

  for (s = 0; s < slots.length; s++) {
    if (normalizeHabitName(slots[s])) {
      hasPinnedSlots = true;
      break;
    }
  }

  if (hasPinnedSlots) {
    for (s = 0; s < MAX_HABITS; s++) {
      if (!normalizeHabitName(slots[s])) {
        continue;
      }
      for (h = 0; h < scheduled.length; h++) {
        habit = scheduled[h];
        if (usedIds[habit.id]) {
          continue;
        }
        if (habitMatchesSlot(habit, slots[s])) {
          slotHabits[s] = habit;
          usedIds[habit.id] = true;
          break;
        }
      }
    }

    for (i = 0; i < scheduled.length; i++) {
      habit = scheduled[i];
      if (usedIds[habit.id]) {
        continue;
      }
      for (s = 0; s < MAX_HABITS; s++) {
        if (!slotHabits[s]) {
          slotHabits[s] = habit;
          usedIds[habit.id] = true;
          break;
        }
      }
    }

    picked = slotHabits;
  } else {
    for (i = 0; i < scheduled.length && picked.length < MAX_HABITS; i++) {
      picked.push(scheduled[i]);
    }
  }

  for (i = 0; i < MAX_HABITS; i++) {
    habit = hasPinnedSlots ? picked[i] : (i < picked.length ? picked[i] : null);
    if (!habit) {
      icons.push('');
      colors.push('');
      continue;
    }
    icons.push(habitIconSlug(habit));
    colors.push(habit.color || 'blue');
    titles.push((habit.title || '?') + (doneMap[habit.id] ? '\u2713' : ''));
    if (doneMap[habit.id]) {
      mask |= (1 << i);
    }
  }

  return {
    count: hasPinnedSlots ? MAX_HABITS : picked.length,
    icons: icons.join('|'),
    colors: colors.join('|'),
    mask: mask,
    titles: titles,
    todayHabitsCount: todayHabits.length,
    scheduledCount: scheduled.length
  };
}

function collectCalendarTodos(recordingsPayload) {
  var todos = recordingsForType(recordingsPayload, 'Calendar::Todo');
  if (todos.length > 0) {
    return todos;
  }
  var payload = unwrapRecordings(recordingsPayload);
  var all = [];
  var key;
  for (key in payload) {
    if (!payload.hasOwnProperty(key)) {
      continue;
    }
    var list = payload[key];
    var i;
    for (i = 0; i < list.length; i++) {
      if (list[i].type === 'CalendarTodo') {
        all.push(list[i]);
      }
    }
  }
  return all;
}

function collectIncompleteTodos(recordingsPayload) {
  var todos = collectCalendarTodos(recordingsPayload);
  var result = [];
  var i;
  for (i = 0; i < todos.length; i++) {
    if (!isZeroDate(todos[i].completed_at)) {
      continue;
    }
    result.push({
      title: todos[i].title || '',
      color: todos[i].color || 'blue'
    });
  }
  return result;
}

function collectCalendarEvents(recordingsPayload) {
  var events = recordingsForType(recordingsPayload, 'Calendar::Event');
  if (events.length > 0) {
    return events;
  }
  var payload = unwrapRecordings(recordingsPayload);
  var all = [];
  var key;
  for (key in payload) {
    if (!payload.hasOwnProperty(key)) {
      continue;
    }
    var list = payload[key];
    var i;
    for (i = 0; i < list.length; i++) {
      if (list[i].type === 'CalendarEvent') {
        all.push(list[i]);
      }
    }
  }
  return all;
}

function parseIsoDate(isoString) {
  if (!isoString) {
    return null;
  }
  var date = new Date(isoString);
  if (isNaN(date.getTime())) {
    return null;
  }
  return date;
}

function formatEventTime(date) {
  var hours = date.getHours();
  var minutes = date.getMinutes();
  var suffix = hours >= 12 ? 'p' : 'a';
  hours = hours % 12;
  if (hours === 0) {
    hours = 12;
  }
  if (minutes === 0) {
    return hours + suffix;
  }
  return hours + ':' + ('0' + minutes).slice(-2) + suffix;
}

function formatEventLine(event, now) {
  var title = (event.title || 'Event').trim();
  if (event.all_day) {
    var prefix = 'All day ';
    if (title.length + prefix.length > 48) {
      return (prefix + title).substring(0, TODO_FOOTER_TEXT_MAX);
    }
    return prefix + title;
  }
  var startsAt = parseIsoDate(event.starts_at);
  if (!startsAt) {
    return title.substring(0, TODO_FOOTER_TEXT_MAX);
  }
  var line = formatEventTime(startsAt) + ' ' + title;
  return line.substring(0, TODO_FOOTER_TEXT_MAX);
}

function eventIsUpcoming(event, now, today) {
  var startsAt = parseIsoDate(event.starts_at);
  var endsAt = parseIsoDate(event.ends_at);
  if (event.all_day) {
    if (!isSameDayAs(event.starts_at, today)) {
      return startsAt && startsAt > now;
    }
    if (endsAt && endsAt <= now) {
      return false;
    }
    return true;
  }
  if (!startsAt) {
    return false;
  }
  if (endsAt && endsAt <= now) {
    return false;
  }
  return startsAt > now || (startsAt <= now && (!endsAt || endsAt > now));
}

function collectUpcomingEvents(recordingsPayload, now, today) {
  var events = collectCalendarEvents(recordingsPayload);
  var upcoming = [];
  var i;
  for (i = 0; i < events.length; i++) {
    if (eventIsUpcoming(events[i], now, today)) {
      upcoming.push(events[i]);
    }
  }
  upcoming.sort(function(a, b) {
    var aTime = parseIsoDate(a.starts_at);
    var bTime = parseIsoDate(b.starts_at);
    if (!aTime && !bTime) {
      return 0;
    }
    if (!aTime) {
      return 1;
    }
    if (!bTime) {
      return -1;
    }
    return aTime.getTime() - bTime.getTime();
  });
  return upcoming;
}

function pickNextEvent(recordingsPayload, today) {
  var now = new Date();
  var upcoming = collectUpcomingEvents(recordingsPayload, now, today);
  if (upcoming.length === 0) {
    return '';
  }
  return formatEventLine(upcoming[0], now);
}

function pickFooter(todayRecordings, weekRecordings, today) {
  var mode = footerContentMode();
  if (mode === 'off') {
    return { text: '', color: 'blue', kind: 0 };
  }
  if (mode === 'event') {
    return {
      text: pickNextEvent(weekRecordings, today),
      color: 'gold',
      kind: 2
    };
  }
  var todo = pickTodo(todayRecordings);
  return { text: todo.text, color: todo.color, kind: 1 };
}

function pickTodo(recordingsPayload) {
  var todos = collectIncompleteTodos(recordingsPayload);
  if (todos.length === 0) {
    todoRotateIndex = 0;
    return { text: '', color: 'blue' };
  }
  if (todoRotateIndex >= todos.length) {
    todoRotateIndex = 0;
  }
  var todo = todos[todoRotateIndex];
  todoRotateIndex = (todoRotateIndex + 1) % todos.length;
  return { text: todo.title, color: todo.color };
}

function themeModeByte() {
  var mode = clayValue(readClaySetting('AppearanceMode', 'light'));
  return String(mode).toLowerCase() === 'dark' ? 1 : 0;
}

function pushThemeToWatch(done) {
  var mode = themeModeByte();
  console.log('Pushing THEME_MODE=' + mode);
  Pebble.sendAppMessage({ 'THEME_MODE': mode }, function() {
    if (done) {
      done();
    }
  }, function(e) {
    console.log('Theme push failed: ' + e.error.message);
    if (done) {
      done();
    }
  });
}

function getStoredTimelinePinIds() {
  try {
    var raw = localStorage.getItem(TIMELINE_PIN_IDS_KEY);
    if (!raw) {
      return [];
    }
    var parsed = JSON.parse(raw);
    return Array.isArray(parsed) ? parsed : [];
  } catch (e) {
    return [];
  }
}

function setStoredTimelinePinIds(ids) {
  localStorage.setItem(TIMELINE_PIN_IDS_KEY, JSON.stringify(ids));
}

function timelinePinIdForEvent(event) {
  var starts = (event.starts_at || '').replace(/[^0-9TZ:-]/g, '').substring(0, 19);
  var id = 'hey-' + event.id + '-' + starts;
  if (id.length > 64) {
    id = id.substring(0, 64);
  }
  return id;
}

function minutesBetween(startIso, endIso) {
  var start = parseIsoDate(startIso);
  var end = parseIsoDate(endIso);
  if (!start || !end) {
    return 60;
  }
  var minutes = Math.round((end.getTime() - start.getTime()) / 60000);
  if (minutes <= 0) {
    return 60;
  }
  return minutes;
}

function buildPinFromEvent(event) {
  return {
    id: timelinePinIdForEvent(event),
    time: event.starts_at,
    duration: minutesBetween(event.starts_at, event.ends_at),
    layout: {
      type: 'calendar',
      title: event.title || 'Event',
      subtitle: event.location || 'Hey Calendar',
      tinyIcon: 'system://TIMELINE_CALENDAR'
    }
  };
}

function timelineRequest(method, pinId, token, body, callback) {
  var headers = {
    'X-User-Token': token
  };
  if (body) {
    headers['Content-Type'] = 'application/json';
  }
  xhrRequest(TIMELINE_API_BASE + encodeURIComponent(pinId), method, headers, body, callback);
}

function deleteTimelinePins(pinIds, token, callback) {
  if (pinIds.length === 0) {
    callback(null);
    return;
  }
  var remaining = pinIds.length;
  var hadError = false;
  var i;
  for (i = 0; i < pinIds.length; i++) {
    (function(pinId) {
      timelineRequest('DELETE', pinId, token, null, function(err, status) {
        if (err || status < 200 || status >= 300) {
          hadError = true;
          console.log('Timeline DELETE failed for ' + pinId + ': ' + status);
        }
        remaining--;
        if (remaining === 0) {
          callback(hadError ? new Error('timeline delete failed') : null);
        }
      });
    })(pinIds[i]);
  }
}

function putTimelinePins(pins, token, callback) {
  if (pins.length === 0) {
    callback(null);
    return;
  }
  var remaining = pins.length;
  var hadError = false;
  var i;
  for (i = 0; i < pins.length; i++) {
    (function(pin) {
      timelineRequest('PUT', pin.id, token, JSON.stringify(pin), function(err, status) {
        if (err || status < 200 || status >= 300) {
          hadError = true;
          console.log('Timeline PUT failed for ' + pin.id + ': ' + status);
        }
        remaining--;
        if (remaining === 0) {
          callback(hadError ? new Error('timeline put failed') : null);
        }
      });
    })(pins[i]);
  }
}

function clearAllTimelinePins(callback) {
  var stored = getStoredTimelinePinIds();
  if (stored.length === 0) {
    callback(null);
    return;
  }
  Pebble.getTimelineToken(function(token) {
    deleteTimelinePins(stored, token, function() {
      setStoredTimelinePinIds([]);
      callback(null);
    });
  }, function() {
    setStoredTimelinePinIds([]);
    callback(new Error('timeline token unavailable'));
  });
}

function syncTimelinePins(recordingsPayload, today) {
  if (!syncTimelineEnabled()) {
    return;
  }

  var now = new Date();
  var events = collectUpcomingEvents(recordingsPayload, now, today).slice(0, MAX_TIMELINE_PINS);
  var pins = [];
  var newIds = [];
  var i;
  for (i = 0; i < events.length; i++) {
    var pin = buildPinFromEvent(events[i]);
    pins.push(pin);
    newIds.push(pin.id);
  }

  Pebble.getTimelineToken(function(token) {
    var stored = getStoredTimelinePinIds();
    var toDelete = [];
    for (i = 0; i < stored.length; i++) {
      if (newIds.indexOf(stored[i]) === -1) {
        toDelete.push(stored[i]);
      }
    }

    deleteTimelinePins(toDelete, token, function() {
      putTimelinePins(pins, token, function(err) {
        if (!err) {
          setStoredTimelinePinIds(newIds);
          console.log('Timeline synced ' + pins.length + ' pins');
        }
      });
    });
  }, function(error) {
    console.log('Timeline token unavailable: ' + error);
  });
}

function sendToWatch(payload) {
  payload.THEME_MODE = themeModeByte();
  Pebble.sendAppMessage(payload, function() {
    console.log('Hey sent: theme=' + payload.THEME_MODE + ' habits=' + payload.HABIT_COUNT +
      ' status=' + payload.SYNC_STATUS);
  }, function(e) {
    console.log('Failed to send Hey data: ' + e.error.message);
  });
}

function sendError(status, reason) {
  console.log('Hey error ' + status + ': ' + reason);
  sendToWatch({
    'HABIT_COUNT': 0,
    'HABIT_DONE_MASK': 0,
    'HABIT_ICONS': '',
    'HABIT_COLORS': '',
    'TODO_TEXT': reason ? reason.substring(0, TODO_FOOTER_TEXT_MAX) : '',
    'TODO_COLOR': 'blue',
    'FOOTER_KIND': 0,
    'SYNC_STATUS': status
  });
}

function fetchHeyData(queryDate) {
  s_fetchId += 1;
  var fetchId = s_fetchId;

  function sendWatchPayload(payload) {
    if (fetchId !== s_fetchId) {
      return;
    }
    sendToWatch(payload);
  }

  function reportError(status, reason) {
    if (fetchId !== s_fetchId) {
      return;
    }
    sendError(status, reason);
  }

  saveSettings();
  var date = queryDate || todayString();

  if (!settings.accessToken) {
    sendWatchPayload({
      'HABIT_COUNT': 0,
      'HABIT_DONE_MASK': 0,
      'HABIT_ICONS': '',
      'HABIT_COLORS': '',
      'TODO_TEXT': 'Add Hey token in watchface settings',
      'TODO_COLOR': 'blue',
      'FOOTER_KIND': 1,
      'SYNC_STATUS': 1
    });
    return;
  }

  apiGet('/calendars.json', fetchId, function(err, status, responseText) {
    if (fetchId !== s_fetchId) {
      return;
    }
    if (err || status < 200 || status >= 300) {
      reportError(status === 401 ? 1 : 2, 'calendars HTTP ' + status);
      return;
    }

    var calendarsPayload;
    try {
      calendarsPayload = JSON.parse(responseText);
    } catch (e) {
      reportError(2, 'calendars JSON parse');
      return;
    }

    var calendarId = findPersonalCalendarId(calendarsPayload);
    if (!calendarId) {
      reportError(2, 'personal calendar not found');
      return;
    }

    var todayPath = '/calendars/' + calendarId + '/recordings.json?starts_on=' +
      date + '&ends_on=' + date;
    var weekPath = '/calendars/' + calendarId + '/recordings.json?starts_on=' +
      date + '&ends_on=' + addDaysString(date, 6);

    apiGet(todayPath, fetchId, function(err2, status2, todayText) {
      if (fetchId !== s_fetchId) {
        return;
      }
      if (err2 || status2 < 200 || status2 >= 300) {
        reportError(status2 === 401 ? 1 : 2, 'recordings HTTP ' + status2);
        return;
      }

      var todayPayload;
      try {
        todayPayload = JSON.parse(todayText);
      } catch (e2) {
        reportError(2, 'recordings JSON parse');
        return;
      }

      apiGet(weekPath, fetchId, function(err3, status3, weekText) {
        if (fetchId !== s_fetchId) {
          return;
        }
        var weekPayload = todayPayload;
        if (!err3 && status3 >= 200 && status3 < 300) {
          try {
            weekPayload = JSON.parse(weekText);
          } catch (e3) {
            weekPayload = todayPayload;
          }
        }

        var catalog = syncHabitCatalog(todayPayload, weekPayload);
        var habits = pickHabits(catalog, todayPayload, date);
        var footer = pickFooter(todayPayload, weekPayload, date);
        var todoCount = collectIncompleteTodos(todayPayload).length;
        var maskBits = '0b' + habits.mask.toString(2);

        console.log('Hey sync ' + date + ': catalog=' + catalog.length +
          ' todayHabits=' + habits.todayHabitsCount +
          ' scheduled=' + habits.scheduledCount + ' picked=' + habits.count +
          ' mask=' + maskBits + ' [' + habits.titles.join(', ') + '], ' +
          todoCount + ' todos, footer=' + footer.text + ', mode=' +
          footerContentMode());

        sendWatchPayload({
          'HABIT_COUNT': habits.count,
          'HABIT_DONE_MASK': habits.mask,
          'HABIT_ICONS': habits.icons,
          'HABIT_COLORS': habits.colors,
          'TODO_TEXT': footer.text.substring(0, TODO_FOOTER_TEXT_MAX),
          'TODO_COLOR': footer.color || 'blue',
          'FOOTER_KIND': footer.kind,
          'SYNC_STATUS': 0
        });

        syncTimelinePins(weekPayload, date);
      });
    });
  });
}

Pebble.addEventListener('ready', function() {
  console.log('Hey pkjs ready');
  pushThemeToWatch(function() {
    fetchHeyData();
  });
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.REQUEST_HEY_DATA) {
    fetchHeyData(e.payload.QUERY_DATE);
  }
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response === 'CANCELLED' || !e.response) {
    return;
  }
  var config = JSON.parse(decodeURIComponent(e.response));
  if (isSettingEnabled(config.DisconnectHey)) {
    clearHeyCredentials();
    persistPreferenceSettings(config);
    pushThemeToWatch(function() {
      fetchHeyData();
    });
    return;
  }
  saveHeyTokenSettings(config);
  var wasTimelineEnabled = syncTimelineEnabled();
  persistPreferenceSettings(config);
  pushThemeToWatch(function() {
    if (config.SyncTimeline !== undefined) {
      var enableTimeline = isSettingEnabled(config.SyncTimeline);
      if (wasTimelineEnabled && !enableTimeline) {
        clearAllTimelinePins(function() {
          fetchHeyData();
        });
        return;
      }
    }
    fetchHeyData();
  });
});
