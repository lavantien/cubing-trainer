#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "srs/fsrs.h"
#include "storage/db.h"
#include "storage/repo_cards.h"
#include "storage/repo_stats.h"

int main(void) {
    ct_config cfg = ct_config_defaults();
    ct_fsrs f;
    ct_fsrs_init(&f, &cfg);
    ct_fsrs_card card = {0};
    long day = 20000;
    int max_state = 0;
    int reviews = 0;
    for (int i = 0; i < 40; i++) {
        ct_fsrs_card next = ct_fsrs_review(&f, &card, i % 7 == 0 ? CT_AGAIN : CT_GOOD,
                                           (double)(day - card.last_review_day), day);
        card = next;
        if (card.state > max_state) {
            max_state = card.state;
        }
        day = card.due_day > day + 1 ? card.due_day : day + 1;
        reviews++;
    }
    printf("reviews=%d max_state_ever=%d final_state=%d\n", reviews, max_state, card.state);

    sqlite3 *db = NULL;
    if (ct_db_open(&db, ":memory:") != SQLITE_OK || ct_db_migrate(db, "db/migrations") != 0) {
        printf("open/migrate failed\n");
        return 1;
    }
    for (int i = 0; i < 3; i++) {
        ct_card c;
        memset(&c, 0, sizeof c);
        snprintf(c.deck, sizeof c.deck, "%s", "deck_a");
        snprintf(c.case_key, sizeof c.case_key, "k%d", i);
        if (ct_repo_card_insert(db, &c) != CT_REPO_OK) {
            printf("insert failed\n");
            return 1;
        }
        ct_srs_row row;
        memset(&row, 0, sizeof row);
        row.stability = card.stability;
        row.difficulty = card.difficulty;
        snprintf(row.due, sizeof row.due, "%s", "2026-01-01T00:00:00Z");
        row.reps = reviews;
        row.state = card.state;
        if (ct_repo_srs_put(db, c.id, &row) != CT_REPO_OK) {
            printf("srs put failed\n");
            return 1;
        }
    }
    ct_deck_stats decks[4];
    int n = ct_repo_deck_stats(db, "2027-01-01T00:00:00Z", decks, 4);
    printf("decks=%d total=%ld due=%ld learning=%ld review=%ld\n", n, decks[0].total_cards,
           decks[0].due_now, decks[0].learning, decks[0].review_state);
    ct_db_close(db);
    return 0;
}
