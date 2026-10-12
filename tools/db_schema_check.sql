WITH want(name) AS (
    VALUES ('settings'), ('letter_scheme'), ('letter_map'), ('flashcard'), ('srs_state'),
           ('review_log'), ('generated_alg'), ('session_log'), ('event_pack'), ('memo_pair')
)
SELECT CASE
           WHEN EXISTS (SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = want.name)
           THEN 'ok table ' || want.name
           ELSE 'FAIL missing table ' || want.name
       END
FROM want;

WITH want(tbl, col) AS (
    VALUES ('settings', 'key'),
           ('letter_scheme', 'id'), ('letter_scheme', 'name'),
           ('letter_map', 'scheme_id'), ('letter_map', 'orbit'), ('letter_map', 'sticker'),
           ('flashcard', 'id'), ('flashcard', 'deck'), ('flashcard', 'case_key'),
           ('flashcard', 'direction'),
           ('srs_state', 'card_id'),
           ('review_log', 'id'), ('review_log', 'card_id'),
           ('generated_alg', 'id'), ('generated_alg', 'orbit'), ('generated_alg', 'target_pair'),
           ('generated_alg', 'buffer_mode'), ('generated_alg', 'category'),
           ('generated_alg', 'params_hash'),
           ('session_log', 'id'), ('session_log', 'deck'),
           ('event_pack', 'id'), ('event_pack', 'event'), ('event_pack', 'method'),
           ('event_pack', 'stage'), ('event_pack', 'case_key'),
           ('memo_pair', 'orbit'), ('memo_pair', 'pair'), ('memo_pair', 'phoneme'),
           ('memo_pair', 'image'), ('memo_pair', 'updated_at')
)
SELECT CASE
           WHEN EXISTS (SELECT 1 FROM pragma_table_info(want.tbl) WHERE name = want.col)
           THEN 'ok column ' || want.tbl || '.' || want.col
           ELSE 'FAIL missing column ' || want.tbl || '.' || want.col
       END
FROM want;

WITH want(tbl, col, type) AS (
    VALUES ('memo_pair', 'orbit', 'INTEGER'), ('memo_pair', 'pair', 'TEXT'),
           ('memo_pair', 'phoneme', 'TEXT'), ('memo_pair', 'image', 'TEXT'),
           ('memo_pair', 'updated_at', 'TEXT')
)
SELECT CASE
           WHEN EXISTS (SELECT 1 FROM pragma_table_info(want.tbl)
                        WHERE name = want.col AND type = want.type)
           THEN 'ok column type ' || want.tbl || '.' || want.col || ' ' || want.type
           ELSE 'FAIL missing column type ' || want.tbl || '.' || want.col || ' ' || want.type
       END
FROM want;

SELECT CASE
           WHEN (SELECT count(*) FROM pragma_table_info('memo_pair') WHERE pk > 0) = 2
                AND (SELECT sum(pk) FROM pragma_table_info('memo_pair')) = 3
           THEN 'ok memo_pair primary key orbit pair'
           ELSE 'FAIL memo_pair primary key'
       END;

WITH want(name) AS (
    VALUES ('idx_flashcard_deck'), ('idx_review_log_card'), ('idx_srs_state_due'),
           ('idx_generated_alg_pair'), ('idx_event_pack_ems')
)
SELECT CASE
           WHEN EXISTS (SELECT 1 FROM sqlite_master WHERE type = 'index' AND name = want.name)
           THEN 'ok index ' || want.name
           ELSE 'FAIL missing index ' || want.name
       END
FROM want;

SELECT CASE
           WHEN (SELECT count(*) FROM sqlite_master
                 WHERE type = 'table' AND name NOT LIKE 'sqlite_%') = 10
           THEN 'ok table count 10'
           ELSE 'FAIL unexpected table count'
       END;

SELECT CASE
           WHEN (SELECT count(*) FROM sqlite_master
                 WHERE type = 'index' AND name NOT LIKE 'sqlite_%') = 5
           THEN 'ok index count 5'
           ELSE 'FAIL unexpected index count'
       END;
