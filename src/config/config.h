#ifndef CT_CONFIG_H
#define CT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ct_grade { CT_AGAIN = 1, CT_HARD = 2, CT_GOOD = 3, CT_EASY = 4 } ct_grade;

typedef enum ct_orbit {
    CT_ORBIT_CORNER = 0,
    CT_ORBIT_EDGE,
    CT_ORBIT_CENTER,
    CT_ORBIT_WING
} ct_orbit;

typedef enum ct_buffer_mode { CT_BUFFER_FIXED = 0, CT_BUFFER_FLOATING } ct_buffer_mode;

typedef enum ct_classification {
    CT_CLS_PURE = 0,
    CT_CLS_A9,
    CT_CLS_B9,
    CT_CLS_ORTHOGONAL,
    CT_CLS_CYCLIC_SHIFT,
    CT_CLS_COLUMN,
    CT_CLS_PER_SPECIAL,
    CT_CLS_HALF_SLICE_PLANE,
    CT_CLS_SP5,
    CT_CLS_SP7,
    CT_CLS_SP9,
    CT_CLS_PARITY,
    CT_CLS_FLIP,
    CT_CLS_TWIST,
    CT_CLS_UNKNOWN
} ct_classification;

typedef enum ct_event {
    CT_EVENT_3X3 = 0,
    CT_EVENT_2X2,
    CT_EVENT_3BLD,
    CT_EVENT_4BLD,
    CT_EVENT_5BLD,
    CT_EVENT_MULTI_BLD,
    CT_EVENT_FMC,
    CT_EVENT_OH,
    CT_EVENT_4X4,
    CT_EVENT_5X5,
    CT_EVENT_6X6,
    CT_EVENT_7X7,
    CT_EVENT_CLOCK,
    CT_EVENT_MEGAMINX,
    CT_EVENT_PYRAMINX,
    CT_EVENT_SKEWB,
    CT_EVENT_SQUARE1,
    CT_EVENT_FTO
} ct_event;

typedef enum ct_method {
    CT_METHOD_CFOP = 0,
    CT_METHOD_3STYLE,
    CT_METHOD_OLD_POCHMANN,
    CT_METHOD_M2,
    CT_METHOD_U2,
    CT_METHOD_R2,
    CT_METHOD_LBL,
    CT_METHOD_L4E,
    CT_METHOD_4LLL,
    CT_METHOD_SARAH_INTERMEDIATE,
    CT_METHOD_YAU,
    CT_METHOD_REDUCTION,
    CT_METHOD_CLL,
    CT_METHOD_L4T_REDUX,
    CT_METHOD_CONCEPT
} ct_method;

typedef enum ct_stage {
    CT_STAGE_PLL = 0,
    CT_STAGE_OLL,
    CT_STAGE_F2L,
    CT_STAGE_CROSS,
    CT_STAGE_CLL,
    CT_STAGE_CUBESHAPE,
    CT_STAGE_CO,
    CT_STAGE_EO,
    CT_STAGE_CP,
    CT_STAGE_EP,
    CT_STAGE_L4E,
    CT_STAGE_4LLL,
    CT_STAGE_SARAH,
    CT_STAGE_L4T,
    CT_STAGE_PARITY,
    CT_STAGE_CONCEPT
} ct_stage;

typedef enum ct_direction { CT_DIR_FR = 0, CT_DIR_FL, CT_DIR_BR, CT_DIR_BL } ct_direction;

typedef enum ct_ui_screen {
    CT_SCREEN_MENU = 0,
    CT_SCREEN_TRAINER,
    CT_SCREEN_CARDS,
    CT_SCREEN_REVIEW,
    CT_SCREEN_SETTINGS,
    CT_SCREEN_STATS
} ct_ui_screen;

typedef enum ct_move_face {
    CT_MOVE_BIT_U = 1u << 0,
    CT_MOVE_BIT_D = 1u << 1,
    CT_MOVE_BIT_L = 1u << 2,
    CT_MOVE_BIT_R = 1u << 3,
    CT_MOVE_BIT_F = 1u << 4,
    CT_MOVE_BIT_B = 1u << 5,
    CT_MOVE_BIT_M = 1u << 6,
    CT_MOVE_BIT_E = 1u << 7,
    CT_MOVE_BIT_S = 1u << 8,
    CT_MOVE_BIT_X = 1u << 9,
    CT_MOVE_BIT_Y = 1u << 10,
    CT_MOVE_BIT_Z = 1u << 11
} ct_move_face;

typedef enum ct_ease { CT_EASE_LINEAR = 0, CT_EASE_CUBIC_IN_OUT } ct_ease;

typedef enum ct_cfg_status {
    CT_CFG_OK = 0,
    CT_CFG_UNKNOWN_KEY = -1,
    CT_CFG_BAD_VALUE = -2
} ct_cfg_status;

