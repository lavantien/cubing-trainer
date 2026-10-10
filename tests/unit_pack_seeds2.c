#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "../src/packs/packs.h"
#include "../src/storage/db.h"
#include "framework.h"

enum { CT_SEED2_LIST_CAP = 192 };

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    ct_db_open(&db, ":memory:");
    ct_db_migrate(db, "db/migrations");
    return db;
}

static int tsv_data_lines(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    int lines = 0;
    int c;
    int last = '\n';
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n')
            lines++;
        last = c;
    }
    if (last != '\n')
        lines++;
    fclose(f);
    return lines - 1;
}

static void stored_hash(sqlite3 *db, ct_event ev, ct_method m, ct_stage st, const char *case_key,
                        char *out, size_t cap) {
    sqlite3_stmt *stmt = NULL;
    out[0] = '\0';
    if (sqlite3_prepare_v2(db,
                           "SELECT content_hash FROM event_pack WHERE event = ? AND method = ?"
                           " AND stage = ? AND case_key = ?",
                           -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, (int)ev);
        sqlite3_bind_int(stmt, 2, (int)m);
        sqlite3_bind_int(stmt, 3, (int)st);
        sqlite3_bind_text(stmt, 4, case_key, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *s = sqlite3_column_text(stmt, 0);
            if (s != NULL)
                snprintf(out, cap, "%s", (const char *)s);
        }
    }
    sqlite3_finalize(stmt);
}

static void verify_pack(const char *path, ct_event ev, ct_method m, ct_stage st, int expected) {
    sqlite3 *db = open_db();
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    CT_ASSERT_EQ(ct_pack_count(db, ev, m, st), expected);
    CT_ASSERT_EQ(tsv_data_lines(path), expected);
    ct_pack_entry got[CT_SEED2_LIST_CAP];
    CT_ASSERT(expected <= CT_SEED2_LIST_CAP);
    int n = ct_pack_list(db, ev, m, st, got, expected);
    CT_ASSERT_EQ(n, expected);
    for (int i = 0; i < n; i++) {
        CT_ASSERT(got[i].case_key[0] != '\0');
        CT_ASSERT(got[i].alg[0] != '\0');
        CT_ASSERT(got[i].front_json[0] != '\0');
        CT_ASSERT(got[i].source_url[0] != '\0');
        CT_ASSERT(got[i].source_at[0] != '\0');
        CT_ASSERT_EQ(got[i].event, ev);
        CT_ASSERT_EQ(got[i].method, m);
        CT_ASSERT_EQ(got[i].stage, st);
        char hex[CT_PACK_HASH_MAX];
        char stored[CT_PACK_HASH_MAX];
        CT_ASSERT_EQ(ct_pack_content_hash(&got[i], hex), CT_PACK_OK);
        stored_hash(db, ev, m, st, got[i].case_key, stored, sizeof stored);
        CT_ASSERT_STR_EQ(stored, hex);
    }
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    CT_ASSERT_EQ(ct_pack_count(db, ev, m, st), expected);
    ct_db_close(db);
}

static ct_pack_entry fetch_one(const char *path, ct_event ev, ct_method m, ct_stage st,
                               const char *case_key, int expected) {
    sqlite3 *db = open_db();
    CT_ASSERT_EQ(ct_pack_seed_file(db, path, ev, m, st), expected);
    ct_pack_entry got;
    CT_ASSERT_EQ(ct_pack_get(db, ev, m, st, case_key, &got), CT_PACK_OK);
    ct_db_close(db);
    return got;
}

CT_SUITE(pack_seed_sq1_cubeshape) {
    verify_pack("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                CT_STAGE_CUBESHAPE, 169);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CUBESHAPE, "cs.001", 169);
    CT_ASSERT_STR_EQ(e.alg, "/");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Kite / Kite\"}");
    e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                  CT_STAGE_CUBESHAPE, "cs.011", 169);
    CT_ASSERT_STR_EQ(e.alg, "/ 1,2 / -3,-3 /");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Scallop / Scallop\"}");
    e = fetch_one("db/seeds/packs/sq1_cubeshape.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                  CT_STAGE_CUBESHAPE, "cs.012", 169);
    CT_ASSERT_STR_EQ(e.alg, "/ 2,1 / 0,3 /");
}

CT_SUITE(pack_seed_sq1_co) {
    verify_pack("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO, 7);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CO, "co.01", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0)/(3,0)/(-1,0)");
    e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO,
                  "co.03", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0)/(-1,0)");
    e = fetch_one("db/seeds/packs/sq1_co.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CO,
                  "co.07", 7);
    CT_ASSERT_STR_EQ(e.alg, "/ (6,6) / (-1,1)");
}

