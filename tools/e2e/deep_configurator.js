const { chromium } = require('playwright');

const BASE = process.env.BASALT_UI_URL || 'http://127.0.0.1:5000';

async function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

async function waitReady(page) {
  await page.waitForFunction(() => {
    const sel = document.getElementById('platform-select');
    return !!sel && sel.options && sel.options.length > 1;
  }, { timeout: 20000 });
}

async function showConfig(page) {
  await waitReady(page);
  await page.click('#btn-open-config');
  await page.waitForFunction(() => {
    const el = document.getElementById('configurator-shell');
    return !!el && getComputedStyle(el).display !== 'none';
  }, { timeout: 15000 });
}

async function pickBoard(page, matcher) {
  const items = page.locator('#board-list .board-item');
  const c = await items.count();
  await assert(c > 0, 'no board items');
  for (let i = 0; i < c; i++) {
    const t = (await items.nth(i).textContent()) || '';
    if (matcher.test(t)) {
      await items.nth(i).click();
      await page.waitForFunction(() => {
        const el = document.getElementById('board-details');
        return !!el && getComputedStyle(el).display !== 'none';
      }, { timeout: 10000 });
      return t;
    }
  }
  throw new Error(`board not found: ${matcher}`);
}

async function run() {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1440, height: 900 } });
  const errs = [];
  page.on('pageerror', e => errs.push(String(e?.message || e)));

  const out = [];
  async function t(name, fn) {
    try {
      await page.goto(BASE, { waitUntil: 'domcontentloaded' });
      await fn();
      out.push({ name, ok: true });
    } catch (e) {
      out.push({ name, ok: false, error: String(e?.message || e) });
    }
  }

  await t('template relevance shows Best Match on CYD', async () => {
    await showConfig(page);
    await page.selectOption('#manufacturer-select', { label: 'Espressif' });
    await pickBoard(page, /CYD/i);
    const txt = (await page.locator('#templates-grid').textContent()) || '';
    await assert(txt.includes('Best Match:'), 'best match hint missing');
  });

  await t('step 2 driver list populates', async () => {
    await showConfig(page);
    await page.selectOption('#manufacturer-select', { label: 'Espressif' });
    await pickBoard(page, /CYD/i);
    await page.click('#btn-next');
    await page.waitForSelector('#step-2.content-section.active', { timeout: 10000 });
    await assert((await page.locator('#module-grid .module-card').count()) > 0, 'no module cards');
    await assert((await page.locator('#module-grid .module-section').count()) > 0, 'no module sections');
  });

  await t('generate preview includes board/platform defines', async () => {
    await showConfig(page);
    await page.selectOption('#manufacturer-select', { label: 'Espressif' });
    await pickBoard(page, /CYD/i);
    await page.click('#btn-next');
    await page.waitForSelector('#step-2.content-section.active', { timeout: 10000 });
    await page.click('#btn-next');
    await page.waitForSelector('#step-3.content-section.active', { timeout: 10000 });
    await page.click('#btn-next');
    await page.waitForSelector('#step-4.content-section.active', { timeout: 10000 });
    await page.click('#btn-generate');
    await page.waitForFunction(() => {
      const t = document.getElementById('config-preview')?.textContent || '';
      return t.includes('BASALT_PLATFORM_') || t.includes('Preview failed');
    }, { timeout: 20000 });
    const txt = (await page.locator('#config-preview').textContent()) || '';
    await assert(/BASALT_PLATFORM_/i.test(txt), 'missing platform define');
    await assert(/BASALT_BOARD_/i.test(txt), 'missing board define');
  });

  out.push({ name: 'no pageerror exceptions', ok: errs.length === 0, error: errs.join(' | ') });
  for (const r of out) console.log(`${r.ok ? 'PASS' : 'FAIL'}: ${r.name}${r.ok ? '' : ` -> ${r.error}`}`);
  await browser.close();
  if (out.some(r => !r.ok)) process.exit(1);
}

run().catch(e => {
  console.error('FATAL:', e);
  process.exit(1);
});
