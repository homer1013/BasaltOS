# Web Configurator E2E

These are lightweight Playwright-based checks for `tools/basaltos_config_gui.html`.

## Prereqs

1. Start the configurator server:

```bash
python tools/basaltos_config_server.py
```

2. In another terminal, install Playwright once (in any temp folder or project-local):

```bash
npm install playwright@1.51.1
npx playwright install chromium
```

## Run

Smoke suite:

```bash
node tools/e2e/smoke_configurator.js
```

Deep suite:

```bash
node tools/e2e/deep_configurator.js
```

Optional URL override:

```bash
BASALT_UI_URL=http://127.0.0.1:5000 node tools/e2e/smoke_configurator.js
```
