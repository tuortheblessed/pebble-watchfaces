# Hey Watchface

Pebble watchface for **Hey** habits, todos, and calendar events. Targets **Emery** (Pebble Time 2, 200×228).

Complete habits in the Hey app on your phone — this watchface is read-only and mirrors your Hey data.

> Unofficial community project. Not affiliated with 37signals or Hey.

---

## Quick start (watch users)

After installing from the Pebble app store:

1. On a **Mac or Windows computer**, open **Terminal** (Mac) or **PowerShell** (Windows).
2. Install [hey-cli](https://github.com/basecamp/hey-cli) and run `hey auth login` (sign in to Hey in the browser).
3. Run `hey auth token` and **copy the long code** it prints.
4. On your **phone**, open this watchface's **Settings** (long-press the watchface → Settings).
5. Paste the code into **Access Token**, tap **Save**.

Your habits appear on the watch within about a minute. Complete habits in the Hey app on your phone — this watchface only displays them.

---

## Developer setup

### 1. Install hey-cli and log in

```bash
go install github.com/basecamp/hey-cli/cmd/hey@latest
export PATH="$HOME/go/bin:$PATH"   # add to ~/.zshrc to persist
hey auth login
```

### 2. Get your access token

```bash
hey auth token
```

Copy the printed token (a long string). Paste it into watchface settings on your phone or in the emulator config UI.

**Optional — auto-refresh when the token expires:**

```bash
hey auth status --json
```

Copy `refresh_token` and `token_endpoint` into the matching optional fields in watchface settings.

### 3. Build and install

```bash
cd hey-watchface
npm install
pebble build
pebble install --emulator emery    # emulator
pebble install --phone             # physical watch via phone
```

### 4. Open settings and save your token

**Emulator:**

```bash
pebble install --emulator emery
pebble emu-app-config --emulator emery
```

Paste the access token, click **Save**. Live Hey data appears within about a minute.

**On a phone:** Long-press the watchface → **Settings**.

> **Publishing:** Never put tokens in source files. Settings come only from Clay/localStorage on each user's phone. After changing pkjs, verify the PBW is clean: `unzip -p build/hey-watchface.pbw pebble-js-app.js | grep -c eyJfcmF` should print `0`.

---

## Settings reference

| Setting | What it does |
|---------|----------------|
| **Access Token** | Required. From `hey auth token` |
| **Refresh Token** | Optional. Keeps sync working when the access token expires |
| **Token Endpoint** | Optional. From `hey auth status --json` |
| **Appearance** | Light or dark; uses Hey's official color palettes |
| **Footer shows** | Bottom bar: rotating todos (default), next calendar event, or hidden |
| **Event calendars** | Optional filter for footer/Timeline events — comma-separated Hey calendar names or IDs. Blank = all calendars |
| **Sync Hey events to Pebble Timeline** | Off by default. Pushes Hey calendar events to the Pebble Timeline |
| **Habit slots 1–4** | Pin habits by name to quadrants (top-left, top-right, bottom-left, bottom-right) |

---

## What the watchface shows

### Habits (corner quadrants)

- Up to **4** habit chips in the **screen corners** (outside the clock ring)
- Official Hey icons (20×20 PNGs) and colors on 32px chips
- **Complete:** filled circle in habit color, contrasting icon
- **Incomplete:** 2px ring in the habit's own color (not gray)
- Only habits **scheduled for today** — matched to Hey's today `recordings.json` (not re-filtered from the cached catalog `days` field)
- Completed habits remain visible (catalog persisted across syncs)

| Habits scheduled today | Layout |
|------------------------|--------|
| 0 | Empty corners; date, clock, and footer still show |
| 1–4 | Pinned to quadrants via settings, or auto-filled TL → TR → BL → BR |
| 5+ | First 4 after slot prefs, then Hey order |

### Date and clock

- Single-line date (`Fri Jun 26`) above the clock hub
- Circular analog clock: purple hour hand, blue minute hand, brand-colored quarter ticks
- Warm off-white background in light mode; Hey dark blue-gray in dark mode

### Footer (below clock)

| Footer setting | Behavior |
|----------------|----------|
| **Todos (rotate)** | Incomplete todos for **today** cycle every sync (~60s); soft lavender pill (theme color) |
| **Next calendar event** | Next upcoming Hey event in the same lavender pill, e.g. `4:45p Little Ninjas`. Times use each event's Hey timezone (not your phone's local time) |
| **Nothing** | Footer hidden |

