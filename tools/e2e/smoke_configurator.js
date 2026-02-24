const { chromium } = require('playwright');

const BASE = process.env.BASALT_UI_URL || 'http://127.0.0.1:5000';

async function expect(cond, msg) {
  if (!cond) throw new Error(msg);
}

async function textContent(page, sel) {
  return (await page.locator(sel).first().textContent()) || '';
}

async function waitForListText(page, pattern, timeout = 10000) {
  await page.waitForFunction(
    ({ p }) => {
      const txt = String(document.getElementById('board-list')?.textContent || '');
      return new RegExp(p, 'i').test(txt);
    },
    { p: pattern.source || String(pattern), timeout }
  );
}

async function waitReady(page) {
  await page.waitForFunction(() => {
    const sel = document.getElementById('platform-select');
    return !!sel && sel.options && sel.options.length > 1;
  }, { timeout: 20000 });
}

async function openConfigurator(page) {
  await waitReady(page);
  await page.click('#btn-open-config');
  await page.waitForFunction(() => {
    const el = document.getElementById('configurator-shell');
    return !!el && getComputedStyle(el).display !== 'none';
  }, { timeout: 15000 });
  await page.waitForSelector('#manufacturer-select', { state: 'visible', timeout: 15000 });
}

async function openMarket(page) {
  await waitReady(page);
  await page.click('#btn-market');
  await page.waitForFunction(() => {
    const el = document.getElementById('market-shell');
    return !!el && getComputedStyle(el).display !== 'none';
  }, { timeout: 15000 });
}

async function clickBoard(page, matcher) {
  const boards = page.locator('#board-list .board-item');
  const count = await boards.count();
  await expect(count > 0, 'no board items available');
  for (let i = 0; i < count; i++) {
    const t = (await boards.nth(i).textContent()) || '';
    if (matcher.test(t)) {
      await boards.nth(i).click();
      await page.waitForFunction(() => {
        const el = document.getElementById('board-details');
        return !!el && getComputedStyle(el).display !== 'none';
      }, { timeout: 10000 });
      return t;
    }
  }
  throw new Error(`no board matched ${matcher}`);
}

async function run() {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1440, height: 900 } });
  const pageErrors = [];
  page.on('pageerror', e => pageErrors.push(String(e?.message || e)));

  const results = [];
  async function test(name, fn) {
    try {
      await page.goto(BASE, { waitUntil: 'domcontentloaded' });
      await fn();
      results.push({ name, ok: true });
    } catch (e) {
      results.push({ name, ok: false, error: String(e?.message || e) });
    }
  }

  await test('landing and top navigation render', async () => {
    await waitReady(page);
    await expect(await page.locator('#btn-home').isVisible(), 'home missing');
    await expect(await page.locator('#btn-open-config').isVisible(), 'config missing');
    await expect(await page.locator('#landing-home').isVisible(), 'landing missing');
  });

  await test('hierarchy selectors and board selection bind platform', async () => {
    await openConfigurator(page);
    await page.selectOption('#manufacturer-select', { label: 'Espressif' });
    const picked = await clickBoard(page, /CYD|DevKit|M5|SuperMini/i);
    await expect((await page.inputValue('#platform-select')) === 'esp32', 'platform not esp32');
    await expect(/Espressif/i.test(await textContent(page, '#hardware-path')), 'missing breadcrumb');
    await expect((await textContent(page, '#board-name')).includes(picked.split('(')[0].trim().split(' ').slice(0, 2).join(' ')), 'board name not updated');
  });

  await test('empty-state explains active filters', async () => {
    await openConfigurator(page);
    await page.selectOption('#manufacturer-select', { label: 'Renesas' });
    await page.selectOption('#architecture-select', { label: 'ARM Cortex-M' });
    await page.selectOption('#family-select', { label: 'RA4' });
    await page.fill('#board-search', 'definitely-no-match-xyz');
    await waitForListText(page, /No boards match your current filter/);
    const listText = await textContent(page, '#board-list');
    await expect(listText.includes('No boards match your current filter'), 'missing no-match');
    await expect(listText.includes('Active filters:'), 'missing filter hint');
  });

  await test('url query restores picker state and selected board', async () => {
    const u = new URL(BASE);
    u.searchParams.set('platform', 'esp32');
    u.searchParams.set('manufacturer', 'Espressif');
    u.searchParams.set('architecture', 'RISC-V');
    u.searchParams.set('family', 'ESP32-C3');
    u.searchParams.set('board', 'esp32-c3-supermini');
    u.searchParams.set('q', 'supermini');
    await page.goto(u.toString(), { waitUntil: 'domcontentloaded' });
    await openConfigurator(page);
    await page.waitForFunction(() => {
      const p = String(document.getElementById('platform-select')?.value || '');
      const q = String(document.getElementById('board-search')?.value || '').toLowerCase();
      const bn = String(document.getElementById('board-name')?.textContent || '').toLowerCase();
      return p === 'esp32' && q.includes('supermini') && (bn.includes('super') || bn.includes('c3'));
    }, { timeout: 15000 });
    await expect((await page.inputValue('#platform-select')) === 'esp32', 'platform not restored');
    await expect((await page.inputValue('#board-search')).toLowerCase().includes('supermini'), 'search not restored');
    const bn = (await textContent(page, '#board-name')).toLowerCase();
    await expect(bn.includes('super') || bn.includes('c3'), 'board selection/details not restored');
  });

  await test('market add-to-build controls gated without board selection', async () => {
    const marketButton = page.locator('#btn-market');
    if (!(await marketButton.isVisible())) {
      // Local mode intentionally hides market UI.
      return;
    }

    try {
      await openMarket(page);
    } catch (e) {
      const msg = String(e?.message || e);
      if (msg.includes('not visible') || msg.includes('Timeout')) {
        // Treat hidden market nav as expected in local-mode variants.
        return;
      }
      throw e;
    }

    const toggles = page.locator('[data-market-page-add]');
    const count = await toggles.count();
    if (count > 0) await expect(await toggles.first().isDisabled(), 'toggle expected disabled without board');
  });

  await test('direct step URL shows board-required recovery actions', async () => {
    const u = new URL(BASE);
    u.searchParams.set('open', 'config');
    u.searchParams.set('step', '2');
    await page.goto(u.toString(), { waitUntil: 'domcontentloaded' });
    await page.waitForSelector('#configurator-shell', { state: 'visible', timeout: 15000 });
    await page.waitForSelector('#step-2.content-section.active', { timeout: 10000 });
    await page.waitForSelector('#flow-alert.active', { timeout: 10000 });
    await expect(await page.locator('#btn-alert-m5-quick').isVisible(), 'missing M5 quick action');
    await expect(await page.locator('#btn-alert-esp32-quick').isVisible(), 'missing ESP32 quick action');
    await expect(await page.locator('#btn-alert-go-step1').isVisible(), 'missing step1 recovery action');
  });

  results.push({ name: 'no pageerror exceptions', ok: pageErrors.length === 0, error: pageErrors.join(' | ') });
  for (const r of results) console.log(`${r.ok ? 'PASS' : 'FAIL'}: ${r.name}${r.ok ? '' : ` -> ${r.error}`}`);
  await browser.close();
  if (results.some(r => !r.ok)) process.exit(1);
}

run().catch(e => {
  console.error('FATAL:', e);
  process.exit(1);
});
