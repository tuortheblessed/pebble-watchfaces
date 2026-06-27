# Pebble App Store Listing

## App Name
Hey

## Tagline
Your Hey calendar on your wrist — habits, todos, and events

## Description
A read-only Pebble watchface that mirrors your **Hey** calendar — today's habits, rotating todos, and upcoming events, styled with Hey's official colors and icons.

**Features:**
- Up to 4 habit chips in a 2×2 grid with official Hey icons and colors
- Filled chip when complete, outlined ring when not — same size either way
- Light and dark themes matching Hey's palettes
- Rotating todo footer, next calendar event, or hidden
- Minute/hour tick ring with Hey blue and purple accents
- Optional sync of Hey calendar events to the Pebble Timeline
- Pin specific habits to quadrants via settings

Requires a Hey account and personal access token from [hey-cli](https://github.com/basecamp/hey-cli). Habits are completed in the Hey app on your phone; this watchface displays them only.

**Platform:** Pebble Time 2 (Emery) — 200×228 color display

## Category
Watchfaces > Productivity

## Keywords
hey, habits, calendar, todos, productivity, 37signals, basecamp, emery, pebble time 2

## Banner Text
Hey habits on your wrist

## Version History

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

1. `emery_screenshot.png` — Light theme with habit grid and todo footer

## App Information
- **Author**: Kyle Dellis
- **Category**: Watchfaces
- **Platform**: Emery (Pebble Time 2)
- **Requires**: Phone-side Hey API access via personal token

## Privacy

Your Hey access token is stored in Pebble app localStorage on your phone. API requests go directly from your phone to `app.hey.com`. No third-party servers are involved.