CT_SUITE(pack_seed_sq1_eo) {
    verify_pack("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO, 7);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_EO, "eo.01", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0) / (3,0) / (3,0) / (-1,-1) / (-2,1) / (-3,0) /");
    e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO,
                  "eo.03", 7);
    CT_ASSERT_STR_EQ(e.alg, "(0,-1) / (0,-3) / (0,-3) / (1,1) / (-1,2) / (1,4) / (-1,0)");
    e = fetch_one("db/seeds/packs/sq1_eo.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EO,
                  "eo.07", 7);
    CT_ASSERT_STR_EQ(e.alg, "(1,0) / (-1,-1) / (3,3) / (1,1) / (-1,0)");
}

CT_SUITE(pack_seed_sq1_cp) {
    verify_pack("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP, 16);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_CP, "cp.01", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,-3) / (-3,0) / (0,3) / (0,-3) / (0,3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cp.08", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,-3) / (-3,3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cpp.01", 16);
    CT_ASSERT_STR_EQ(e.alg,
                     "/ (3,3) / (-1,0) / (2,0) / (-4,0) / (4,0) / (2,0) / (1,0) / (-3,-3) /");
    e = fetch_one("db/seeds/packs/sq1_cp.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_CP,
                  "cpp.08", 16);
    CT_ASSERT_STR_EQ(e.alg, "/ (3,3) / (1,0) / (4,-2) / (2,-4) / (0,-4) / (3,3) / (3,0) / (3,3) /");
}

CT_SUITE(pack_seed_sq1_ep) {
    verify_pack("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP, 99);
    ct_pack_entry e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL,
                                CT_STAGE_EP, "ep.01", 99);
    CT_ASSERT_STR_EQ(e.alg, "/ (-3,0) / (0,3) / (0,-3) / (0,3) / (2,0) / (0,2) / (-2,0) / (4,0)"
                            " / (0,-2) / (0,2) / (-1,4) / (0,-3) / (0,3) *");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Adj on U\",\"parity\":true}");
    e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP,
                  "ep.10", 99);
    CT_ASSERT_STR_EQ(e.alg, "/ (0,3) / (-3,0) / (3,0) / (-3,0) / (0,-2) / (-2,0) / (0,2) / (0,-4)"
                            " / (2,0) / (-2,0) / (-4,1) / (3,0) / (-3,0) *");
    e = fetch_one("db/seeds/packs/sq1_ep.tsv", CT_EVENT_SQUARE1, CT_METHOD_LBL, CT_STAGE_EP,
                  "ep.99", 99);
    CT_ASSERT_STR_EQ(e.alg,
                     "(U) (1,0) / (5,-1) / (3,3) / (3,0) / (1,1) / (-3,0) / (3,-3) / (5,0) *");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"W-W\"}");
}

CT_SUITE(pack_seed_clock) {
    verify_pack("db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT,
                14);
    ct_pack_entry e = fetch_one("db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL,
                                CT_STAGE_CONCEPT, "clock.01", 14);
    CT_ASSERT_STR_EQ(e.alg, "Push down the bottom two pins, push up the top two. Turn a top corner"
                            " until the centre clock matches the bottom middle.");
    e = fetch_one("db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT,
                  "clock.05", 14);
    CT_ASSERT_STR_EQ(e.alg, "Push up all pins. Turn any corner till the centre clock is 12.");
    e = fetch_one("db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT,
                  "clock.10", 14);
    CT_ASSERT_STR_EQ(e.alg, "Push down the bottom left pin, push the other three up. Turn a top"
                            " corner till the centre clock matches the bottom left clock.");
    e = fetch_one("db/seeds/packs/clock_algs.tsv", CT_EVENT_CLOCK, CT_METHOD_LBL, CT_STAGE_CONCEPT,
                  "clock.14", 14);
    CT_ASSERT_STR_EQ(e.alg, "Push up all pins. Turn any corner till the centre clock is 12.");
}

