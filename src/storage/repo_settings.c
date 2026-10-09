#include "repo_settings.h"

#include "../config/config.h"

#include <string.h>

#define CT_SPEFFZ_SCHEME_NAME "speffz"

int ct_repo_settings_get(sqlite3 *db, const char *key, char *out, unsigned cap) {
    if (db == NULL || key == NULL || out == NULL || cap == 0)
        return -1;
    out[0] = '\0';
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, "SELECT value FROM settings WHERE key = ?", -1, &st, NULL) !=
        SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st);
    int ret = -1;
    if (rc == SQLITE_ROW) {
        const unsigned char *v = sqlite3_column_text(st, 0);
        size_t len = 0;
        if (v != NULL)
            len = strlen((const char *)v);
        if (len > (size_t)cap - 1)
            len = (size_t)cap - 1;
        if (len > 0)
            memcpy(out, v, len);
        out[len] = '\0';
        ret = 0;
    } else if (rc == SQLITE_DONE) {
        ret = 1;
    }
    sqlite3_finalize(st);
    return ret;
}

int ct_repo_settings_set(sqlite3 *db, const char *key, const char *value) {
    if (db == NULL || key == NULL || value == NULL)
        return -1;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO settings(key, value) VALUES(?, ?)"
                           " ON CONFLICT(key) DO UPDATE SET value = excluded.value",
                           -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    sqlite3_bind_text(st, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(st, 2, value, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? 0 : -1;
}

int ct_letter_scheme_create(sqlite3 *db, const char *name, int *id) {
    if (db == NULL || name == NULL || id == NULL)
        return -1;
    *id = 0;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, "INSERT INTO letter_scheme(name) VALUES(?)", -1, &st, NULL) !=
        SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    sqlite3_bind_text(st, 1, name, -1, SQLITE_STATIC);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE)
        return -1;
    *id = (int)sqlite3_last_insert_rowid(db);
    return 0;
}

int ct_letter_scheme_default_id(sqlite3 *db) {
    if (db == NULL)
        return -1;
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT id FROM letter_scheme WHERE is_default <> 0 ORDER BY id LIMIT 1",
                           -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    int rc = sqlite3_step(st);
    int ret = 0;
    if (rc == SQLITE_ROW) {
        ret = sqlite3_column_int(st, 0);
    } else if (rc != SQLITE_DONE) {
        ret = -1;
    }
    sqlite3_finalize(st);
    return ret;
}

int ct_letter_scheme_set_default(sqlite3 *db, int scheme_id) {
    if (db == NULL || scheme_id <= 0)
        return -1;
    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
        return -1;
    sqlite3_stmt *clear = NULL;
    sqlite3_stmt *mark = NULL;
    int ret = -1;
    if (sqlite3_prepare_v2(db, "UPDATE letter_scheme SET is_default = 0 WHERE is_default <> 0", -1,
                           &clear, NULL) == SQLITE_OK &&
        sqlite3_prepare_v2(db, "UPDATE letter_scheme SET is_default = 1 WHERE id = ?", -1, &mark,
                           NULL) == SQLITE_OK) {
        sqlite3_bind_int(mark, 1, scheme_id);
        if (sqlite3_step(clear) == SQLITE_DONE && sqlite3_step(mark) == SQLITE_DONE &&
            sqlite3_changes(db) > 0) {
            ret = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
        }
    }
    sqlite3_finalize(clear);
    sqlite3_finalize(mark);
    if (ret != 0)
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
    return ret;
}

int ct_letter_map_load(sqlite3 *db, int scheme_id, ct_letter_map *out) {
    if (db == NULL || out == NULL)
        return -1;
    memset(out, 0, sizeof *out);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, "SELECT orbit, sticker, letter FROM letter_map WHERE scheme_id = ?",
                           -1, &st, NULL) != SQLITE_OK) {
        sqlite3_finalize(st);
        return -1;
    }
    sqlite3_bind_int(st, 1, scheme_id);
    int count = 0;
    int rc;
    while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
        int orbit = sqlite3_column_int(st, 0);
        int sticker = sqlite3_column_int(st, 1);
        const unsigned char *letter = sqlite3_column_text(st, 2);
        if (letter == NULL || letter[0] == '\0' || letter[1] != '\0')
            continue;
        if (orbit != CT_ORBIT_CORNER && orbit != CT_ORBIT_EDGE)
            continue;
        if (sticker < 0 || sticker >= CT_LETTER_STICKERS)
            continue;
        if (orbit == CT_ORBIT_CORNER)
            out->corner[sticker] = (char)letter[0];
        else
            out->edge[sticker] = (char)letter[0];
        count++;
    }
    sqlite3_finalize(st);
    return rc == SQLITE_DONE ? count : -1;
}

