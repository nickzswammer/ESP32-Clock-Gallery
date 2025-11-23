# ESP32 E-Paper Clock / Gallery Display

  | ![Fully Assembled Front Side](/docs/front.jpeg) | 
  |:--:| 
  | *Clock Frontside* |

A customizable ESP32-based image clock gallery project, combining Arduino firmware, Python scripting, electronics, and PCB design. This project allows users to upload images via a web interface, convert them to a binary format suitable for display on a clock-style device, and manage them on the ESP32.

---

## Features

  | ![Power Management Block Diagram](/docs/ESP32_PWR.png) | 
  |:--:| 
  | *Power Management Block Diagram* |

- **Embedded / Microcontroller:**
  - ESP32 main controller
  - Real-Time Clock (RTC) module for accurate timekeeping
  - Pushbutton controls for menu and navigation
  - SPI and I²C interfaces for peripheral communication

- **Python Backend:**
  - Flask web server for uploading images
  - Automated image conversion to `.bin` format with dithering
  - Image resizing and centering for consistent display
  - Batch download as ZIP archives for easy transfer to the ESP32

- **Electronics & PCB:**
  - 2-layer custom PCB design
  - Integrated power circuitry with MOSFET-based power muxing
  - Battery protection and charging circuitry
  - SPI and I²C routed to peripherals for modularity

  | ![Exploded Component View](/docs/exploded.jpeg) | 
  |:--:| 
  | *Internal Component View* |

---

## Image Processing Workflow

1. Users upload images via the Flask web interface.
2. Images are resized to fit a **400×300 display resolution**, converted to grayscale, and dithered to 1-bit black and white.
3. The processed images are converted into a `.bin` file containing:
   - 2-byte width
   - 2-byte height
   - Pixel data in row-major order
4. All processed `.bin` files can be downloaded in a ZIP archive for easy transfer to the ESP32.

---

## Folder Structure

```

project_root/
├── image_processor/      # Python backend
│   ├── uploaded_files/       # Raw uploaded images
│   ├── processed_files/      # Converted .bin images
│   ├── zipped_files/         # Zip archives of processed images
│   ├── templates/            # Flask HTML templates
│   ├── index.html
│   ├── static/               # Optional CSS/JS for Flask
│   ├── app.py                # Flask backend and image processing
│   └── download_zip.html
├── GxEPD2_display_selection_new_style.h # GxEPD2 Library E-Paper Display Select
├── GxEPD2_selection_check.h             
├── WebHelpers.cpp                       # Helper functions for the upload/ webserver processing
├── WebHelpers.h         
├── full_disp_AND_upload.ino             # Full Arduino file containing firmware for clock functionality
└── README.md 

````
---

## ESP32 Hardware and Firmware

### Hardware

* **ESP32 Microcontroller**: Handles display updates, button input, RTC communication, and Wi-Fi connectivity.
* **GxEPD2 E-Ink Display**: SPI-connected, supports partial/full updates for low power.
* **MicroSD Card**: Stores images for display; accessed via SPI.
* **DS3231 RTC**: Provides accurate time over I²C.
* **MAX17043 Fuel Gauge**: Monitors LiPo battery percentage.
* **Buttons**: Four GPIO-connected buttons for menu navigation and mode switching.
* **Power Management**: Supports light/deep sleep; wake on button or timer.

### Firmware

* **Initialization**: Sets up peripherals, Wi-Fi, and web server.
* **Main Loop**: Handles button input, updates clock display, manages sleep, and serves web requests.
* **Gallery Management**: Reads images from SD card, navigates files, and supports slideshow.
* **Power Optimization**: Disables unused peripherals and enters sleep during inactivity.

---

## PCB & Electronics

| ![PCB Layout View](/docs/PCB.png) | 
|:--:| 
| *PCB Layout View* |

- **2-layer PCB:**
  - Integrated power supply with custom MOSFET power muxing
  - Battery protection and charging
  - SPI and I²C routing for peripheral modules
  - ESP32 microcontroller footprint with all required signals exposed

- **Peripherals:**
  - RTC module
  - Pushbuttons for user input
  - Optional displays (LCD, OLED, or e-paper)

- **Circuit Features:**
  - Power management: allows USB and battery operation
  - Safe charging and overcurrent protection
  - Modular design for easy hardware expansion

---

## Case & Mechanical Design

- Custom enclosure design in Fusion 360 to fit the ESP32 PCB and display
- Mounting points for pushbuttons
- Openings for charging port, buttons, and display window
- 3d printed

---

## Author

**Nicholas Zhang**
