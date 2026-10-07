-- Enable foreign key support
PRAGMA foreign_keys = ON;

-- 1. Users Table (Donors, NGOs, Animal Shelters)
CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('DONOR', 'NGO', 'ANIMAL_SHELTER')),
    requirements TEXT DEFAULT 'any',
    allergies TEXT DEFAULT 'none',
    contact TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. Food Inventory Table
CREATE TABLE IF NOT EXISTS food_items (
    food_id INTEGER PRIMARY KEY AUTOINCREMENT,
    donor_id INTEGER NOT NULL,
    food_name TEXT NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity > 0),
    expiry_days INTEGER NOT NULL CHECK(expiry_days >= 0),
    allergens TEXT DEFAULT 'none',
    status TEXT DEFAULT 'AVAILABLE' CHECK(status IN ('AVAILABLE', 'RESCUED', 'EXPIRED')),
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (donor_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 3. Food Requests Table
CREATE TABLE IF NOT EXISTS food_requests (
    request_id INTEGER PRIMARY KEY AUTOINCREMENT,
    recipient_id INTEGER NOT NULL,
    food_needed TEXT NOT NULL,
    quantity_needed INTEGER NOT NULL CHECK(quantity_needed > 0),
    status TEXT DEFAULT 'PENDING' CHECK(status IN ('PENDING', 'MATCHED', 'CANCELLED')),
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (recipient_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 4. Allocations & Audit Trail Table
CREATE TABLE IF NOT EXISTS matches (
    match_id INTEGER PRIMARY KEY AUTOINCREMENT,
    donor_id INTEGER NOT NULL,
    recipient_id INTEGER NOT NULL,
    food_id INTEGER NOT NULL,
    request_id INTEGER NOT NULL,
    matched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (donor_id) REFERENCES users(user_id),
    FOREIGN KEY (recipient_id) REFERENCES users(user_id),
    FOREIGN KEY (food_id) REFERENCES food_items(food_id),
    FOREIGN KEY (request_id) REFERENCES food_requests(request_id)
);
