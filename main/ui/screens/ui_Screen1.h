#ifndef UI_SCREEN1_H
#define UI_SCREEN1_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_Screen1_screen_init(void);
extern void ui_Screen1_screen_destroy(void);

extern lv_obj_t *ui_Screen1;
extern lv_obj_t *ui_lambda_band;
extern lv_obj_t *ui_img_lambda;
extern lv_obj_t *ui_label_lambda_val;

extern lv_obj_t *ui_bar_boost;
extern lv_obj_t *ui_label_boost_ch;
extern lv_obj_t *ui_label_boost_val;

extern lv_obj_t *ui_bar_ect;
extern lv_obj_t *ui_label_ect_ch;
extern lv_obj_t *ui_label_ect_val;

extern lv_obj_t *ui_bar_ign;
extern lv_obj_t *ui_label_ign_ch;
extern lv_obj_t *ui_label_ign_val;

extern lv_obj_t *ui_bar_iat;
extern lv_obj_t *ui_label_iat_ch;
extern lv_obj_t *ui_label_iat_val;

extern lv_obj_t *ui_no_ecu_pill;
extern lv_obj_t *ui_no_ecu_label;

#ifdef __cplusplus
}
#endif

#endif
