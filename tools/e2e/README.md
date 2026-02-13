# Web Configurator E2E

These are lightweight Playwright-based checks for `tools/basaltos_config_gui.html`.

## Prereqs

1. Start the configurator server:

```bash
python tools/basaltos_config_server.py
```

2. Install harness dependencies (project-local):

```bash
npm --prefix tools/e2e install
npm --prefix tools/e2e run e2e:install-browsers
```

## Run

Smoke suite:

```bash
npm --prefix tools/e2e run e2e:smoke
```

Deep suite:

```bash
npm --prefix tools/e2e run e2e:deep
```

Local-mode guard suite:

```bash
npm --prefix tools/e2e run e2e:local
```

All suites:

```bash
npm --prefix tools/e2e run e2e:all
```

Optional URL override:

```bash
BASALT_UI_URL=http://127.0.0.1:5000 npm --prefix tools/e2e run e2e:smoke
```
