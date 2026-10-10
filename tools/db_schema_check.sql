WITH want(name) AS (
    VALUES ('settings'), ('letter_scheme'), ('letter_map'), ('flashcard'), ('srs_state'),
           ('review_log'), ('generated_alg'), ('session_log'), ('event_pack')
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
           ('event_pack', 'stage'), ('event_pack', 'case_key')
)
SELECT CASE
           WHEN EXISTS (SELECT 1 FROM pragma_table_info(want.tbl) WHERE name = want.col)
           THEN 'ok column ' || want.tbl || '.' || want.col
           ELSE 'FAIL missing column ' || want.tbl || '.' || want.col
       END
FROM want;

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
                 WHERE type = 'table' AND name NOT LIKE 'sqlite_%') = 9
           THEN 'ok table count 9'
           ELSE 'FAIL unexpected table count'
       END;

SELECT CASE
           WHEN (SELECT count(*) FROM sqlite_master
                 WHERE type = 'index' AND name NOT LIKE 'sqlite_%') = 5
           THEN 'ok index count 5'
           ELSE 'FAIL unexpected index count'
       END;
