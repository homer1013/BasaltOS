const { chromium } = require('playwright');

const BASE = process.env.BASALT_UI_URL || 'http://127.0.0.1:5000';

function fail(msg) {
  throw new Error(msg);
}

async function waitReady(page) {
  await page.waitForFunction(() => {
    const sel = document.getElementById('platform-select');
    return !!sel && sel.options && sel.options.length > 1;
  }, { timeout: 20000 });
}

async function assertHidden(page, selector, name) {
  const el = page.locator(selector);
  if (await el.count() === 0) return;
  if (await el.first().isVisible()) fail(`${name} should be hidden in local mode`);
}

async function run() {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1400, height: 900 } });

  await page.goto(BASE, { waitUntil: 'domcontentloaded' });
  await waitReady(page);

  // Top-nav and profile affordances must stay hidden in local mode.
  await assertHidden(page, '#btn-market', 'market nav button');
  await assertHidden(page, '#menu-profile', 'profile menu item');
  await assertHidden(page, '#menu-switch-user', 'switch-user menu item');
  await assertHidden(page, '#top-profile-chip', 'top profile chip');
  await assertHidden(page, '#btn-open-market-from-config', 'config market shortcut');
  await assertHidden(page, '#stat-market', 'landing market stat');

  // Market/profile shells should not be displayed.
  await assertHidden(page, '#market-shell', 'market shell');
  await assertHidden(page, '#profile-shell', 'profile shell');

  // Enter configurator and confirm hidden-state remains consistent.
  await page.click('#btn-open-config');
  await page.waitForFunction(() => {
    const el = document.getElementById('configurator-shell');
    return !!el && getComputedStyle(el).display !== 'none';
  }, { timeout: 15000 });

  await assertHidden(page, '#btn-market', 'market nav button after entering configurator');
  await assertHidden(page, '#menu-profile', 'profile menu item after entering configurator');
  await assertHidden(page, '#menu-switch-user', 'switch-user menu item after entering configurator');
  await assertHidden(page, '#btn-open-market-from-config', 'config market shortcut after entering configurator');

  // API should reject market endpoints in local mode.
  const res = await fetch(`${BASE}/api/market/catalog?platform=esp32`);
  if (res.status !== 404) fail(`expected market catalog 404, got ${res.status}`);
  const body = await res.text();
  if (!body.includes('disabled in local configurator mode')) {
    fail('market catalog 404 response did not include local-mode disabled message');
  }

  await browser.close();
  console.log('PASS: local mode UI/API guard checks');
}

run().catch((e) => {
  console.error('FAIL:', e && e.message ? e.message : String(e));
  process.exit(1);
});
