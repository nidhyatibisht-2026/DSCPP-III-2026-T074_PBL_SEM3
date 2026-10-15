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
    is_assigned INTEGER DEFAULT 0,
    is_cancelled INTEGER DEFAULT 0,
    is_expired INTEGER DEFAULT 0,
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
    quantity INTEGER NOT NULL,
    delivery_cost REAL NOT NULL,
    status TEXT NOT NULL,
    donor_confirmed INTEGER DEFAULT 0,
    recipient_confirmed INTEGER DEFAULT 0,
    is_spoiled INTEGER DEFAULT 0,
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
