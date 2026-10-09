CREATE TABLE settings(
    key TEXT PRIMARY KEY,
    value TEXT
);

CREATE TABLE letter_scheme(
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    is_default INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE letter_map(
    scheme_id INTEGER NOT NULL,
    orbit INTEGER NOT NULL,
    sticker INTEGER NOT NULL,
    letter TEXT NOT NULL,
    PRIMARY KEY(scheme_id, orbit, sticker)
);

CREATE TABLE flashcard(
    id INTEGER PRIMARY KEY,
    deck TEXT NOT NULL,
    case_key TEXT NOT NULL,
    direction INTEGER NOT NULL DEFAULT 0,
    front_json TEXT NOT NULL DEFAULT '{}',
    alg_3x3 TEXT,
    alg_big TEXT,
    alg_oh TEXT,
    notes TEXT,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    UNIQUE(deck, case_key, direction)
);

CREATE TABLE srs_state(
    card_id INTEGER PRIMARY KEY REFERENCES flashcard(id) ON DELETE CASCADE,
    stability REAL NOT NULL,
    difficulty REAL NOT NULL,
    due TEXT NOT NULL,
    last_review TEXT,
    reps INTEGER NOT NULL DEFAULT 0,
    lapses INTEGER NOT NULL DEFAULT 0,
    state INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE review_log(
    id INTEGER PRIMARY KEY,
    card_id INTEGER NOT NULL REFERENCES flashcard(id) ON DELETE CASCADE,
    rating INTEGER NOT NULL,
    reviewed_at TEXT NOT NULL,
    stability REAL NOT NULL,
    difficulty REAL NOT NULL,
    elapsed_days REAL NOT NULL,
    scheduled_days REAL NOT NULL
);

CREATE TABLE generated_alg(
    id INTEGER PRIMARY KEY,
    orbit INTEGER NOT NULL,
    target_pair TEXT NOT NULL,
    buffer_mode INTEGER NOT NULL,
    category INTEGER NOT NULL,
    sequence TEXT NOT NULL,
    classification INTEGER NOT NULL,
    setup TEXT,
    interchange TEXT,
    insertion TEXT,
    qtm INTEGER NOT NULL,
    score REAL NOT NULL,
    score_parts_json TEXT,
    params_hash TEXT NOT NULL,
    created_at TEXT NOT NULL,
    UNIQUE(orbit, category, target_pair, buffer_mode, params_hash)
);

CREATE TABLE session_log(
    id INTEGER PRIMARY KEY,
    deck TEXT NOT NULL,
    started_at TEXT NOT NULL,
    ended_at TEXT,
    n_cards INTEGER NOT NULL DEFAULT 0,
    n_again INTEGER NOT NULL DEFAULT 0,
    n_hard INTEGER NOT NULL DEFAULT 0,
    n_good INTEGER NOT NULL DEFAULT 0,
    n_easy INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE event_pack(
    id INTEGER PRIMARY KEY,
    event INTEGER NOT NULL,
    method INTEGER NOT NULL,
    stage INTEGER NOT NULL,
    case_key TEXT NOT NULL,
    alg TEXT,
    front_json TEXT NOT NULL DEFAULT '{}',
    source_url TEXT,
    source_at TEXT,
    content_hash TEXT,
    UNIQUE(event, method, stage, case_key)
);

CREATE INDEX idx_flashcard_deck ON flashcard(deck);

CREATE INDEX idx_review_log_card ON review_log(card_id);

CREATE INDEX idx_srs_state_due ON srs_state(due);

CREATE INDEX idx_generated_alg_pair ON generated_alg(orbit, target_pair);

CREATE INDEX idx_event_pack_ems ON event_pack(event, method, stage);
