#include "../ui.h"
#include "ST7701S.h"
#include <stdint.h>
#include <stdio.h>

LV_FONT_DECLARE(Schoolbell_24);
LV_FONT_DECLARE(Doto_Extra_Bold_84);
lv_obj_t *ui_Screen1;
static lv_obj_t *content, *mode_value, *mode_caption, *brightness_value;
static unsigned mode = 1, selected_gauge, selected_page[4];
static unsigned brightness = 75, day_brightness = 75;
static bool night;
static const char *gauges[] = {"SPEEDO", "TACH", "INFO1", "INFO2"};
/* Local previews only. No remote protocol or command IDs are assumed. */
static const char *pages[4][3] = {
    {"ROAD", "VAC / BOOST", "TRIP"},
    {"ROAD", "ENGINE", "SHIFT"},
    {"CRITICAL", "ENGINE", "TRANSMISSION"},
    {"FUEL / VOLTS", "FUEL", "ELECTRICAL"}
};
enum { HOME, GAUGES, TRANSMISSION, DISPLAY, SETTINGS, GAUGE_PAGE,
       GAUGE_0 = 10, MODE_PREV = 20, MODE_NEXT, PAGE_PREV, PAGE_NEXT, DEFAULTS };
static void show(unsigned page);

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                       int width, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, text);
    lv_label_set_long_mode(o, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, width);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static void action(lv_event_t *e)
{
    unsigned id = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if(id >= GAUGE_0 && id < GAUGE_0 + 4) {
        selected_gauge = id - GAUGE_0;
        show(GAUGE_PAGE);
    } else if(id == MODE_PREV || id == MODE_NEXT) {
        if(id == MODE_PREV) { if(mode > 1) --mode; }
        else if(mode < 8) ++mode;
        lv_label_set_text_fmt(mode_value, "%u", mode);
        lv_label_set_text_fmt(mode_caption, "MODE %u", mode);
    } else if(id == PAGE_PREV || id == PAGE_NEXT) {
        unsigned *p = &selected_page[selected_gauge];
        *p = id == PAGE_NEXT ? (*p + 1) % 3 : (*p + 2) % 3;
        show(GAUGE_PAGE);
    } else if(id == DEFAULTS) {
        for(unsigned i = 0; i < 4; ++i) selected_page[i] = 0;
        show(GAUGES);
    } else show(id);
}

static lv_obj_t *button(const char *text, int x, int y, int w, int h,
                        unsigned id, bool red)
{
    lv_obj_t *o = lv_btn_create(content);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x030B16), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 20, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(red ? 0xFF2020 : 0x00BAFF), 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x14344B), LV_STATE_PRESSED);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(o, action, LV_EVENT_CLICKED, (void *)(uintptr_t)id);
    lv_obj_t *t = label(o, text, 4, 0, w - 8, &Schoolbell_24, 0xFFFFFF);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);
    return o;
}

static void divider(int y)
{
    lv_obj_t *o = lv_obj_create(content);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, 76, y);
    lv_obj_set_size(o, 328, 3);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

static void apply_brightness(void)
{
    Set_Backlight(brightness);
    if(brightness_value)
        lv_label_set_text_fmt(brightness_value, "%u%%", brightness);
}

static void brightness_changed(lv_event_t *e)
{
    brightness = (unsigned)lv_slider_get_value(lv_event_get_target(e));
    if(!night) day_brightness = brightness;
    apply_brightness();
}

static void night_changed(lv_event_t *e)
{
    night = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    brightness = night ? 20 : day_brightness;
    apply_brightness();
    /* Keep the slider consistent without rebuilding an active switch event. */
    lv_obj_t *slider = lv_event_get_user_data(e);
    lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
}

