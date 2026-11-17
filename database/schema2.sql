-- schema2.sql
PRAGMA foreign_keys = ON;

-- 1. Users Table (Stores Donors, NGOs, and Animal Shelters)
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    role TEXT NOT NULL CHECK(role IN ('DONOR', 'NGO', 'SHELTER')),
    name TEXT NOT NULL,
    contact TEXT NOT NULL UNIQUE,
    address TEXT NOT NULL,
    extra_info TEXT DEFAULT '',
    penalty_points INTEGER DEFAULT 0,
    penalty_fines REAL DEFAULT 0.0,
    is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. Master Food Inventory Table (Linked List & Min-Heap Records)
CREATE TABLE IF NOT EXISTS food_items (
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

-- 3. Recipient Food Requests Table (FIFO Request Pipeline)
CREATE TABLE IF NOT EXISTS food_requests (
    request_id INTEGER PRIMARY KEY,
    recipient_id INTEGER NOT NULL,
    food_name TEXT NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity >= 0),
    allergies TEXT DEFAULT 'none',
    status TEXT DEFAULT 'Pending' CHECK(status IN ('Pending', 'Matched', 'Cancelled')),
    request_timestamp INTEGER NOT NULL,
    FOREIGN KEY (recipient_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 4. Order Transactions & Dispatch Ledger Table
CREATE TABLE IF NOT EXISTS orders (
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
    FOREIGN KEY (food_id) REFERENCES food_items(food_id),
    FOREIGN KEY (donor_id) REFERENCES users(id),
    FOREIGN KEY (recipient_id) REFERENCES users(id)
);

-- 5. Notifications & User Inbox Table
CREATE TABLE IF NOT EXISTS notifications (
    notif_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    message TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 6. Permanent ID Sequences
-- These counters preserve the highest ID ever issued, even if old records
-- are later deleted from food_items or food_requests.
CREATE TABLE IF NOT EXISTS id_sequences (
    entity TEXT PRIMARY KEY,
    next_id INTEGER NOT NULL CHECK(next_id > 0)
);
