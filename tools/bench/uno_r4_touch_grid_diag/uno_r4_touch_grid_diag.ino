#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include "TouchScreen_kbv.h"
#define TouchScreen TouchScreen_kbv
#define TSPoint TSPoint_kbv

MCUFRIEND_kbv tft;

const int XP = 6, XM = A2, YP = A1, YM = 7;
TouchScreen ts(XP, YP, XM, YM, 300);

const int MINZ = 120;
const int MAXZ = 1000;
const int GRID_COLS = 4;
const int GRID_ROWS = 4;
const int NPTS = GRID_COLS * GRID_ROWS;

int tx[NPTS], ty[NPTS];
int rx[NPTS], ry[NPTS], rz[NPTS];

void restorePins() {
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);
}

TSPoint readPoint() {
  TSPoint p = ts.getPoint();
  restorePins();
  return p;
}

int med5(int a, int b, int c, int d, int e) {
  int v[5] = {a, b, c, d, e};
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (v[j] < v[i]) { int t = v[i]; v[i] = v[j]; v[j] = t; }
    }
  }
  return v[2];
}

bool captureStableRaw(int *outx, int *outy, int *outz) {
  int xs[5], ys[5], zs[5];
  int got = 0;
  unsigned long start = millis();
  while ((millis() - start) < 4000) {
    TSPoint p = readPoint();
    if (p.z > MINZ && p.z < MAXZ) {
      xs[got] = p.x;
      ys[got] = p.y;
      zs[got] = p.z;
      got++;
      if (got >= 5) {
        *outx = med5(xs[0], xs[1], xs[2], xs[3], xs[4]);
        *outy = med5(ys[0], ys[1], ys[2], ys[3], ys[4]);
        *outz = med5(zs[0], zs[1], zs[2], zs[3], zs[4]);
        return true;
      }
      delay(20);
    } else {
      got = 0;
      delay(10);
    }
  }
  return false;
}

bool solve3x3(float a[3][3], float b[3], float out[3]) {
  for (int i = 0; i < 3; i++) {
    int piv = i;
    float best = (a[i][i] < 0) ? -a[i][i] : a[i][i];
    for (int r = i + 1; r < 3; r++) {
      float v = (a[r][i] < 0) ? -a[r][i] : a[r][i];
      if (v > best) { best = v; piv = r; }
    }
    if (best < 1e-6f) return false;
    if (piv != i) {
      for (int c = i; c < 3; c++) { float t = a[i][c]; a[i][c] = a[piv][c]; a[piv][c] = t; }
      float tb = b[i]; b[i] = b[piv]; b[piv] = tb;
    }
    float div = a[i][i];
    for (int c = i; c < 3; c++) a[i][c] /= div;
    b[i] /= div;
    for (int r = 0; r < 3; r++) {
      if (r == i) continue;
      float f = a[r][i];
      for (int c = i; c < 3; c++) a[r][c] -= f * a[i][c];
      b[r] -= f * b[i];
    }
  }
  out[0] = b[0]; out[1] = b[1]; out[2] = b[2];
  return true;
}