### Calendar events (footer and Timeline)

Hey stores habits/todos on your **personal** calendar and calendar events on separate calendars (e.g. "Personal HEY", "Family"). v1.0.8 fetches events from all Hey calendars (or only those listed in **Event calendars**) while habits still sync from the personal calendar only.

Event times in the footer match the Hey app: a Family event in `America/Chicago` shows `4:45p`, not the time converted to your phone's timezone.

### Pebble Timeline (opt-in)

When **Sync Hey events to Pebble Timeline** is enabled:

- Hey `Calendar::Event` items from all (or filtered) event calendars become Timeline pins on your watch
- View them in the **system Timeline** (scroll with Up/Down) — not on the watchface canvas
- Syncs on each refresh; turning the toggle off removes synced pins
- Timeline pin uploads run every 3rd sync to reduce phone network use
- **Requires a real phone** with Rebble/Pebble app — Timeline PUT does not work in the QEMU emulator

---

## How sync works

```
Watch  ←AppMessage→  Phone (pkjs)  ←HTTPS→  app.hey.com
```

- Watch requests data every **60 seconds** (aligned with the minute tick)
- Token and API calls run on the **phone** (or emulator), not on the watch
- Habits/todos: personal Hey calendar only. Events/Timeline: all calendars (or **Event calendars** filter)
- `SYNC_STATUS` on the watch: `0` = OK, `1` = auth error, `2` = other error
- Habit catalog is cached in phone localStorage so completed habits are not dropped when Hey omits them from today's recording payload

---

## Project layout

```
hey-watchface/
├── src/c/main.c              # Watchface rendering and AppMessage handling
├── src/c/habit_icons.gen.c   # Generated icon slug → resource ID lookup
├── src/pkjs/index.js         # Hey API sync, habit catalog, timeline
├── src/pkjs/config.js        # Clay settings UI
├── scripts/
│   ├── generate_habit_icons.py
│   └── habit_icon_resources.json
├── resources/images/habits/  # 47 icon pairs + default (committed PNGs)
├── package.json              # Pebble manifest and resource list
├── wscript
├── emery_screenshot.png      # Store / preview screenshot
└── STORE.md                  # App store listing draft
```

---

## Regenerating habit icons

Icons are downloaded from Hey's CDN, rasterized from SVG to **20×20** PNGs (16px max content), and pebbleized for crisp 1:1 drawing on the watch.

**Requirements:** Python 3, Pillow (`pip install Pillow`), Node/npx (for `@resvg/resvg-js-cli`).

```bash
# Full regen — all 47 icons (~30–90 min; safe to interrupt and resume)
python3 scripts/generate_habit_icons.py

# Resume specific slugs after an interrupted run
python3 scripts/generate_habit_icons.py --only tree,tv,walk

# Re-center existing PNGs without re-downloading
python3 scripts/generate_habit_icons.py --pad-only

pebble build
```

The script updates `habit_icon_resources.json`, `src/c/habit_icons.gen.c`, and `package.json` automatically.

---

## Publishing

See `STORE.md` for listing copy. To publish to the Pebble App Store:

```bash
pebble login
pebble publish
```

Or non-interactive:

```bash
pebble publish --non-interactive --description "Hey habits, todos, and calendar on Pebble Time 2"
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| "Set Hey token in settings" | Open settings, paste `hey auth token`, Save |
| Auth error / habits empty | Token expired — add refresh token + endpoint, or run `hey auth login` again |
| Wrong habits showing | Check habit slot names; only today's scheduled habits appear |
| Habits vanish when all complete | Update to latest pkjs — catalog cache keeps all 4 slots filled |
| Footer empty in "Next calendar event" mode | Events live on separate Hey calendars — update to v1.0.8+; optionally set **Event calendars** |
| Event time wrong vs Hey app | Footer uses event timezone (US zones supported); unknown zones fall back to UTC |
| Timeline empty | Enable toggle in settings; install via Rebble/Pebble app on a **real phone** (not emulator); Timeline must be enabled for app UUID `32fc2d81-66e4-45ee-8589-2350312ed88c` in Rebble dev portal if PUT fails |
| Footer overlaps habits | Rebuild latest version — footer sits below the clock ring |

---

## License

Community project for personal use. Hey name, icons, and API are property of 37signals. Use your own Hey account and token.
