# 0003. The web UI is one self-contained file

Status: Accepted
Date: 2026-11-17

## Context

The control panel is served by the ESP32 from flash. The display often sits on an IoT VLAN without internet access, and the old page loaded fonts and scripts from CDNs, so it rendered broken or slowly there. Absolute URLs also broke access by IP address.

## Decision

- `control-panel/index.html` is a single file with inline CSS and JS. No CDN, no web fonts, no external images.
- All requests use relative URLs; the status WebSocket uses `location.hostname` with port 81.
- Opening the file from `file://` or with `?demo` runs demo mode with sample data, so design work needs no device.
- The firmware serves a copy embedded in `main/http_server_index.h` as a raw string literal.

## Consequences

- Every byte counts against flash; keep the page lean and system-font based.
- `control-panel/index.html` and `main/http_server_index.h` must stay in sync. The pre-commit hook in `.githooks/` regenerates the header from the staged HTML (minified when `html-minifier-terser` or `html-minifier` is installed, unminified otherwise).
- New status fields need changes in three places: `http_server.cpp`, the UI, and `docs/api.md`.
