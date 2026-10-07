import sqlite3
import os

DB_FILE = "food_surplus.db"
SCHEMA_FILE = "schema.sql"

class FoodSurplusDBMS:
    def __init__(self, db_path=DB_FILE):
        self.db_path = db_path
        self.init_database()

    def get_connection(self):
        """Creates a database connection with foreign key support enabled."""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row # Access columns by name: row['food_name']
        conn.execute("PRAGMA foreign_keys = ON;")
        return conn

    def init_database(self):
        """Initializes tables using schema.sql if they do not exist."""
        if not os.path.exists(SCHEMA_FILE):
            raise FileNotFoundError(f"Missing '{SCHEMA_FILE}' in the project directory.")
        
        conn = self.get_connection()
        with open(SCHEMA_FILE, "r") as f:
            conn.executescript(f.read())
        conn.commit()
        conn.close()
        print(f"[DBMS SUCCESS] Database '{self.db_path}' initialized with relational schema.")

    # ------------------ 1. USER REGISTRATION ------------------
    def register_user(self, name, role, requirements="any", allergies="none", contact="9999999999"):
        """Registers a Donor, NGO, or Animal Shelter."""
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            INSERT INTO users (name, role, requirements, allergies, contact)
            VALUES (?, ?, ?, ?, ?);
        """
        cursor.execute(query, (name, role.upper(), requirements, allergies.lower(), contact))
        conn.commit()
        user_id = cursor.lastrowid
        conn.close()
        print(f"[DBMS] Registered {role} '{name}' with User ID #{user_id}")
        return user_id

    # ------------------ 2. FOOD INVENTORY REGISTRATION ------------------
    def add_food_item(self, donor_id, food_name, quantity, expiry_days, allergens="none"):
        """Allows a donor to register surplus food."""
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            INSERT INTO food_items (donor_id, food_name, quantity, expiry_days, allergens)
            VALUES (?, ?, ?, ?, ?);
        """
        cursor.execute(query, (donor_id, food_name, quantity, expiry_days, allergens.lower()))
        conn.commit()
        food_id = cursor.lastrowid
        conn.close()
        print(f"[DBMS] Added Food Item '{food_name}' (ID #{food_id}) by Donor #{donor_id}")
        return food_id

    # ------------------ 3. FOOD REQUEST REGISTRATION ------------------
    def add_food_request(self, recipient_id, food_needed, quantity_needed):
        """Allows an NGO or Shelter to submit a food demand."""
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            INSERT INTO food_requests (recipient_id, food_needed, quantity_needed)
            VALUES (?, ?, ?);
        """
        cursor.execute(query, (recipient_id, food_needed, quantity_needed))
        conn.commit()
        request_id = cursor.lastrowid
        conn.close()
        print(f"[DBMS] Submitted Request '{food_needed}' (ID #{request_id}) by Recipient #{recipient_id}")
        return request_id

    # ------------------ 4. QUERIES (MIN-HEAP & FIFO LOGIC) ------------------
    def get_urgent_inventory(self):
        """
        Retrieves active available food sorted by nearest expiry first.
        SQL counterpart of the C++ Min-Heap Priority Queue.
        """
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            SELECT f.food_id, f.food_name, f.quantity, f.expiry_days, f.allergens, 
                   u.name AS donor_name, f.status
            FROM food_items f
            JOIN users u ON f.donor_id = u.user_id
            WHERE f.status = 'AVAILABLE'
            ORDER BY f.expiry_days ASC;
        """
        cursor.execute(query)
        rows = [dict(row) for row in cursor.fetchall()]
        conn.close()
        return rows

    def get_pending_requests(self):
        """
        Retrieves pending requests in chronological order.
        SQL counterpart of the C++ FIFO Queue.
        """
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            SELECT r.request_id, r.recipient_id, r.food_needed, r.quantity_needed, 
                   u.name AS recipient_name, u.role, u.allergies
            FROM food_requests r
            JOIN users u ON r.recipient_id = u.user_id
            WHERE r.status = 'PENDING'
            ORDER BY r.request_id ASC;
        """
        cursor.execute(query)
        rows = [dict(row) for row in cursor.fetchall()]
        conn.close()
        return rows

    # ------------------ 5. TRANSACTION (ALLOCATION & AUDIT) ------------------
    def execute_match_transaction(self, donor_id, recipient_id, food_id, request_id):
        """
        Executes an ACID-compliant transaction:
        1. Logs the match in the matches table
        2. Sets food item status to 'RESCUED'
        3. Sets request status to 'MATCHED'
        Rolls back completely if any step fails.
        """
        conn = self.get_connection()
        cursor = conn.cursor()
        try:
            cursor.execute("BEGIN TRANSACTION;")

            cursor.execute("""
                INSERT INTO matches (donor_id, recipient_id, food_id, request_id)
                VALUES (?, ?, ?, ?);
            """, (donor_id, recipient_id, food_id, request_id))

            cursor.execute("""
                UPDATE food_items 
                SET status = 'RESCUED' 
                WHERE food_id = ?;
            """, (food_id,))

            cursor.execute("""
                UPDATE food_requests 
                SET status = 'MATCHED' 
                WHERE request_id = ?;
            """, (request_id,))

            conn.commit()
            print(f"[DBMS TRANSACTION SUCCESS] Food #{food_id} assigned to Request #{request_id}.")
            return True
        except Exception as e:
            conn.rollback()
            print(f"[DBMS TRANSACTION FAILED] Error: {e}. Changes rolled back.")
            return False
        finally:
            conn.close()

    # ------------------ 6. AUDIT TRAIL QUERY ------------------
    def get_audit_trail(self):
        """Performs a 4-table relational JOIN to display complete match history."""
        conn = self.get_connection()
        cursor = conn.cursor()
        query = """
            SELECT m.match_id, 
                   d.name AS donor_name, 
                   r.name AS recipient_name, 
                   f.food_name, 
                   f.quantity,
                   m.matched_at
            FROM matches m
            JOIN users d ON m.donor_id = d.user_id
            JOIN users r ON m.recipient_id = r.user_id
            JOIN food_items f ON m.food_id = f.food_id
            ORDER BY m.match_id DESC;
        """
        cursor.execute(query)
        rows = [dict(row) for row in cursor.fetchall()]
        conn.close()
        return rows


# =====================================================================
# TEST BENCH / EXECUTION DEMO (Runs automatically when you press F5)
# =====================================================================
if __name__ == "__main__":
    print("=" * 60)
    print(" FOOD SURPLUS MANAGEMENT & REDISTRIBUTION SYSTEM - DBMS")
    print("=" * 60)

    # Initialize DBMS
    db = FoodSurplusDBMS()

    # Seed Sample Users
    d1 = db.register_user("Royal Orchid Banquet Hall", "DONOR", contact="9876543210")
    d2 = db.register_user("Spice Route Bistro", "DONOR", contact="9812345678")
    r1 = db.register_user("Sneha Care Home", "NGO", allergies="peanuts", contact="9123456780")
    r2 = db.register_user("Happy Tails Sanctuary", "ANIMAL_SHELTER", contact="9000011111")

    # Seed Sample Food Items
    f1 = db.add_food_item(d1, "Vegetable Pulao & Dal", quantity=60, expiry_days=1, allergens="none")
    f2 = db.add_food_item(d1, "Kheer Dessert", quantity=30, expiry_days=2, allergens="dairy,nuts")
    f3 = db.add_food_item(d2, "Steamed White Rice", quantity=40, expiry_days=3, allergens="none")

    # Seed Sample Recipient Requests
    req1 = db.add_food_request(r1, "Cooked Vegetarian Lunch", quantity_needed=50)
    req2 = db.add_food_request(r2, "Boiled Rice without spices", quantity_needed=30)

    # Demonstrate Urgency Query (Min-Heap in SQL)
    print("\n" + "-" * 50)
    print("QUERY: Active Inventory (Sorted by Urgency / Expiry):")
    print("-" * 50)
    for item in db.get_urgent_inventory():
        print(f" -> [Expires in {item['expiry_days']}d] Food #{item['food_id']}: {item['food_name']} "
              f"({item['quantity']} portions) | Donor: {item['donor_name']} | Allergens: {item['allergens']}")

    # Demonstrate ACID Match Transaction
    print("\n" + "-" * 50)
    print("TRANSACTION: Executing Safe Allocation (Matching f1 with req1):")
    print("-" * 50)
    db.execute_match_transaction(donor_id=d1, recipient_id=r1, food_id=f1, request_id=req1)

    # Demonstrate Audit Trail Query (JOIN across tables)
    print("\n" + "-" * 50)
    print("QUERY: Distribution Audit Trail (Matched History):")
    print("-" * 50)
    for log in db.get_audit_trail():
        print(f" -> Match #{log['match_id']}: {log['food_name']} ({log['quantity']} portions) "
              f"from '{log['donor_name']}' -> allocated to '{log['recipient_name']}' at {log['matched_at']}")

    print("\n[DBMS FINISHED] All operations completed. Inspect 'food_surplus.db' in DB Browser!")