#define CT_FACE_COUNT 12
#define CT_FSRS_WEIGHT_COUNT 19
#define CT_FSRS_REQUEST_RETENTION 0.9
#define CT_FSRS_MAX_IVL_DAYS 36500.0
#define CT_FSRS_MIN_STABILITY 0.1
#define CT_FSRS_MAX_DIFFICULTY 10.0
#define CT_FSRS_MIN_DIFFICULTY 1.0
#define CT_FSRS_FACTOR (19.0 / 81.0)
#define CT_FSRS_DECAY (-0.5)
#define CT_FSRS_RETENTION_MIN 0.7
#define CT_FSRS_RETENTION_MAX 0.99
#define CT_GEN_DEFAULT_PREFERRED_MOVES ((1u << 0) | (1u << 1) | (1u << 3))
#define CT_GEN_PENALTY_PREFERRED 0.0
#define CT_GEN_PENALTY_OTHER 2.0
#define CT_GEN_WEIGHT_LEN 1.0
#define CT_GEN_WEIGHT_SUBSET 2.0
#define CT_GEN_WEIGHT_REGRIP 1.5
#define CT_GEN_WEIGHT_FINGER 0.5
#define CT_FINGER_FACE_COUNT 6
#define CT_FINGER_DIR_COUNT 2
#define CT_FINGER_COST_CHEAP 0.2
#define CT_FINGER_COST_MEDIUM 0.4
#define CT_FINGER_COST_EXPENSIVE 0.8
#define CT_GEN_SETUP_BFS_DEPTH 4
#define CT_GEN_FALLBACK_DEPTH 5
#define CT_GEN_STRAGGLER_DEPTH 6
#define CT_MIN_DEPTH 1
#define CT_ANIM_MOVE_SECONDS 0.25
#define CT_ANIM_MIN_MOVE_SECONDS 0.01
#define CT_ANIM_QUEUE_CAP 64
#define CT_ANIM_SPEED_EPS_SCALE 1000000
#define CT_ANIM_EASE_HALF 0.5
#define CT_ANIM_EASE_CUBIC_SCALE 4.0
#define CT_RENDERER_MOVE_MILLIS 350
#define CT_RENDERER_DEFAULT_CUBE_SIZE 3.0f
#define CT_RENDERER_DEFAULT_INSET 0.12f
#define CT_RENDERER_INSET_MAX 0.95f
#define CT_RENDERER_OUTSET_FRACTION 0.02f
#define CT_RENDERER_PI 3.14159265358979323846
#define CT_RENDERER_RIGHT_ANGLE_DEGREES 90.0
#define CT_RENDERER_STRAIGHT_ANGLE_DEGREES 180.0
#define CT_MESH_FLOATS_PER_VERT 3
#define CT_MESH_BYTES_PER_COLOR 4
#define CT_QUAD_CORNER_COUNT 4
#define CT_QUAD_CORNER_ROW_MIN_COL_MIN 0
#define CT_QUAD_CORNER_ROW_MAX_COL_MIN 1
#define CT_QUAD_CORNER_ROW_MAX_COL_MAX 2
#define CT_QUAD_CORNER_ROW_MIN_COL_MAX 3
#define CT_WINDOW_WIDTH 1280
#define CT_WINDOW_HEIGHT 720
#define CT_WINDOW_MIN_DIMENSION 1
#define CT_WINDOW_TARGET_FPS 60
#define CT_MIN_FPS 1
#define CT_HEADLESS_WIDTH 960
#define CT_HEADLESS_HEIGHT 540
#define CT_HEADLESS_MID_PROGRESS 0.5
#define CT_HEADLESS_CAM_X 4.5f
#define CT_HEADLESS_CAM_Y 4.5f
#define CT_HEADLESS_CAM_Z 4.5f
#define CT_HEADLESS_CAM_FOVY 45.0f
#define CT_DB_DIR_NAME "cubing-trainer"
#define CT_DB_FILENAME "trainer.db"
#define CT_DB_NAME_MAX 128
#define CT_INTERCHANGE_MAX_CANDIDATES 180
#define CT_INTERCHANGE_PERM_UNSET 0xFF
#define CT_SQ1_UNITS 12
#define CT_SQ1_PIECES 16
#define CT_CLOCK_CLOCKS 18
#define CT_CLOCK_HOURS 12
#define CT_STATS_DECK_MAX 64
#define CT_STATS_CASE_KEY_MAX 64
#define CT_STATS_DAY_MAX 11
#define CT_STATS_STAMP_MAX 32
#define CT_FUZZ_ITERATIONS 10000
#define CT_DB_BUSY_TIMEOUT_MS 5000
#define CT_DB_PATH_MAX 260
#define CT_DATA_DIR_ENV "LOCALAPPDATA"
#define CT_MIGRATIONS_ENV "CT_MIGRATIONS_DIR"
#define CT_MIGRATIONS_EXE_REL "../../db/migrations"
#define CT_APP_EXE_PATH_MAX 1024
#define CT_PROC_SELF_EXE "/proc/self/exe"
#define CT_DIR_MODE 0755
#define CT_PATH_ROOT_DRIVE 2
#define CT_PATH_ROOT_DRIVE_SEP 3
#define CT_PATH_ROOT_ABS 1
#define CT_PATH_UNC_COMPONENTS 2
#define CT_MIG_PATH_MAX (CT_DB_PATH_MAX * 2)
#define CT_MIG_COPY_BUF 4096
#define CT_MIG_PRAGMA_MAX 64
#define CT_DECIMAL_BASE 10
#define CT_LOOP_CATCHUP_STEPS 4
#define CT_UI_TITLE_SIZE_DIVISOR 14
#define CT_UI_BUTTON_HEIGHT_DIVISOR 16
#define CT_CLASSIFY_LABEL_MAX 24
#define CT_CLASSIFY_REASON_MAX 96
#define CT_CLASSIFY_SETUP_MAX 4
#define CT_CLASSIFY_PART_MAX 5
#define CT_CLASSIFY_MOVES_MAX 64
#define CT_CLASSIFY_INSERT_LEN 3
#define CT_CLASSIFY_SPECIAL_LEN 5
#define CT_CLASSIFY_SP5_MOVES 5
#define CT_CLASSIFY_SP7_MOVES 7
#define CT_CLASSIFY_SP9_MOVES 9
#define CT_CLASSIFY_COLUMN_MOVES 11
#define CT_CLASSIFY_FMT_MAX 48

