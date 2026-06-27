# Hey Watchface

Pebble watchface for **Hey** habits, todos, and calendar events. Targets **Emery** (Pebble Time 2, 200×228).

Complete habits in the Hey app on your phone — this watchface is read-only and mirrors your Hey data.

> Unofficial community project. Not affiliated with 37signals or Hey.

---

## Quick start

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

Copy the printed token (a long string). You will paste this into watchface settings.

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
# Terminal 1 — emulator with watchface running
pebble install --emulator emery

# Terminal 2 — settings page in browser
pebble emu-app-config --emulator emery
```

Paste the access token, configure options below, click **Save**. Live Hey data appears within ~30 seconds.

**On a phone:** Install via CloudPebble or the Pebble app, then long-press the watchface → **Settings**.

### Optional: dev token file (emulator)

Avoid reopening settings on every rebuild:

```bash
cp src/pkjs/dev-settings.js.example src/pkjs/dev-settings.js
# edit dev-settings.js with your token, then:
pebble build && pebble install --emulator emery
```

`dev-settings.js` is gitignored — never commit real tokens.

---

## Settings reference

| Setting | What it does |
|---------|----------------|
| **Access Token** | Required. From `hey auth token` |
| **Refresh Token** | Optional. Keeps sync working when the access token expires |
| **Token Endpoint** | Optional. From `hey auth status --json` |
| **Appearance** | Light or dark; uses Hey's official color palettes |
| **Footer shows** | Bottom bar: rotating todos (default), next calendar event, or hidden |
| **Sync Hey events to Pebble Timeline** | Off by default. Pushes Hey calendar events to the Pebble Timeline |
| **Habit slots 1–4** | Pin habits by name to quadrants (top-left, top-right, bottom-left, bottom-right) |

---

## What the watchface shows

### Habits (center grid)

- Up to **4** habit chips in a 2×2 grid around the date
- Official Hey icons (20×20 PNGs) and colors on 32px chips
- **Complete:** filled circle in habit color, contrasting icon
- **Incomplete:** 2px outline ring, dark icon on light / light icon on dark
- Only habits **scheduled for today** (Hey `days` field: Mon=0 … Sun=6)
- Completed habits remain visible (catalog persisted across syncs)

| Habits scheduled today | Layout |
|------------------------|--------|
| 0 | Empty grid; date and footer still show |
| 1–3 | Top-left → top-right → bottom-left |
| 4 | Full 2×2 grid |
| 5+ | First 4 after slot prefs, then Hey order |

### Date and time ring

- Two-line date (weekday + month/day) centered between chips
- Tick ring: current minute in Hey blue, current hour in Hey purple

### Footer (bottom bar)

| Footer setting | Behavior |
|----------------|----------|
| **Todos (rotate)** | Incomplete todos for **today** cycle every sync (~30s); wraps to two lines when needed |
| **Next calendar event** | Next upcoming Hey event (today through +6 days), e.g. `2:30p Team standup` |
| **Nothing** | Footer hidden |

### Pebble Timeline (opt-in)

When **Sync Hey events to Pebble Timeline** is enabled:

- Hey `Calendar::Event` items become Timeline pins on your watch
- View them in the **system Timeline** (scroll with Up/Down) — not on the watchface canvas
- Syncs on each refresh; turning the toggle off removes synced pins

---

## How sync works

```
Watch  ←AppMessage→  Phone (pkjs)  ←HTTPS→  app.hey.com
```

- Watch requests data every **30 seconds**
- Token and API calls run on the **phone** (or emulator), not on the watch
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
├── screenshot_emery.png      # Store / preview screenshot
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
| Timeline empty | Enable toggle in settings; timeline token may require install via Rebble/Pebble app |
| Footer overlaps ticks | Rebuild latest version — footer is anchored above tick clearance |

---

## License

Community project for personal use. Hey name, icons, and API are property of 37signals. Use your own Hey account and token.
