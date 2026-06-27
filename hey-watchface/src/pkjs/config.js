module.exports = [
  {
    "type": "heading",
    "defaultValue": "Connect your Hey account"
  },
  {
    "type": "text",
    "defaultValue": "You need a personal access token from Hey. This is a one-time setup on a Mac or Windows computer — not on your phone."
  },
  {
    "type": "text",
    "defaultValue": "On your computer:\n1. Open Terminal (Mac) or PowerShell (Windows)\n2. Install hey-cli (see github.com/basecamp/hey-cli)\n3. Run: hey auth login\n   Sign in to Hey when the browser opens\n4. Run: hey auth token\n   Copy the long code it prints"
  },
  {
    "type": "text",
    "defaultValue": "Paste that code into Access Token below, then tap Save. Your watch will show your Hey habits within about 30 seconds."
  },
  {
    "type": "input",
    "messageKey": "HeyAccessToken",
    "label": "Access Token (required)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Paste the code from: hey auth token",
      "limit": 512
    }
  },
  {
    "type": "text",
    "defaultValue": "Optional — keeps sync working when the token expires. On your computer run: hey auth status --json and copy refresh_token and token_endpoint below."
  },
  {
    "type": "input",
    "messageKey": "HeyRefreshToken",
    "label": "Refresh Token (optional)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "From hey auth status --json",
      "limit": 512
    }
  },
  {
    "type": "input",
    "messageKey": "HeyTokenEndpoint",
    "label": "Token Endpoint (optional)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "From hey auth status --json",
      "limit": 256
    }
  },
  {
    "type": "heading",
    "defaultValue": "Appearance"
  },
  {
    "type": "select",
    "messageKey": "AppearanceMode",
    "label": "Theme",
    "defaultValue": "light",
    "options": [
      { "label": "Light", "value": "light" },
      { "label": "Dark", "value": "dark" }
    ]
  },
  {
    "type": "heading",
    "defaultValue": "Footer"
  },
  {
    "type": "select",
    "messageKey": "FooterContent",
    "label": "Footer shows",
    "defaultValue": "todos",
    "options": [
      { "label": "Todos (rotate)", "value": "todos" },
      { "label": "Next calendar event", "value": "event" },
      { "label": "Nothing", "value": "off" }
    ]
  },
  {
    "type": "toggle",
    "messageKey": "SyncTimeline",
    "label": "Sync Hey events to Pebble Timeline",
    "defaultValue": false,
    "description": "Pushes Hey calendar events to the Pebble Timeline (scroll with Up/Down). Uses extra phone network on each sync."
  },
  {
    "type": "heading",
    "defaultValue": "Habit slots"
  },
  {
    "type": "text",
    "defaultValue": "Optional: pin habits by their exact Hey name to a corner of the watchface (top-left, top-right, bottom-left, bottom-right). Leave blank to show today's habits automatically."
  },
  {
    "type": "input",
    "messageKey": "HabitSlot1",
    "label": "Slot 1 (top-left)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Exact habit name in Hey",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot2",
    "label": "Slot 2 (top-right)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Exact habit name in Hey",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot3",
    "label": "Slot 3 (bottom-left)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Exact habit name in Hey",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot4",
    "label": "Slot 4 (bottom-right)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Exact habit name in Hey",
      "limit": 48
    }
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