int ct_letter_map_set(sqlite3 *db, int scheme_id, int orbit, int sticker, char letter) {
    if (db == NULL || scheme_id <= 0)
        return -1;
    if (orbit != CT_ORBIT_CORNER && orbit != CT_ORBIT_EDGE)
        return -1;
    if (sticker < 0 || sticker >= CT_LETTER_STICKERS)
        return -1;
    if (letter < CT_LETTER_FIRST || letter > CT_LETTER_LAST)
        return -1;
    sqlite3_stmt *find = NULL;
    sqlite3_stmt *upsert = NULL;
    int ret = -1;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM letter_scheme WHERE id = ?", -1, &find, NULL) ==
            SQLITE_OK &&
        sqlite3_prepare_v2(
            db,
            "INSERT INTO letter_map(scheme_id, orbit, sticker, letter) VALUES(?, ?, ?, ?)"
            " ON CONFLICT(scheme_id, orbit, sticker) DO UPDATE SET letter = excluded.letter",
            -1, &upsert, NULL) == SQLITE_OK) {
        sqlite3_bind_int(find, 1, scheme_id);
        int frc = sqlite3_step(find);
        if (frc == SQLITE_ROW) {
            char text[2] = {letter, '\0'};
            sqlite3_bind_int(upsert, 1, scheme_id);
            sqlite3_bind_int(upsert, 2, orbit);
            sqlite3_bind_int(upsert, 3, sticker);
            sqlite3_bind_text(upsert, 4, text, 1, SQLITE_TRANSIENT);
            ret = sqlite3_step(upsert) == SQLITE_DONE ? 0 : -1;
        } else {
            ret = -1;
        }
    }
    sqlite3_finalize(find);
    sqlite3_finalize(upsert);
    return ret;
}

int ct_letter_seed_speffz(sqlite3 *db) {
    if (db == NULL)
        return -1;
    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
        return -1;
    sqlite3_stmt *find = NULL;
    sqlite3_stmt *make = NULL;
    sqlite3_stmt *mark = NULL;
    sqlite3_stmt *upsert = NULL;
    int ret = -1;
    if (sqlite3_prepare_v2(db, "SELECT id FROM letter_scheme WHERE name = ?", -1, &find, NULL) !=
        SQLITE_OK) {
        goto done;
    }
    if (sqlite3_prepare_v2(db, "INSERT INTO letter_scheme(name, is_default) VALUES(?, 0)", -1,
                           &make, NULL) != SQLITE_OK) {
        goto done;
    }
    if (sqlite3_prepare_v2(db, "UPDATE letter_scheme SET is_default = 1 WHERE id = ?", -1, &mark,
                           NULL) != SQLITE_OK) {
        goto done;
    }
    if (sqlite3_prepare_v2(
            db,
            "INSERT INTO letter_map(scheme_id, orbit, sticker, letter) VALUES(?, ?, ?, ?)"
            " ON CONFLICT(scheme_id, orbit, sticker) DO NOTHING",
            -1, &upsert, NULL) != SQLITE_OK) {
        goto done;
    }
    sqlite3_bind_text(find, 1, CT_SPEFFZ_SCHEME_NAME, -1, SQLITE_STATIC);
    int rc = sqlite3_step(find);
    int scheme = 0;
    if (rc == SQLITE_ROW) {
        scheme = sqlite3_column_int(find, 0);
    } else if (rc == SQLITE_DONE) {
        sqlite3_bind_text(make, 1, CT_SPEFFZ_SCHEME_NAME, -1, SQLITE_STATIC);
        if (sqlite3_step(make) != SQLITE_DONE)
            goto done;
        scheme = (int)sqlite3_last_insert_rowid(db);
    } else {
        goto done;
    }
    int existing_default = ct_letter_scheme_default_id(db);
    if (existing_default < 0)
        goto done;
    if (existing_default == 0) {
        sqlite3_bind_int(mark, 1, scheme);
        if (sqlite3_step(mark) != SQLITE_DONE || sqlite3_changes(db) == 0)
            goto done;
    }
    for (int orbit = CT_ORBIT_CORNER; orbit <= CT_ORBIT_EDGE; orbit++) {
        for (int face = 0; face < CT_LETTER_FACES; face++) {
            for (int pos = 0; pos < CT_LETTER_PER_FACE; pos++) {
                int sticker = face * CT_LETTER_PER_FACE + pos;
                char text[2] = {(char)(CT_LETTER_FIRST + sticker), '\0'};
                sqlite3_reset(upsert);
                sqlite3_bind_int(upsert, 1, scheme);
                sqlite3_bind_int(upsert, 2, orbit);
                sqlite3_bind_int(upsert, 3, sticker);
                sqlite3_bind_text(upsert, 4, text, 1, SQLITE_TRANSIENT);
                if (sqlite3_step(upsert) != SQLITE_DONE)
                    goto done;
            }
        }
    }
    if (sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK)
        goto done;
    ret = 0;
done:
    sqlite3_finalize(find);
    sqlite3_finalize(make);
    sqlite3_finalize(mark);
    sqlite3_finalize(upsert);
    if (ret != 0)
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
    return ret;
}