CT_SUITE(pack_seed_bigcube_parity) {
    verify_pack("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY,
                5);
    ct_pack_entry e = fetch_one("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU,
                                CT_STAGE_PARITY, "parity.oll4", 5);
    CT_ASSERT_STR_EQ(e.alg, "Rw U2, X, Rw U2, Rw U2 , Rw' U2, Lw U2, Rw' U2, Rw U2, Rw' U2, Rw'");
    e = fetch_one("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY,
                  "parity.pll4", 5);
    CT_ASSERT_STR_EQ(e.alg, "r2, U2, r2, Uw2, r2, u2");
    e = fetch_one("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY,
                  "parity.wing5", 5);
    CT_ASSERT_STR_EQ(e.alg, "Rw U2 x Rw U2 Rw U2 Rw' U2 Lw U2 3Rw' U2 Rw U2 Rw' U2 Rw'");
    e = fetch_one("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY,
                  "parity.oll4flip", 5);
    CT_ASSERT_STR_EQ(e.alg, "r', U2, l, F2, l', F2, r2, U2, r, U2, r', U2, F2, r2, F2");
    e = fetch_one("db/seeds/packs/bigcube_parity.tsv", CT_EVENT_4X4, CT_METHOD_YAU, CT_STAGE_PARITY,
                  "parity.pll4adj", 5);
    CT_ASSERT_STR_EQ(e.alg, "(R' U R U' ) r2, U2, r2, Uw2, r2, u2, (U R' U' R )");
}

CT_SUITE(pack_seed_m2) {
    verify_pack("db/seeds/packs/bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT, 22);
    ct_pack_entry e = fetch_one("db/seeds/packs/bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2,
                                CT_STAGE_CONCEPT, "m2.01", 22);
    CT_ASSERT_STR_EQ(e.alg, "M2");
    CT_ASSERT_STR_EQ(e.front_json, "{\"buffer\":\"DF\",\"target\":\"UB\"}");
    CT_ASSERT_STR_EQ(e.source_url, "https://www.speedsolving.com/wiki/index.php/M2");
    e = fetch_one("db/seeds/packs/bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT,
                  "m2.11", 22);
    CT_ASSERT_STR_EQ(e.alg, "x' U' R2 U M2 U' R2 U x");
    CT_ASSERT_STR_EQ(e.front_json, "{\"buffer\":\"DF\",\"target\":\"RF\"}");
    e = fetch_one("db/seeds/packs/bld_m2.tsv", CT_EVENT_3BLD, CT_METHOD_M2, CT_STAGE_CONCEPT,
                  "m2.22", 22);
    CT_ASSERT_STR_EQ(e.alg, "M2' D U R2 U' M' U R2 U' M D'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"buffer\":\"DF\",\"target\":\"BD\"}");
}

CT_SUITE(pack_seed_oldpoch) {
    verify_pack("db/seeds/packs/bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN,
                CT_STAGE_CONCEPT, 38);
    ct_pack_entry e = fetch_one("db/seeds/packs/bld_oldpoch.tsv", CT_EVENT_3BLD,
                                CT_METHOD_OLD_POCHMANN, CT_STAGE_CONCEPT, "op.01", 38);
    CT_ASSERT_STR_EQ(e.alg, "R U R' U' R' F R2 U' R' U' R U R' F'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"buffer\":\"UR\",\"target\":\"UL\",\"kind\":\"edge swap\"}");
    CT_ASSERT_STR_EQ(e.source_url, "https://jperm.net/bld");
    e = fetch_one("db/seeds/packs/bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN,
                  CT_STAGE_CONCEPT, "op.13", 38);
    CT_ASSERT_STR_EQ(e.alg, "L");
    CT_ASSERT_STR_EQ(e.front_json, "{\"letter\":\"R\",\"orbit\":\"edge\",\"target_pos\":\"UL\"}");
    e = fetch_one("db/seeds/packs/bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN,
                  CT_STAGE_CONCEPT, "op.24", 38);
    CT_ASSERT_STR_EQ(e.alg, "D L2");
    CT_ASSERT_STR_EQ(e.front_json, "{\"letter\":\"W\",\"orbit\":\"edge\",\"target_pos\":\"UL\"}");
    e = fetch_one("db/seeds/packs/bld_oldpoch.tsv", CT_EVENT_3BLD, CT_METHOD_OLD_POCHMANN,
                  CT_STAGE_CONCEPT, "op.38", 38);
    CT_ASSERT_STR_EQ(e.alg, "F' D");
    CT_ASSERT_STR_EQ(e.front_json,
                     "{\"letter\":\"F\",\"orbit\":\"corner\",\"target_pos\":\"DFR\"}");
}

CT_SUITE(pack_seed_fto) {
    verify_pack("db/seeds/packs/fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX, CT_STAGE_L4T,
                36);
    ct_pack_entry e = fetch_one("db/seeds/packs/fto_l4t_redux.tsv", CT_EVENT_FTO,
                                CT_METHOD_L4T_REDUX, CT_STAGE_L4T, "l4t.01", 36);
    CT_ASSERT_STR_EQ(e.alg, "L' U L U L' U L");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Last Layer.Sune\"}");
    CT_ASSERT_STR_EQ(
        e.source_url,
        "https://docs.google.com/spreadsheets/d/1_IHL077w-9U-qtpF9hvC6CrUn7etua5QgPH8vEMqK58/edit");
    e = fetch_one("db/seeds/packs/fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX,
                  CT_STAGE_L4T, "l4t.11", 36);
    CT_ASSERT_STR_EQ(e.alg, "R U R'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Last Three Edges.Sexy\"}");
    e = fetch_one("db/seeds/packs/fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX,
                  CT_STAGE_L4T, "l4t.21", 36);
    CT_ASSERT_STR_EQ(e.alg, "L' U L R U' R'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Separated Bar.Good Sochi\"}");
    e = fetch_one("db/seeds/packs/fto_l4t_redux.tsv", CT_EVENT_FTO, CT_METHOD_L4T_REDUX,
                  CT_STAGE_L4T, "l4t.36", 36);
    CT_ASSERT_STR_EQ(e.alg, "U BR U BR' F U F'");
    CT_ASSERT_STR_EQ(e.front_json, "{\"case\":\"Reduction.parity fix\"}");
    CT_ASSERT_STR_EQ(
        e.source_url,
        "https://docs.google.com/document/d/18CMTrFC9PYkEsheOHnywzvV0EdCb3SFGQXSK0DDb30o/edit");
}

