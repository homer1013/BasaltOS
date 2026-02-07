print("[flappy_bird] start")

# BasaltOS App Market app
# Inspired by the glide/flap mini-game logic from /home/homer/Qubi_ESP_IDF/main/qubi_app.cpp

try:
    import basalt
except Exception as e:
    basalt = None
    print("flappy_bird: basalt import failed:", e)

if not basalt:
    print("flappy_bird: basalt module unavailable")
    print("[flappy_bird] done")
    raise SystemExit

try:
    import time
except Exception:
    time = None

try:
    import random
except Exception:
    random = None


def sleep_ms(ms):
    if hasattr(basalt, "timer"):
        basalt.timer.sleep_ms(int(ms))
        return
    if time and hasattr(time, "sleep_ms"):
        time.sleep_ms(int(ms))


def ticks_ms():
    if time and hasattr(time, "ticks_ms"):
        return int(time.ticks_ms())
    return 0


def rand_int(a, b):
    if random and hasattr(random, "randint"):
        return int(random.randint(a, b))
    span = int(b - a + 1)
    if span <= 0:
        return int(a)
    return int(a + (ticks_ms() % span))

def max2(a, b):
    return a if a >= b else b

FB_STAGE = "boot"
BUTTON_PIN = 3
BUTTON_ACTIVE_LOW = 1

def set_stage(name):
    global FB_STAGE
    FB_STAGE = name


class UiBackend:
    def __init__(self):
        self.kind = "none"
        self.dev = None
        self.w = 0
        self.h = 0
        self.button_ready = 0

        if hasattr(basalt, "ui") and hasattr(basalt.ui, "ready") and basalt.ui.ready():
            self.kind = "tft"
            self.dev = basalt.ui
            self.w = 320
            self.h = 480
            return

        if hasattr(basalt, "ssd1306"):
            d = basalt.ssd1306
            if hasattr(d, "ready") and d.ready():
                self.kind = "ssd1306"
                self.dev = d
                self.w = int(d.width()) if hasattr(d, "width") else 128
                self.h = int(d.height()) if hasattr(d, "height") else 64

        # Optional physical button (works for OLED/non-touch boards too).
        if hasattr(basalt, "gpio"):
            try:
                basalt.gpio.mode(int(BUTTON_PIN), 0)
                self.button_ready = 1
            except Exception:
                self.button_ready = 0

    def ready(self):
        return self.kind != "none"

    def clear(self):
        if self.kind == "tft":
            self.dev.clear()
        elif self.kind == "ssd1306":
            self.dev.clear(0)
            self.dev.show()

    def text_at(self, x, y, msg, color=1):
        if self.kind == "tft":
            self.dev.text_at(int(x), int(y), msg)
        elif self.kind == "ssd1306":
            self.dev.text_at(int(x), int(y), msg, int(1 if color else 0))

    def rect(self, x, y, w, h, color=1, fill=False):
        if self.kind == "tft":
            self.dev.rect(int(x), int(y), int(w), int(h), int(color), 1 if fill else 0)
        elif self.kind == "ssd1306":
            self.dev.rect(int(x), int(y), int(w), int(h), int(1 if color else 0), 1 if fill else 0)

    def circle(self, x, y, r, color=1, fill=False):
        if self.kind == "tft":
            self.dev.circle(int(x), int(y), int(r), int(color), 1 if fill else 0)
        elif self.kind == "ssd1306":
            self.dev.circle(int(x), int(y), int(r), int(1 if color else 0), 1 if fill else 0)

    def line(self, x0, y0, x1, y1, color=1):
        if self.kind == "tft":
            self.dev.line(int(x0), int(y0), int(x1), int(y1), int(color))
        elif self.kind == "ssd1306":
            self.dev.line(int(x0), int(y0), int(x1), int(y1), int(1 if color else 0))

    def present(self):
        if self.kind == "ssd1306":
            self.dev.show()

    def pressed(self):
        # Touch only for TFT path today.
        if self.kind == "tft" and hasattr(self.dev, "touch"):
            t = self.dev.touch()
            if t and int(t[0]):
                return 1
        if self.button_ready and hasattr(basalt, "gpio"):
            try:
                v = int(basalt.gpio.get(int(BUTTON_PIN)))
                if BUTTON_ACTIVE_LOW:
                    return 1 if v == 0 else 0
                return 1 if v != 0 else 0
            except Exception:
                return 0
        return 0


ui = UiBackend()
if not ui.ready():
    print("flappy_bird: no compatible display API available (need basalt.ui or basalt.ssd1306)")
    print("[flappy_bird] done")
    raise SystemExit

print("flappy_bird: display backend =", ui.kind)

if ui.kind == "tft":
    X0 = 10
    Y0 = 32
    W = 220
    H = 180
    FLOOR_H = 14
    BIRD_R = 5
    PIPE_W = 18
    GAP_H = 54
    PIPE_SPEED = 3
    GRAVITY = 1
    FLAP_VY = -7
else:
    # SSD1306 128x64 (or similar) compact rules.
    X0 = 0
    Y0 = 0
    W = max2(64, ui.w)
    H = max2(32, ui.h)
    FLOOR_H = 8
    BIRD_R = 2
    PIPE_W = 8
    GAP_H = 18 if H >= 64 else 14
    PIPE_SPEED = 2
    GRAVITY = 1
    FLAP_VY = -3