static inline unsigned ct_move_face_from_char(char c) {
    switch (c) {
    case 'U':
        return CT_MOVE_BIT_U;
    case 'D':
        return CT_MOVE_BIT_D;
    case 'L':
        return CT_MOVE_BIT_L;
    case 'R':
        return CT_MOVE_BIT_R;
    case 'F':
        return CT_MOVE_BIT_F;
    case 'B':
        return CT_MOVE_BIT_B;
    case 'M':
        return CT_MOVE_BIT_M;
    case 'E':
        return CT_MOVE_BIT_E;
    case 'S':
        return CT_MOVE_BIT_S;
    case 'x':
        return CT_MOVE_BIT_X;
    case 'y':
        return CT_MOVE_BIT_Y;
    case 'z':
        return CT_MOVE_BIT_Z;
    default:
        return 0;
    }
}

static inline char ct_move_face_char(unsigned face) {
    switch (face) {
    case CT_MOVE_BIT_U:
        return 'U';
    case CT_MOVE_BIT_D:
        return 'D';
    case CT_MOVE_BIT_L:
        return 'L';
    case CT_MOVE_BIT_R:
        return 'R';
    case CT_MOVE_BIT_F:
        return 'F';
    case CT_MOVE_BIT_B:
        return 'B';
    case CT_MOVE_BIT_M:
        return 'M';
    case CT_MOVE_BIT_E:
        return 'E';
    case CT_MOVE_BIT_S:
        return 'S';
    case CT_MOVE_BIT_X:
        return 'x';
    case CT_MOVE_BIT_Y:
        return 'y';
    case CT_MOVE_BIT_Z:
        return 'z';
    default:
        return '\0';
    }
}

typedef struct ct_fsrs_cfg {
    double w[CT_FSRS_WEIGHT_COUNT];
    double request_retention;
    double max_ivl_days;
    double min_stability;
    double max_difficulty;
    double min_difficulty;
    double factor;
    double decay;
} ct_fsrs_cfg;

typedef enum ct_finger_dir { CT_FINGER_DIR_CW = 0, CT_FINGER_DIR_CCW = 1 } ct_finger_dir;

extern const double ct_finger_defaults[CT_FINGER_FACE_COUNT][CT_FINGER_DIR_COUNT];

typedef struct ct_gen_cfg {
    unsigned preferred_moves;
    double penalty[CT_FACE_COUNT];
    double weight_len;
    double weight_subset;
    double weight_regrip;
    double weight_finger;
    double finger_cost[CT_FINGER_FACE_COUNT][CT_FINGER_DIR_COUNT];
    int setup_bfs_depth;
    int fallback_depth;
    int straggler_depth;
} ct_gen_cfg;

typedef struct ct_anim_cfg {
    double move_seconds;
    ct_ease ease;
} ct_anim_cfg;

typedef struct ct_window_cfg {
    int width;
    int height;
    int target_fps;
} ct_window_cfg;

typedef struct ct_db_cfg {
    char dir_name[CT_DB_NAME_MAX];
    char filename[CT_DB_NAME_MAX];
} ct_db_cfg;

typedef struct ct_fuzz_cfg {
    int iterations;
} ct_fuzz_cfg;

typedef struct ct_config {
    ct_fsrs_cfg fsrs;
    ct_gen_cfg gen;
    ct_anim_cfg anim;
    ct_window_cfg window;
    ct_db_cfg db;
    ct_fuzz_cfg fuzz;
} ct_config;

ct_config ct_config_defaults(void);

int ct_config_set(ct_config *cfg, const char *key, const char *value);

#ifdef __cplusplus
}
#endif

#endif
