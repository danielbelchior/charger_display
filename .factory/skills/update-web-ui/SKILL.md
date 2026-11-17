---
name: update-web-ui
description: Edit the charger_display web control panel and sync it into the firmware. Use when changing control-panel/index.html, the dashboard look, status fields shown in the UI, or main/http_server_index.h.
---

# Update the web UI

`control-panel/index.html` is the source. The firmware serves a copy embedded in `main/http_server_index.h`. See [ADR 0003](../../../docs/adr/0003-self-contained-web-ui.md).

## Rules

- One self-contained file: inline CSS and JS, system fonts, no CDN, no external images. The display often has no internet access.
- Relative URLs for REST (`/status`, `/sensor/...`, `/brightness/...`, `/boot`); status WebSocket at `ws://<location.hostname>:81/`.
- Keep demo mode working: opening via `file://` or with `?demo` must animate sample data without network calls.
- Flash is limited; avoid large inline assets (base64 images, icon fonts).

## Steps

1. Edit `control-panel/index.html`.
2. Preview: open it in a browser via `file://` (demo mode). If a headless browser is available, take a screenshot to check layout at desktop and phone widths.
3. If the UI needs a new field, add it to the `/status` JSON in `main/http_server.cpp` and document it in `docs/api.md`.
4. Sync the header. With the hook enabled (`git config core.hooksPath .githooks`), committing the HTML regenerates it (minified if `html-minifier-terser` or `html-minifier` is installed). Otherwise, write it unminified:

   ```sh
   { printf 'const char index_html[] PROGMEM = R"rawliteral(\n'; cat control-panel/index.html; printf ')rawliteral";\n'; } > main/http_server_index.h
   ```

   The HTML must not contain the sequence `)rawliteral"`.
5. Check the copy matches: the content between the markers in `main/http_server_index.h` equals `control-panel/index.html` (or its minified form).
6. If the look changed noticeably, update `webui-demo.png` (screenshot of demo mode) used by the README.
