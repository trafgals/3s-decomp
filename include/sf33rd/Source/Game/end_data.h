#ifndef END_DATA_H
#define END_DATA_H

#include "structs.h"
#include "types.h"

extern const u16* ending_map_tbl[][3];

extern const u8 end_use_scr[];
extern const u8 end_use_real_scr[];
extern const u16 bgtex_ending_file[];
extern const u32 ending_priority[];
extern const u8 ending_opaque[];
extern const u32 bgtex_ending_gbix[][6];
extern const u8 ending_use_family[];
extern const u8 ending_rewrite_scr[];
extern const s8 edrw_num[20][2];

#if defined(TARGET_PS2)
extern const u32 edrw_data_tbl[][3];
extern const u32 gedrw_data_tbl[][2];
extern const u32 cedrw_data_tbl[][2];
#else
// The PS2 tables embed absolute addresses of the ending data, which is what
// the retail binary expects. A host build cannot truncate a 64-bit pointer
// into a u32, so there the address column keeps a real pointer instead.
// This mirrors the layout used for bgrw_data_tbl in bg_data.h.
typedef struct {
    u32 bg_num;
    u32 rwgbix;
    const s16* rw_ptr;
} edrw_data_tbl_elem;

typedef struct {
    u32 rwgbix;
    const s16* rw_ptr;
} gedrw_data_tbl_elem;

extern const edrw_data_tbl_elem edrw_data_tbl[];
extern const gedrw_data_tbl_elem gedrw_data_tbl[];
extern const gedrw_data_tbl_elem cedrw_data_tbl[];
#endif

extern const s32 gouki_end_nosekae[2][16];

extern s8 staff_r_no;
extern s8 end_name_cut[2];
extern s16 end_fade_timer;
extern s8 end_staff_flag;
extern s8 end_no_cut;
extern s8 end_fade_flag;
extern s8 ending_all_end;
extern s8 end_etc_flag;
extern s16 e_line_step;
extern END_W end_w;

#endif // END_DATA_H