void reportAffineError() {
  float sxx=0, syy=0, sxy=0, sx1=0, sy1=0;
  float bx0=0, bx1=0, bx2=0, by0=0, by1=0, by2=0;
  for (int i = 0; i < NPTS; i++) {
    float x = (float)rx[i], y = (float)ry[i];
    sxx += x*x; syy += y*y; sxy += x*y; sx1 += x; sy1 += y;
    bx0 += x*tx[i]; bx1 += y*tx[i]; bx2 += tx[i];
    by0 += x*ty[i]; by1 += y*ty[i]; by2 += ty[i];
  }
  float m1[3][3] = {{sxx,sxy,sx1},{sxy,syy,sy1},{sx1,sy1,(float)NPTS}};
  float m2[3][3] = {{sxx,sxy,sx1},{sxy,syy,sy1},{sx1,sy1,(float)NPTS}};
  float bx[3] = {bx0,bx1,bx2};
  float by[3] = {by0,by1,by2};
  float ox[3], oy[3];
  if (!solve3x3(m1,bx,ox) || !solve3x3(m2,by,oy)) {
    Serial.println("affine-fit: failed");
    return;
  }
  Serial.print("affine ax="); Serial.print(ox[0],6);
  Serial.print(" bx="); Serial.print(ox[1],6);
  Serial.print(" cx="); Serial.print(ox[2],3);
  Serial.print(" ay="); Serial.print(oy[0],6);
  Serial.print(" by="); Serial.print(oy[1],6);
  Serial.print(" cy="); Serial.println(oy[2],3);

  float rms = 0.0f;
  float maxe = 0.0f;
  int maxi = -1;
  for (int i = 0; i < NPTS; i++) {
    float px = ox[0]*rx[i] + ox[1]*ry[i] + ox[2];
    float py = oy[0]*rx[i] + oy[1]*ry[i] + oy[2];
    float ex = px - tx[i];
    float ey = py - ty[i];
    float e = sqrtf(ex*ex + ey*ey);
    rms += e*e;
    if (e > maxe) { maxe = e; maxi = i; }
    Serial.print("pt "); Serial.print(i);
    Serial.print(" target("); Serial.print(tx[i]); Serial.print(","); Serial.print(ty[i]); Serial.print(")");
    Serial.print(" raw("); Serial.print(rx[i]); Serial.print(","); Serial.print(ry[i]); Serial.print(")");
    Serial.print(" err="); Serial.println(e,2);
  }
  rms = sqrtf(rms / NPTS);
  Serial.print("affine-rms-px="); Serial.println(rms,2);
  Serial.print("affine-max-px="); Serial.print(maxe,2);
  Serial.print(" at pt="); Serial.println(maxi);
}

void drawTarget(int x, int y, uint16_t color) {
  tft.drawLine(x - 10, y, x + 10, y, color);
  tft.drawLine(x, y - 10, x, y + 10, color);
  tft.drawCircle(x, y, 13, color);
}

void setup() {
  Serial.begin(115200);
  uint16_t id = tft.readID();
  if (id == 0xD3D3 || id == 0xFFFF || id == 0x0000 || id == 0x0101) id = 0x9486;
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(2);
  tft.setCursor(4, 4);
  tft.print("GRID DIAG 4x4");
  Serial.println("grid-diag:start");

  const int x0 = 20, x1 = tft.width() - 20;
  const int y0 = 70, y1 = tft.height() - 20;

  int idx = 0;
  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      tx[idx] = x0 + (x1 - x0) * c / (GRID_COLS - 1);
      ty[idx] = y0 + (y1 - y0) * r / (GRID_ROWS - 1);
      idx++;
    }
  }

  for (int i = 0; i < NPTS; i++) {
    tft.fillRect(0, 28, tft.width(), 34, 0x0000);
    tft.setCursor(4, 34);
    tft.setTextColor(0xFFE0, 0x0000);
    tft.print("Touch point "); tft.print(i + 1); tft.print("/"); tft.print(NPTS);
    drawTarget(tx[i], ty[i], 0xF800);

    int x=0,y=0,z=0;
    bool ok = captureStableRaw(&x,&y,&z);
    if (!ok) {
      Serial.print("pt "); Serial.print(i); Serial.println(" capture-timeout");
      rx[i] = ry[i] = rz[i] = -1;
      continue;
    }
    rx[i] = x; ry[i] = y; rz[i] = z;
    drawTarget(tx[i], ty[i], 0x07E0);
    Serial.print("pt "); Serial.print(i);
    Serial.print(" target("); Serial.print(tx[i]); Serial.print(","); Serial.print(ty[i]); Serial.print(")");
    Serial.print(" raw("); Serial.print(rx[i]); Serial.print(","); Serial.print(ry[i]); Serial.print(")");
    Serial.print(" z="); Serial.println(rz[i]);
    delay(200);
  }

  reportAffineError();
  Serial.println("grid-diag:done");

  tft.fillRect(0, 28, tft.width(), 34, 0x0000);
  tft.setCursor(4, 34);
  tft.setTextColor(0x07E0, 0x0000);
  tft.print("Done. See serial.");
}

void loop() {}
