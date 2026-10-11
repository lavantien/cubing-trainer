#include "config/config.h"
#include "packs/seed_registry.h"
#include "storage/db.h"
#include "trainer/selector.h"
#include "ui/ui.h"

#include <stdio.h>
#include <string.h>

static sqlite3 *open_db(void) {
    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK)
        return NULL;
    if (ct_db_migrate(db, "db/migrations") != CT_REPO_OK) {
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

int main(void) {
    sqlite3 *db = open_db();
    if (db == NULL) {
        printf("db open failed\n");
        return 1;
    }
    int failed = ct_pack_seed_registered(db, "db/seeds/packs");
    printf("registry_entries %d failed %d\n", ct_pack_seed_registry_count(), failed);

    long total = 0;
    for (int i = 0; i < ct_pack_seed_registry_count(); i++) {
        const ct_pack_seed_reg *r = &ct_pack_seed_registry[i];
        long n = ct_pack_count(db, r->event, r->method, r->stage);
        printf("reg %d %s ev %d m %d st %d rows %ld\n", i, r->file, (int)r->event, (int)r->method,
               (int)r->stage, n);
        total += n;
    }
    printf("event_pack_total %ld\n", total);

    ct_selector_scheme scheme;
    if (ct_selector_scheme_load(db, &scheme) != 0) {
        printf("scheme load failed\n");
        return 1;
    }
    printf("corner_buffer %d edge_buffer %d\n", scheme.buffer_corner, scheme.buffer_edge);

    ct_selector_list lists[8];
    int nlists = ct_selector_lists(CT_EVENT_3BLD, CT_METHOD_3STYLE, lists, 8);
    printf("bld3style_lists %d\n", nlists);
    int group_totals[CT_SELECTOR_GROUPS_MAX] = {0};
    for (int i = 0; i < nlists; i++) {
        int count = ct_selector_case_count(db, &scheme, CT_EVENT_3BLD, CT_METHOD_3STYLE, i);
        int groups[CT_SELECTOR_GROUPS_MAX] = {0};
        int gmax = 0;
        for (int k = 0; k < count; k++) {
            int g = ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, i, k);
            if (g < 0)
                continue;
            groups[g]++;
            if (g + 1 > gmax)
                gmax = g + 1;
        }
        printf("list %d stage %d source %d cases %d groups", i, (int)lists[i].stage,
               (int)lists[i].source, count);
        for (int g = 0; g < gmax; g++)
            printf(" %s=%d", ct_selector_group_label(g), groups[g]);
        printf("\n");
        if (lists[i].source == CT_SELECTOR_PACK) {
            for (int g = 0; g < CT_SELECTOR_GROUPS_MAX; g++)
                group_totals[g] += groups[g];
        }
    }

    ct_pack_entry e;
    int pack_count = ct_selector_case_count(db, &scheme, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2);
    if (ct_selector_pack_case(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, 0, &e) == 0)
        printf("pack_first %s\n", e.case_key);
    if (ct_selector_pack_case(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, pack_count - 1, &e) == 0)
        printf("pack_last %s\n", e.case_key);
    printf("pack_index_eq_count %s\n",
           ct_selector_pack_case(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, pack_count, &e) == 0
               ? "ACCEPTED"
               : "rejected");
    printf("pack_index_neg %s\n",
           ct_selector_pack_case(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, -1, &e) == 0
               ? "ACCEPTED"
               : "rejected");
    printf("group_on_chart_list %d\n",
           ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 0, 0));
    printf("group_on_pack_out_of_range %d\n",
           ct_selector_case_group(db, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2, pack_count));

    int yau_lists = ct_selector_lists(CT_EVENT_4X4, CT_METHOD_YAU, lists, 8);
    for (int i = 0; i < yau_lists; i++) {
        int count = ct_selector_case_count(db, &scheme, CT_EVENT_4X4, CT_METHOD_YAU, i);
        int groups[CT_SELECTOR_GROUPS_MAX] = {0};
        int gmax = 0;
        for (int k = 0; k < count; k++) {
            int g = ct_selector_case_group(db, CT_EVENT_4X4, CT_METHOD_YAU, i, k);
            if (g < 0)
                continue;
            groups[g]++;
            if (g + 1 > gmax)
                gmax = g + 1;
        }
        printf("yau_list %d cases %d gmax %d", i, count, gmax);
        for (int g = 0; g < gmax; g++)
            printf(" %s=%d", ct_selector_group_label(g), groups[g]);
        printf("\n");
        if (gmax > 1) {
            printf("yau_list %d renders as: list %s cases %d |", i,
                   "parity", count);
            for (int g = 0; g < gmax; g++)
                printf(" %s %d", ct_selector_group_label(g), groups[g]);
            printf("\n");
        }
    }

    sqlite3_close(db);

    sqlite3 *empty = open_db();
    if (empty == NULL) {
        printf("empty db open failed\n");
        return 1;
    }
    if (ct_letter_seed_speffz(empty) != 0) {
        printf("speffz seed failed\n");
        return 1;
    }
    ct_selector_scheme es;
    ct_selector_scheme_load(empty, &es);
    int parity_cases = ct_selector_case_count(empty, &es, CT_EVENT_3BLD, CT_METHOD_3STYLE, 2);
    printf("empty_db_parity_cases %d\n", parity_cases);
    ct_trainer_form tf;
    if (ct_trainer_form_open(&tf, empty) == CT_REPO_OK) {
        ct_trainer_form_select_event(&tf, CT_EVENT_3BLD);
        ct_trainer_form_select_method(&tf, CT_METHOD_3STYLE);
        ct_trainer_form_select_list(&tf, 2);
        printf("empty_form_lists %d empty_form_cases %d empty_group_count %d\n", tf.list_count,
               tf.case_count, tf.group_count);
        int rc = ct_trainer_form_select_case(&tf, 0);
        printf("empty_select_case rc %d status %d\n", rc, (int)tf.status);
    }
    sqlite3_close(empty);
    return 0;
}
