# Pebble App Store Listing

## App Name
Hey

## Tagline
Your Hey calendar on your wrist — habits, todos, and events

## Description
A read-only Pebble watchface that mirrors your **Hey** calendar — today's habits, rotating todos, and upcoming events, styled with Hey's official colors and icons.

**Features:**
- Up to 4 habit chips in screen corners with official Hey icons and colors
- Filled chip when complete, colored ring when not — same size either way
- Circular analog clock with Hey blue/purple hands and brand-colored ticks
- Single-line date above the clock; tinted todo/event pill below (no checkbox)
- Light and dark themes with warm off-white and Hey dark palettes
- Rotating todo footer tinted by Hey color, next calendar event, or hidden
- Optional sync of Hey calendar events to the Pebble Timeline
- Pin specific habits to corner quadrants via settings

Requires a Hey account and personal access token from [hey-cli](https://github.com/basecamp/hey-cli). Habits are completed in the Hey app on your phone; this watchface displays them only.

**Platform:** Pebble Time 2 (Emery) — 200×228 color display

## Category
Watchfaces > Productivity

## Keywords
hey, habits, calendar, todos, productivity, 37signals, basecamp, emery, pebble time 2

## Banner Text
Hey habits on your wrist

## Version History

### v1.0.7
- Redesign: corner habit quadrants, circular analog clock, single-line date
- Colorful Hey palette: brand hands/ticks, colored habit rings, warm light background
- Todo footer: tinted pill by Hey color, no checkbox (read-only)

### v1.0.6
- Fix: pinned habit slots stay in labeled quadrants instead of collapsing to first positions

### v1.0.5
- Fix: dark mode, footer mode, and timeline toggle now read/write Clay clay-settings correctly
- Fix: AppearanceMode was never persisted when saving settings

### v1.0.4
- Fix: Disconnect toggle now works (Clay sends 1/0, not true/false)
- Fix: clear tokens from Clay's clay-settings blob, not just loose localStorage keys
- Fix: cancel in-flight syncs so stale API responses cannot restore your data

### v1.0.3
- Settings: computer setup steps shown on separate lines for readability

### v1.0.2
- Fix: changing access token no longer silently re-authenticates via saved refresh token
- Add "Disconnect Hey account" toggle in settings
- Empty token fields now properly clear saved credentials

### v1.0.1
- Fix: access token no longer bundled in app package
- Clearer settings instructions for token setup on a computer
- Empty habit grid until user saves their own token

### v1.0.0
- Initial release
- Hey habit sync with persistent catalog (completed habits stay visible)
- 47 official Hey habit icons at native 20×20 resolution
- Light/dark theme, footer modes, habit slot pinning
- Optional Pebble Timeline sync for Hey calendar events

## Support
Report issues in the repository where you obtained this watchface.

## Screenshots

1. `emery_screenshot.png` — Light theme with corner habits, analog clock, tinted todo footer

## App Information
- **Author**: Kyle Dellis
- **Category**: Watchfaces
- **Platform**: Emery (Pebble Time 2)
- **Requires**: Phone-side Hey API access via personal token

## Privacy

Your Hey access token is stored in Pebble app localStorage on your phone. API requests go directly from your phone to `app.hey.com`. No third-party servers are involved.
