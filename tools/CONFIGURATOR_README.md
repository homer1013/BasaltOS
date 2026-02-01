# BasaltOS Configuration GUI

A web-based graphical interface for configuring BasaltOS projects. Similar to STM32CubeMX or MPLAB X, this tool helps you select boards, enable modules, configure pins, and generate configuration files without manually editing code.

## Features

- 🎯 **Visual Board Selection** - Browse and select from supported development boards
- 🧩 **Module Management** - Enable/disable features like TFT, SD card, WiFi, etc.
- 📌 **Pin Configuration** - Assign GPIO pins with conflict detection
- 📝 **Code Generation** - Automatically generate `basalt_config.h` and `sdkconfig.defaults`
- 🔍 **Configuration Preview** - See generated configuration before saving
- 💾 **Save/Load Projects** - Save your configuration for reuse

## Architecture

The GUI consists of two parts:

1. **Frontend** (`basaltos_config_gui.html`) - Interactive web interface
2. **Backend** (`basaltos_config_server.py`) - Flask server that reads board/module JSON files

## Quick Start

### Option 1: Standalone HTML (Demo Mode)

For a quick demo without the backend:

```bash
# Simply open the HTML file in your browser
open basaltos_config_gui.html
```

This works standalone but uses hardcoded example data.

### Option 2: Full Setup (Recommended)

For full integration with your BasaltOS project:

1. **Install dependencies:**
```bash
pip install flask flask-cors
```

2. **Organize your BasaltOS structure:**
```
BasaltOS/
├── boards/           # Board JSON files
├── modules/          # Module JSON files  
├── platforms/        # Platform JSON files
├── config/
│   └── generated/    # Output directory
└── tools/
    ├── basaltos_config_gui.html
    └── basaltos_config_server.py
```

3. **Start the server:**
```bash
cd BasaltOS/tools
python basaltos_config_server.py
```

4. **Open the GUI:**
```
http://localhost:5000
```

## Configuration File Structure

### Board JSON (`boards/cyd_3248s035r.json`)

```json
{
  "name": "CYD ESP32-3248S035R",
  "description": "3.5 inch TFT display with ESP32",
  "platform": "esp32",
  "mcu": "ESP32-WROOM-32",
  "flash": "4MB",
  "ram": "520KB",
  "display": "320x480 ST7796",
  "pins": {
    "tft_mosi": 13,
    "tft_sclk": 14,
    "tft_cs": 15,
    "tft_dc": 2,
    "led": 16
  },
  "pin_definitions": {
    "tft_mosi": {
      "description": "TFT SPI MOSI",
      "alternatives": [13, 23],
      "type": "output"
    }
  },
  "sdkconfig": [
    "CONFIG_ESP32_DEFAULT_CPU_FREQ_240=y",
    "CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y"
  ]
}
```

### Module JSON (`modules/tft.json`)

```json
{
  "name": "TFT Display",
  "description": "Color TFT display driver",
  "platforms": ["esp32", "stm32", "rp2040"],
  "default_enabled": false,
  "dependencies": ["spi"],
  "pins": [
    "tft_mosi",
    "tft_sclk",
    "tft_cs",
    "tft_dc"
  ],
  "configuration_options": [
    {
      "id": "tft_driver",
      "name": "Display Driver",
      "type": "select",
      "options": ["ST7796", "ST7789", "ILI9341"],
      "default": "ST7796"
    }
  ],
  "sdkconfig": [
    "CONFIG_SPI_MASTER_IN_IRAM=y"
  ]
}
```

## Workflow

### Step 1: Select Platform & Board

1. Choose platform (ESP32, STM32, AVR, etc.)
2. Browse available boards for that platform
3. View board specifications and capabilities

### Step 2: Enable Modules

1. Toggle modules on/off based on your needs
2. See which pins each module requires
3. View module dependencies automatically

### Step 3: Configure Pins

