#include "trainer/cfop.h"

#include <stdio.h>
#include <string.h>

#include "storage/repo_cards.h"

static const char *const ct_cfop_pll_cases[] = {"Aa", "Ab", "E",  "F",  "Ga", "Gb", "Gc",
                                                "Gd", "H",  "Ja", "Jb", "Na", "Nb", "Ra",
                                                "Rb", "T",  "Ua", "Ub", "V",  "Y",  "Z"};

static const char *const ct_cfop_oll_cases[] = {
    "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14", "15",
    "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
    "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45",
    "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57"};

static const char *const ct_cfop_f2l_cases[] = {
    "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14",
    "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28",
    "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41"};

static const struct {
    const char *slot;
    int direction;
} ct_cfop_f2l_slots[] = {
    {"fr", CT_DIR_FR}, {"fl", CT_DIR_FL}, {"br", CT_DIR_BR}, {"bl", CT_DIR_BL}};

static const struct {
    const char *deck;
    const char *tag;
} ct_cfop_stages[] = {[CT_STAGE_PLL] = {"cfop_pll", "pll"},
                      [CT_STAGE_OLL] = {"cfop_oll", "oll"},
                      [CT_STAGE_F2L] = {"cfop_f2l", "f2l"}};

static int ct_cfop_stage_ok(ct_stage stage) {
    return (int)stage >= 0 && (int)stage <= (int)CT_STAGE_F2L;
}

static int ct_cfop_f2l_per_slot(void) {
    return (int)(sizeof ct_cfop_f2l_cases / sizeof ct_cfop_f2l_cases[0]);
}

int ct_cfop_case_count(ct_stage stage) {
    if (!ct_cfop_stage_ok(stage)) {
        return -1;
    }
    switch (stage) {
    case CT_STAGE_PLL:
        return (int)(sizeof ct_cfop_pll_cases / sizeof ct_cfop_pll_cases[0]);
    case CT_STAGE_OLL:
        return (int)(sizeof ct_cfop_oll_cases / sizeof ct_cfop_oll_cases[0]);
    default:
        return ct_cfop_f2l_per_slot() *
               (int)(sizeof ct_cfop_f2l_slots / sizeof ct_cfop_f2l_slots[0]);
    }
}

static int ct_cfop_case_name(ct_stage stage, int index, char *out, unsigned cap) {
    switch (stage) {
    case CT_STAGE_PLL:
        return snprintf(out, cap, "%s", ct_cfop_pll_cases[index]);
    case CT_STAGE_OLL:
        return snprintf(out, cap, "%s", ct_cfop_oll_cases[index]);
    default:
        return snprintf(out, cap, "%s.%s", ct_cfop_f2l_slots[index / ct_cfop_f2l_per_slot()].slot,
                        ct_cfop_f2l_cases[index % ct_cfop_f2l_per_slot()]);
    }
}

int ct_cfop_case_key(ct_stage stage, int index, char *out, unsigned cap) {
    if (out == NULL) {
        return -1;
    }
    int count = ct_cfop_case_count(stage);
    if (count < 0 || index < 0 || index >= count) {
        return -1;
    }
    char name[CT_CARD_CASE_KEY_MAX];
    int len = ct_cfop_case_name(stage, index, name, sizeof name);
    if (len < 0 || (unsigned)len >= sizeof name) {
        return -1;
    }
    char key[CT_CARD_CASE_KEY_MAX];
    len = snprintf(key, sizeof key, "%s.%s", ct_cfop_stages[stage].tag, name);
    if (len < 0 || (unsigned)len >= sizeof key || (unsigned)len >= cap) {
        return -1;
    }
    memcpy(out, key, (size_t)len + 1);
    return 0;
}

int ct_cfop_case_direction(ct_stage stage, int index) {
    int count = ct_cfop_case_count(stage);
    if (count < 0 || index < 0 || index >= count) {
        return -1;
    }
    if (stage != CT_STAGE_F2L) {
        return CT_DIR_FR;
    }
    return ct_cfop_f2l_slots[index / ct_cfop_f2l_per_slot()].direction;
}

int ct_cfop_deck_name(ct_stage stage, char *out, unsigned cap) {
    if (out == NULL || !ct_cfop_stage_ok(stage)) {
        return -1;
    }
    const char *deck = ct_cfop_stages[stage].deck;
    size_t len = strlen(deck);
    if (cap <= len) {
        return -1;
    }
    memcpy(out, deck, len + 1);
    return 0;
}

int ct_cfop_populate(sqlite3 *db, ct_stage stage) {
    if (db == NULL) {
        return -1;
    }
    int count = ct_cfop_case_count(stage);
    if (count < 0) {
        return -1;
    }
    char deck[CT_CARD_DECK_MAX];
    if (ct_cfop_deck_name(stage, deck, sizeof deck) != 0) {
        return -1;
    }
    int created = 0;
    for (int i = 0; i < count; i++) {
        char key[CT_CARD_CASE_KEY_MAX];
        if (ct_cfop_case_key(stage, i, key, sizeof key) != 0) {
            return -1;
        }
        int direction = ct_cfop_case_direction(stage, i);
        if (direction < 0) {
            return -1;
        }
        ct_card existing;
        int rc = ct_repo_card_get_by_key(db, deck, key, direction, &existing);
        if (rc == CT_REPO_OK) {
            continue;
        }
        if (rc != CT_REPO_MISSING) {
            return -1;
        }
        char name[CT_CARD_CASE_KEY_MAX];
        int len = ct_cfop_case_name(stage, i, name, sizeof name);
        if (len < 0 || (unsigned)len >= sizeof name) {
            return -1;
        }
        ct_card c;
        memset(&c, 0, sizeof c);
        memcpy(c.deck, deck, strlen(deck) + 1);
        memcpy(c.case_key, key, strlen(key) + 1);
        c.direction = direction;
        snprintf(c.front_json, sizeof c.front_json, "{\"stage\":\"%s\",\"case\":\"%s\"}",
                 ct_cfop_stages[stage].tag, name);
        rc = ct_repo_card_insert(db, &c);
        if (rc == CT_REPO_CONFLICT) {
            continue;
        }
        if (rc != CT_REPO_OK) {
            return -1;
        }
        created++;
    }
    return created;
}