BIRD_X = X0 + (W // 4)


def draw_text(y, msg):
    ui.text_at(X0 + 1, int(y), msg, 1)


def wait_replay(ms_timeout):
    start = ticks_ms()
    while True:
        if ui.pressed():
            return True
        if ms_timeout > 0 and ticks_ms() - start >= ms_timeout:
            return False
        sleep_ms(30)


def run_round():
    set_stage("init")
    bird_y = Y0 + H // 2
    bird_vy = 0
    score = 0

    pipe_x = X0 + W + 4
    gap_center = Y0 + 8 + rand_int(0, max2(1, H - FLOOR_H - 16))
    passed = False
    game_over = False

    auto_mode = (ui.kind == "ssd1306" and not ui.button_ready)
    if auto_mode:
        print("flappy_bird: oled mode uses auto-flap demo (no touch input)")
        auto_next_flap = ticks_ms() + rand_int(180, 320)
    elif ui.button_ready:
        print("flappy_bird: button mode on GPIO" + str(int(BUTTON_PIN)))

    def draw_static_scene():
        ui.rect(X0, Y0, W, H, 1, False)
        floor_y = Y0 + H - FLOOR_H
        ui.rect(X0 + 1, floor_y, max2(1, W - 2), max2(1, FLOOR_H - 1), 1, True)

    def draw_pipe(px, gt, gb, color):
        top_h = max2(0, gt - Y0)
        bot_y = gb
        bot_h = max2(0, (Y0 + H - FLOOR_H) - bot_y)
        if top_h > 0:
            ui.rect(px, Y0, PIPE_W, top_h, color, True)
        if bot_h > 0:
            ui.rect(px, bot_y, PIPE_W, bot_h, color, True)

    def draw_bird(by, color):
        ui.circle(BIRD_X, int(by), BIRD_R, color, True)

    set_stage("scene_init")
    ui.clear()
    draw_static_scene()
    if ui.kind == "tft":
        draw_text(Y0 + 2, "Flappy Bird")
        draw_text(Y0 + 12, "score")
    else:
        draw_text(0, "FB")
    ui.present()

    prev_bird_y = bird_y
    prev_pipe_x = pipe_x
    prev_gap_center = gap_center

    while not game_over:
        set_stage("loop")
        if auto_mode:
            set_stage("autoflap")
            now = ticks_ms()
            # Simple heuristic autopilot near gap center.
            if now >= auto_next_flap:
                if bird_y > gap_center + 1:
                    bird_vy = FLAP_VY
                auto_next_flap = now + rand_int(120, 260)
        else:
            set_stage("touch")
            if ui.pressed():
                bird_vy = FLAP_VY

        set_stage("physics")
        bird_vy += GRAVITY
        bird_y += bird_vy

        set_stage("pipe")
        pipe_x -= PIPE_SPEED
        if pipe_x + PIPE_W < X0:
            pipe_x = X0 + W + rand_int(8, 28)
            gap_center = Y0 + 8 + rand_int(0, max2(1, H - FLOOR_H - 16))
            passed = False

        gap_top = gap_center - GAP_H // 2
        gap_bottom = gap_center + GAP_H // 2
        prev_gap_top = prev_gap_center - GAP_H // 2
        prev_gap_bottom = prev_gap_center + GAP_H // 2

        set_stage("collisions")
        if (not passed) and (pipe_x + PIPE_W < BIRD_X):
            passed = True
            score += 1

        if bird_y - BIRD_R <= Y0:
            game_over = True
        if bird_y + BIRD_R >= Y0 + H - FLOOR_H:
            game_over = True

        in_pipe_x = (BIRD_X + BIRD_R >= pipe_x) and (BIRD_X - BIRD_R <= pipe_x + PIPE_W)
        if in_pipe_x and ((bird_y - BIRD_R <= gap_top) or (bird_y + BIRD_R >= gap_bottom)):
            game_over = True

        set_stage("draw")
        # Erase old dynamic objects only.
        draw_pipe(prev_pipe_x, prev_gap_top, prev_gap_bottom, 0)
        draw_bird(prev_bird_y, 0)

        # Restore static geometry touched by erase operations.
        draw_static_scene()
        if ui.kind == "tft":
            draw_text(Y0 + 2, "Flappy Bird")
            draw_text(Y0 + 12, "score")
        else:
            draw_text(0, "FB")

        # Draw current dynamic objects.
        draw_pipe(pipe_x, gap_top, gap_bottom, 1)
        draw_bird(bird_y, 1)

        set_stage("present")
        ui.present()
        sleep_ms(20 if ui.kind == "tft" else 6)

        prev_bird_y = bird_y
        prev_pipe_x = pipe_x
        prev_gap_center = gap_center

    set_stage("game_over")
    ui.text_at(X0 + 6, Y0 + H // 2 - 6, "GAME OVER", 1)
    ui.text_at(X0 + 6, Y0 + H // 2 + 4, "score", 1)
    ui.present()
    set_stage("wait_replay")
    return wait_replay(5000 if ui.kind == "tft" else 3000)


try:
    ui.clear()
    if ui.kind == "tft":
        draw_text(Y0 - 24, "Basalt App Market")
        draw_text(Y0 - 14, "Qubi-inspired Flappy")
        ui.present()
        sleep_ms(350)
    else:
        ui.text_at(0, 0, "Basalt Flappy", 1)
        ui.text_at(0, 10, "Qubi-inspired", 1)
        ui.text_at(0, 20, "OLED demo mode", 1)
        ui.present()
        sleep_ms(550)

    keep_playing = True
    rounds = 0
    max_rounds = 4 if ui.kind == "ssd1306" else 999999
    while keep_playing and rounds < max_rounds:
        rounds += 1
        keep_playing = run_round()

except Exception as e:
    print("flappy_bird error:", e, "stage=", FB_STAGE)

print("[flappy_bird] done")
