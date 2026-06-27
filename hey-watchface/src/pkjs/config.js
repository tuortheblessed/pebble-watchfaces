module.exports = [
  {
    "type": "heading",
    "defaultValue": "Hey"
  },
  {
    "type": "text",
    "defaultValue": "1) Install hey-cli: go install github.com/basecamp/hey-cli/cmd/hey@latest\n2) hey auth login\n3) hey auth token — paste below\n\nOptional: hey auth status --json for refresh token fields."
  },
  {
    "type": "input",
    "messageKey": "HeyAccessToken",
    "label": "Access Token",
    "defaultValue": "",
    "attributes": {
      "placeholder": "Paste token from hey auth token",
      "limit": 512
    }
  },
  {
    "type": "input",
    "messageKey": "HeyRefreshToken",
    "label": "Refresh Token (optional)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "For auto-refresh",
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
    "type": "select",
    "messageKey": "AppearanceMode",
    "label": "Appearance",
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
    "defaultValue": "Pin habits by name (only habits scheduled for today are shown). Slots: top-left, top-right, bottom-left, bottom-right. Leave blank to auto-fill."
  },
  {
    "type": "input",
    "messageKey": "HabitSlot1",
    "label": "Slot 1 (top-left)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "e.g. Love",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot2",
    "label": "Slot 2 (top-right)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "e.g. Kettlebells",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot3",
    "label": "Slot 3 (bottom-left)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "e.g. Bible",
      "limit": 48
    }
  },
  {
    "type": "input",
    "messageKey": "HabitSlot4",
    "label": "Slot 4 (bottom-right)",
    "defaultValue": "",
    "attributes": {
      "placeholder": "e.g. Help Kids",
      "limit": 48
    }
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
