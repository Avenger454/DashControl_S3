# DASH CONTROL — first hardware preview

Target: Waveshare ESP32-S3-Touch-LCD-2.1, 480 × 480 round display, LVGL 8.

This branch replaces the AFR/boost receiver UI with native LVGL control menus:
Home, Gauge Select, gauge-page previews, Transmission Mode preview (1–8),
Display (10–100% local backlight and 20% night setting), and Settings.

This is the first functional menu pass, not the final artwork match.
It uses text headings, blue button outlines, and red transmission accents.
No silver bezel or per-frame image scaling is used.
Gauge page names are provisional local previews; communication is not implemented.
No CAN, UART, or transmission commands are sent.
Mode selection is explicitly labeled OFFLINE / PREVIEW.
All four gauge-page previews start at their first/default page every boot.
Settings resets only these local previews; it cannot yet reset remote gauges.

The existing LCD and touch drivers remain in place. Panel and LVGL resolution
are 480 × 480; touch x_max/y_max use the same 480 definitions.
Waveshare lists CST820 for this board, whereas the imported driver uses GT911.
Confirm working touch on the physical board; that discrepancy is not resolved here.

LVGL object initialization now completes before its handler task starts.
Handler task delay is bounded to 20 ms and at least one RTOS tick.
Existing partial DMA draw buffers are retained, with allocation assertions.

Validation needed in ESP-IDF and on hardware:
- Build and flash the feature/dash-control-ui branch.
- Check Home text fit, all four touch targets, edge clearance, and Back navigation.
- Visit each gauge and cycle page previews, then reset defaults.
- Check mode arrows stop at 1 and 8; no command is sent.
- Check local brightness, night mode, and the corresponding slider value.
- Confirm frame rate, CPU usage, touch alignment, and stability after repeated navigation.

No ESP-IDF build or hardware test was available in the assistant environment.