CT_SUITE(pack_seed_fmc) {
    verify_pack("db/seeds/packs/fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT,
                CT_STAGE_CONCEPT, 16);
    ct_pack_entry e = fetch_one("db/seeds/packs/fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT,
                                CT_STAGE_CONCEPT, "fmc.01", 16);
    CT_ASSERT_STR_EQ(
        e.alg,
        "Build a 2x2x2 block, then extend to 2x2x3. After the 2x2x2 block there are 3 sides to "
        "extend on, explore all of them.");
    CT_ASSERT_STR_EQ(e.front_json, "{\"concept\":\"blockbuilding\"}");
    CT_ASSERT_STR_EQ(e.source_url,
                     "https://www.speedsolving.com/wiki/index.php/Fewest_Moves_techniques");
    e = fetch_one("db/seeds/packs/fmc_concepts.tsv", CT_EVENT_FMC, CT_METHOD_CONCEPT,
                  CT_STAGE_CONCEPT, "fmc.16", 16);
    CT_ASSERT_STR_EQ(
        e.alg,
        "Pseudo blocks have colors that do not match surrounding pieces. Non-matching blocks "
        "misalign centers only. Passive blockbuilding allows wrong orientation inside blocks. "
        "Domino blockbuilding restricts to 180 degree turns.");
    CT_ASSERT_STR_EQ(e.front_json, "{\"concept\":\"pseudo blockbuilding\"}");
    CT_ASSERT_STR_EQ(e.source_url, "https://www.speedsolving.com/wiki/index.php/Blockbuilding");
}

CT_SUITE(pack_seed_mbld) {
    verify_pack("db/seeds/packs/mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT,
                CT_STAGE_CONCEPT, 14);
    ct_pack_entry e = fetch_one("db/seeds/packs/mbld_concepts.tsv", CT_EVENT_MULTI_BLD,
                                CT_METHOD_CONCEPT, CT_STAGE_CONCEPT, "mbld.01", 14);
    CT_ASSERT_STR_EQ(
        e.alg,
        "Before the attempt submit the number of puzzles to attempt, at least 2, and that number "
        "cannot be changed afterwards.");
    CT_ASSERT_STR_EQ(e.front_json, "{\"rule\":\"H1a\"}");
    CT_ASSERT_STR_EQ(e.source_url, "https://www.worldcubeassociation.org/regulations/");
    e = fetch_one("db/seeds/packs/mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT,
                  CT_STAGE_CONCEPT, "mbld.07", 14);
    CT_ASSERT_STR_EQ(
        e.alg,
        "Assign each sticker one letter of a fixed scheme, Speffz is the standard, then memorize "
        "the cycle letters. A lone letter at the end signals parity.");
    CT_ASSERT_STR_EQ(e.front_json, "{\"memo\":\"lettering scheme\"}");
    e = fetch_one("db/seeds/packs/mbld_concepts.tsv", CT_EVENT_MULTI_BLD, CT_METHOD_CONCEPT,
                  CT_STAGE_CONCEPT, "mbld.14", 14);
    CT_ASSERT_STR_EQ(
        e.alg,
        "Person Action Object: each sticker maps to a person an action and an object, 6 letters "
        "compress to about 4 images, e.g. J K E T O R = a JoKer EaTing an OReo.");
    CT_ASSERT_STR_EQ(e.front_json, "{\"memo\":\"PAO\"}");
    CT_ASSERT_STR_EQ(e.source_url, "https://www.speedsolving.com/wiki/index.php/BLD_Memorisation");
}
