.bail on
PRAGMA foreign_keys = OFF;

BEGIN IMMEDIATE;

CREATE TABLE food_items_new (
    food_id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    category TEXT,
    quantity INTEGER NOT NULL CHECK(quantity >= 0),
    prep_time INTEGER NOT NULL,
    expiry_time INTEGER NOT NULL,
    shelf_hours INTEGER NOT NULL,
    diet_type TEXT,
    ingredients TEXT,
    allergens TEXT DEFAULT 'none',
    spice_level TEXT,
    extra_notes TEXT,
    donor_id INTEGER NOT NULL,
    donor_address TEXT,
    is_assigned INTEGER NOT NULL DEFAULT 0 CHECK(is_assigned IN (0,1)),
    is_cancelled INTEGER NOT NULL DEFAULT 0 CHECK(is_cancelled IN (0,1)),
    is_expired INTEGER NOT NULL DEFAULT 0 CHECK(is_expired IN (0,1)),
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (donor_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE orders_new (
    order_id INTEGER PRIMARY KEY,
    food_id INTEGER NOT NULL,
    donor_id INTEGER NOT NULL,
    recipient_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity > 0),
    delivery_cost REAL NOT NULL CHECK(delivery_cost >= 0),
    status TEXT NOT NULL CHECK(status IN (
        'Dispatched',
        'Delivered/Fulfilled',
        'Complaint: Spoiled Food'
    )),
    donor_confirmed INTEGER NOT NULL DEFAULT 0 CHECK(donor_confirmed IN (0,1)),
    recipient_confirmed INTEGER NOT NULL DEFAULT 0 CHECK(recipient_confirmed IN (0,1)),
    is_spoiled INTEGER NOT NULL DEFAULT 0 CHECK(is_spoiled IN (0,1)),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (food_id) REFERENCES food_items_new(food_id),
    FOREIGN KEY (donor_id) REFERENCES users(id),
    FOREIGN KEY (recipient_id) REFERENCES users(id)
);

INSERT INTO food_items_new (
    food_id, name, category, quantity, prep_time, expiry_time,
    shelf_hours, diet_type, ingredients, allergens, spice_level,
    extra_notes, donor_id, donor_address, is_assigned,
    is_cancelled, is_expired, logged_at
)
SELECT
    food_id, name, category, quantity, prep_time, expiry_time,
    shelf_hours, diet_type, ingredients, allergens, spice_level,
    extra_notes, donor_id, donor_address, is_assigned,
    is_cancelled, is_expired, logged_at
FROM food_items;

INSERT INTO orders_new (
    order_id, food_id, donor_id, recipient_id, quantity,
    delivery_cost, status, donor_confirmed, recipient_confirmed,
    is_spoiled, created_at
)
SELECT
    order_id, food_id, donor_id, recipient_id, quantity,
    delivery_cost, status, donor_confirmed, recipient_confirmed,
    is_spoiled, created_at
FROM orders;

DROP TABLE orders;
DROP TABLE food_items;

ALTER TABLE food_items_new RENAME TO food_items;
ALTER TABLE orders_new RENAME TO orders;

COMMIT;

PRAGMA foreign_keys = ON;
PRAGMA foreign_key_check;