1. Review pin assignments for enabled modules
2. Modify pins if needed (conflict checking)
3. See alternative pin options

### Step 4: Generate Configuration

1. Preview generated configuration
2. Download files:
   - `basalt_config.h` - C header with defines
   - `sdkconfig.defaults` - ESP-IDF configuration
   - `basalt_config.json` - Full project config

## API Endpoints

The backend provides a REST API:

- `GET /api/platforms` - List available platforms
- `GET /api/boards/<platform>` - Get boards for platform
- `GET /api/modules?platform=<platform>` - Get available modules
- `GET /api/board/<board_id>` - Get detailed board info
- `POST /api/generate` - Generate configuration files
- `POST /api/preview/<config_type>` - Preview config without saving

## Example API Usage

```javascript
// Get boards for ESP32
fetch('http://localhost:5000/api/boards/esp32')
  .then(r => r.json())
  .then(boards => console.log(boards));

// Generate configuration
fetch('http://localhost:5000/api/generate', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    platform: 'esp32',
    board_id: 'cyd_3248s035r',
    enabled_modules: ['uart', 'tft', 'fs_spiffs'],
    custom_pins: {
      led: 16
    }
  })
})
.then(r => r.json())
.then(result => console.log(result));
```

## Generated Files

### basalt_config.h

```c
/* BasaltOS Generated Configuration */
#pragma once

/* Board: CYD ESP32-3248S035R */
#define BASALT_BOARD_CYD_3248S035R

/* Feature Gates */
#define BASALT_ENABLE_UART 1
#define BASALT_ENABLE_TFT 1
#define BASALT_ENABLE_FS_SPIFFS 1
#define BASALT_ENABLE_WIFI 0

/* Pin Assignments */
#define BASALT_PIN_TFT_MOSI 13
#define BASALT_PIN_TFT_SCLK 14
#define BASALT_PIN_TFT_CS 15
#define BASALT_PIN_LED 16
```

### sdkconfig.defaults

```ini
# BasaltOS Generated SDK Config
CONFIG_ESP32_DEFAULT_CPU_FREQ_240=y
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_ESP_CONSOLE_UART_BAUDRATE=115200
CONFIG_SPI_MASTER_IN_IRAM=y
```

## Extending the System

### Adding a New Board

1. Create `boards/my_board.json`
2. Define pins, capabilities, platform
3. Restart server to load new board

### Adding a New Module

1. Create `modules/my_module.json`
2. Define pins, dependencies, options
3. Restart server to load new module

### Adding Platform Support

1. Create `platforms/my_platform.json`
2. Define toolchain, SDK config defaults
3. Update board configs to reference platform

## Design Philosophy

This GUI follows the principle of **"Configure Once, Build Anywhere"**:

1. **Declarative Configuration** - Board/module capabilities defined in JSON
2. **Separation of Concerns** - Hardware config separate from application code
3. **No Hardcoding** - `app_main.c` uses generated defines only
4. **Scalable** - Works from ATtiny85 to ESP32

## Future Enhancements

- [ ] Pin conflict detection and highlighting
- [ ] Visual pin diagram/pinout viewer
- [ ] Module dependency graph visualization
- [ ] Import/export configurations
- [ ] Multi-board projects (e.g., master/slave setups)
- [ ] Real-time validation
- [ ] Code completion hints
- [ ] Build integration (trigger idf.py from GUI)

## Troubleshooting

**Server won't start:**
- Check that Flask is installed: `pip install flask flask-cors`
- Verify paths in `BASALTOS_ROOT` are correct

**Boards not showing:**
- Ensure `boards/` directory exists
- Check JSON syntax in board files
- Look for errors in server console

**Configuration not generating:**
- Check that `config/generated/` directory is writable
- Verify all required fields in POST request

## License

Part of the BasaltOS project. See main repository for license details.

## Contributing

Contributions welcome! Areas that need work:
- Additional board definitions
- New module types
- UI/UX improvements
- Testing on different platforms