static void show(unsigned page)
{
    mode_value = mode_caption = brightness_value = NULL;
    lv_obj_clean(content);
    const char *titles[] = {"DASH CONTROL", "GAUGE SELECT", "TRANSMISSION MODE",
                            "DISPLAY", "SETTINGS", "GAUGE PAGE"};
    label(content, "FIREBIRD", 130, 45, 220, &Schoolbell_24, 0xFF2020);
    label(content, titles[page], 42, 97, 396, &Schoolbell_24, 0xFFFFFF);
    divider(140);
    if(page == HOME || page == GAUGES) {
        if(page == HOME) {
            button("GAUGES", 62, 164, 172, 96, GAUGES, false);
            button("TRANSMISSION", 246, 164, 172, 96, TRANSMISSION, true);
            button("DISPLAY", 62, 274, 172, 96, DISPLAY, false);
            button("SETTINGS", 246, 274, 172, 96, SETTINGS, false);
            label(content, "REMOTE DISPLAYS OFFLINE", 80, 389, 320,
                  LV_FONT_DEFAULT, 0x80B6D0);
        } else {
            for(unsigned i = 0; i < 4; ++i)
                button(gauges[i], 62 + (i % 2) * 184, 164 + (i / 2) * 106,
                       172, 94, GAUGE_0 + i, false);
        }
    } else if(page == TRANSMISSION) {
        label(content, "OFFLINE - PREVIEW ONLY", 70, 155, 340,
              LV_FONT_DEFAULT, 0x80B6D0);
        button("<", 54, 213, 76, 100, MODE_PREV, false);
        button(">", 350, 213, 76, 100, MODE_NEXT, false);
        lv_obj_t *box = button("", 148, 195, 184, 133, TRANSMISSION, true);
        /* The centre is a display, never a transmission command button. */
        lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);
        mode_value = label(box, "", 0, 17, 184, &Doto_Extra_Bold_84, 0xFFFFFF);
        lv_label_set_text_fmt(mode_value, "%u", mode);
        mode_caption = label(content, "", 130, 344, 220, &Schoolbell_24, 0xFFFFFF);
        lv_label_set_text_fmt(mode_caption, "MODE %u", mode);
    } else if(page == DISPLAY) {
        label(content, "SCREEN BRIGHTNESS", 70, 160, 340,
              &Schoolbell_24, 0xFFFFFF);
        lv_obj_t *slider = lv_slider_create(content);
        lv_obj_set_pos(slider, 88, 216);
        lv_obj_set_size(slider, 304, 24);
        lv_slider_set_range(slider, 10, 100);
        lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(slider, lv_color_hex(0x00BFFF), LV_PART_INDICATOR);
        lv_obj_add_event_cb(slider, brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);
        brightness_value = label(content, "", 150, 259, 180, &Schoolbell_24, 0xFFFFFF);
        apply_brightness();
        label(content, "NIGHT MODE", 80, 324, 220, &Schoolbell_24, 0xFFFFFF);
        lv_obj_t *sw = lv_switch_create(content);
        lv_obj_set_pos(sw, 322, 322);
        lv_obj_set_size(sw, 64, 34);
        if(night) lv_obj_add_state(sw, LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, night_changed, LV_EVENT_VALUE_CHANGED, slider);
    } else if(page == SETTINGS) {
        label(content, "Fixed startup pages\nCritical information first",
              80, 171, 320, &Schoolbell_24, 0xFFFFFF);
        button("DEFAULT PAGES", 92, 256, 296, 64, DEFAULTS, false);
        label(content, "Local preview - not sent to gauges",
              70, 344, 340, LV_FONT_DEFAULT, 0x80B6D0);
    } else if(page == GAUGE_PAGE) {
        label(content, gauges[selected_gauge], 100, 166, 280,
              &Schoolbell_24, 0xFFFFFF);
        button("<", 54, 218, 70, 84, PAGE_PREV, false);
        button(">", 356, 218, 70, 84, PAGE_NEXT, false);
        label(content, pages[selected_gauge][selected_page[selected_gauge]],
              129, 237, 222, &Schoolbell_24, 0xFFFFFF);
        label(content, "PREVIEW - GAUGE OFFLINE", 70, 344, 340,
              LV_FONT_DEFAULT, 0x80B6D0);
    }
    if(page != HOME) button("BACK", 156, 394, 168, 44,
                            page == GAUGE_PAGE ? GAUGES : HOME, false);
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_remove_style_all(ui_Screen1);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    content = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, 480, 480);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    /* All four page previews always start at the critical/default page. */
    for(unsigned i = 0; i < 4; ++i) selected_page[i] = 0;
    show(HOME);
}

void ui_Screen1_screen_destroy(void)
{
    if(ui_Screen1) lv_obj_del(ui_Screen1);
    ui_Screen1 = content = mode_value = mode_caption = brightness_value = NULL;
}
