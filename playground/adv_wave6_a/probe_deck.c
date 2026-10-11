#include <stdio.h>

#include <sqlite3.h>

#include "config/config.h"
#include "packs/packs.h"
#include "storage/db.h"
#include "trainer/selector.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: probe_deck <db>\n");
        return 2;
    }
    sqlite3 *db = NULL;
    remove(argv[1]);
    if (sqlite3_open(argv[1], &db) != SQLITE_OK || ct_db_migrate(db, "../../db/migrations") != 0) {
        fprintf(stderr, "db setup failed\n");
        return 1;
    }
    int l2e = ct_pack_seed_file(db, "../../db/seeds/packs/bigcube_l2e.tsv", CT_EVENT_5X5,
                                CT_METHOD_REDUCTION, CT_STAGE_PARITY);
    int par = ct_pack_seed_file(db, "../../db/seeds/packs/bigcube_parity.tsv", CT_EVENT_5X5,
                                CT_METHOD_REDUCTION, CT_STAGE_PARITY);
    printf("seeded l2e %d parity %d\n", l2e, par);
    ct_selector_list lists[8];
    int list_count = ct_selector_lists(CT_EVENT_5X5, CT_METHOD_REDUCTION, lists, 8);
    printf("lists %d stage[0] %d\n", list_count, lists[0].stage);
    int total = ct_selector_case_count(db, NULL, CT_EVENT_5X5, CT_METHOD_REDUCTION, 0);
    printf("case count %d\n", total);
    int counts[CT_SELECTOR_GROUPS_MAX] = {0};
    int group_count = 0;
    char key[64];
    for (int i = 0; i < total; i++) {
        int g = ct_selector_case_group(db, CT_EVENT_5X5, CT_METHOD_REDUCTION, 0, i);
        counts[g]++;
        if (g + 1 > group_count) {
            group_count = g + 1;
        }
        ct_selector_case_key(db, NULL, CT_EVENT_5X5, CT_METHOD_REDUCTION, 0, i, key, sizeof key);
        if (i < 3 || g != 0) {
            printf("  case %d key %s group %d %s\n", i, key, g, ct_selector_group_label(g));
        }
    }
    printf("group header shown: %s, counts:", group_count > 1 ? "yes" : "no");
    for (int g = 0; g < group_count; g++) {
        printf(" %s %d", ct_selector_group_label(g), counts[g]);
    }
    printf("\n");
    sqlite3_close(db);
    return 0;
}
