# app.py — Python Flask API Bridge (Frontend <-> phase2.cpp/exe <-> SQLite DBMS)
from flask import Flask, request, jsonify, send_from_directory
import sqlite3
import subprocess
import time
import os
from db_manage2 import FoodSurplusDBMS

app = Flask(__name__, static_folder=".")
dbms = FoodSurplusDBMS("food_surplus2.db")

# Option 1: Configured directly for your laptop's phase2.cpp and phase2.exe
CPP_SOURCE = "phase2.cpp"
CPP_EXE = "phase2.exe" if os.name == "nt" else "./phase2"

def ensure_cpp_compiled():
    """Uses existing phase2.exe or compiles phase2.cpp if phase2.exe is missing."""
    exe_clean = "phase2.exe" if os.name == "nt" else "phase2"
    if os.path.exists(CPP_SOURCE) and not os.path.exists(exe_clean):
        try:
            subprocess.run(["g++", "-std=c++17", CPP_SOURCE, "-o", exe_clean], check=True)
            print(f"[Flask-C++ Bridge] Compiled {CPP_SOURCE} -> {exe_clean}")
        except Exception as e:
            print(f"[Flask-C++ Bridge] Note: g++ auto-compile skipped ({e})")

def invoke_cpp_engine(stdin_payload):
    """
    Feeds form inputs directly into phase2.exe with a 3-second timeout guard
    so the process never hangs.
    """
    exe_clean = "phase2.exe" if os.name == "nt" else "phase2"
    if not os.path.exists(exe_clean):
        return ""
    try:
        res = subprocess.run(
            [CPP_EXE],
            input=stdin_payload,
            text=True,
            capture_output=True,
            timeout=3
        )
        return res.stdout
    except Exception:
        return ""

# Sync existing users between C++ (users.dat) and SQLite (food_surplus2.db) on startup
ensure_cpp_compiled()
dbms.sync_from_cpp()
dbms.sync_to_cpp()

# Hash Table Allergen Dictionary (mirrors phase2.cpp)
ALLERGEN_MAP = {
    "milk": "Dairy", "milk solids": "Dairy", "cheese": "Dairy",
    "yogurt": "Dairy", "butter": "Dairy", "paneer": "Dairy",
    "cashew": "Tree Nuts", "almond": "Tree Nuts", "walnut": "Tree Nuts",
    "peanut": "Peanuts", "peanuts": "Peanuts", "peanut butter": "Peanuts",
    "wheat": "Gluten", "maida": "Gluten", "semolina": "Gluten", "bread": "Gluten",
    "egg": "Egg", "soy": "Soy", "soy sauce": "Soy", "tofu": "Soy"
}
TOXIC_LIST = ["onion", "garlic", "chocolate", "xylitol", "chili", "pepper", "raisin", "grape"]

def get_safe_connection():
    """Returns a SQLite connection with a 10s lock timeout and Foreign Keys enabled."""
    conn = sqlite3.connect("food_surplus2.db", timeout=10.0)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON;")
    return conn

def detect_allergens(ingredients_str):
    tokens = [t.strip().lower() for t in ingredients_str.split(",") if t.strip() and t.strip().lower() != "none"]
    detected = []
    for tok in tokens:
        if tok in ALLERGEN_MAP and ALLERGEN_MAP[tok] not in detected:
            detected.append(ALLERGEN_MAP[tok])
    return ", ".join(detected) if detected else "none"

def auto_purge_expired(conn):
    now = int(time.time())
    cur = conn.cursor()
    cur.execute("""
        SELECT food_id, name, donor_id FROM food_items
        WHERE is_assigned = 0 AND is_cancelled = 0 AND is_expired = 0 AND expiry_time <= ?
    """, (now,))
    for r in cur.fetchall():
        cur.execute("UPDATE food_items SET is_expired = 1 WHERE food_id = ?", (r["food_id"],))
        msg = f"[AUTOMATIC EXPIRY NOTICE] Donation Food #{r['food_id']} ({r['name']}) expired without recipient requests."
        cur.execute("INSERT INTO notifications (user_id, message, timestamp) VALUES (?, ?, ?)",
                    (r["donor_id"], msg, now))
    conn.commit()

def run_matching_pipeline(conn):
    """
    Executes the deterministic matching pipeline across SQLite & C++ rules:
    - FIFO Queue order for pending recipient requests (ORDER BY request_id ASC)
    - Min-Heap Urgency order for active food items (ORDER BY expiry_time ASC)
    - Polymorphic dietary & toxicity screening + Hash Table allergen filtering
    """
    auto_purge_expired(conn)
    cur = conn.cursor()
    now = int(time.time())

    cur.execute("SELECT * FROM food_requests WHERE status = 'Pending' AND quantity > 0 ORDER BY request_id ASC")
    requests = cur.fetchall()

    for req in requests:
        cur.execute("SELECT * FROM users WHERE id = ?", (req["recipient_id"],))
        rec = cur.fetchone()
        if not rec:
            continue

        cur.execute("""
            SELECT * FROM food_items
            WHERE is_assigned = 0 AND is_cancelled = 0 AND is_expired = 0 AND expiry_time > ?
            ORDER BY expiry_time ASC
        """, (now,))
        foods = cur.fetchall()

        compatible_lots = []
        for food in foods:
            if food["donor_id"] == rec["id"]:
                continue
            if food["name"].lower() not in req["food_name"].lower() and req["food_name"].lower() not in food["name"].lower():
                continue
            if rec["role"] == "NGO" and rec["extra_info"].strip().lower() == "veg" and food["diet_type"].lower() == "non-veg":
                continue
            if rec["role"] == "SHELTER":
                if food["diet_type"].lower() != "plain":
                    continue
                if any(tox in food["ingredients"].lower() for tox in TOXIC_LIST):
                    continue

            req_alg = [a.strip().lower() for a in (req["allergies"] or "").split(",") if a.strip() and a.strip().lower() != "none"]
            food_alg = [a.strip().lower() for a in (food["allergens"] or "").split(",") if a.strip() and a.strip().lower() != "none"]
            conflict = False
            for p in food_alg:
                for rst in req_alg:
                    if p == rst or p == rst + "s" or rst == p + "s":
                        conflict = True
                        break
            if not conflict:
                compatible_lots.append(food)

        if not compatible_lots:
            continue

        needed = req["quantity"]
        for food in compatible_lots:
            if needed <= 0:
                break
            take = min(food["quantity"], needed)
            needed -= take

            if food["quantity"] == take:
                cur.execute("UPDATE food_items SET is_assigned = 1 WHERE food_id = ?", (food["food_id"],))
            else:
                cur.execute("UPDATE food_items SET quantity = quantity - ? WHERE food_id = ?", (take, food["food_id"]))

            cur.execute("SELECT COALESCE(MAX(order_id), 0) + 1 FROM orders")
            next_oid = cur.fetchone()[0]

            cur.execute("""
                INSERT INTO orders (order_id, food_id, donor_id, recipient_id, quantity, delivery_cost, status)
                VALUES (?, ?, ?, ?, ?, 30.0, 'Dispatched')
            """, (next_oid, food["food_id"], food["donor_id"], rec["id"], take))

            cur.execute("INSERT INTO notifications (user_id, message, timestamp) VALUES (?, ?, ?)",
                        (food["donor_id"], f"Order #{next_oid}: Food #{food['food_id']} ({take} servings) matched to {rec['name']}.", now))
            cur.execute("INSERT INTO notifications (user_id, message, timestamp) VALUES (?, ?, ?)",
                        (rec["id"], f"Order #{next_oid}: {take} servings of {food['name']} from Donor #{food['donor_id']} dispatched.", now))

        if needed == 0:
            cur.execute("UPDATE food_requests SET status = 'Matched', quantity = 0 WHERE request_id = ?", (req["request_id"],))
        else:
            cur.execute("UPDATE food_requests SET quantity = ? WHERE request_id = ?", (needed, req["request_id"]))

    conn.commit()

# CORS headers so browser fetch() calls from index.html always succeed
@app.after_request
def add_cors(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    return response

# Serve Frontend UI at http://localhost:8080/
@app.route("/")
def serve_index():
    return send_from_directory(".", "index.html")

@app.route("/<path:path>")
def serve_static(path):
    return send_from_directory(".", path)

# -------------------- 1. DASHBOARD STATS --------------------
@app.route("/api/stats", methods=["GET"])
def api_stats():
    try:
        conn = get_safe_connection()
        auto_purge_expired(conn)
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*), COALESCE(SUM(quantity), 0) FROM food_items WHERE is_assigned = 0 AND is_cancelled = 0 AND is_expired = 0")
        donations, servings = cur.fetchone()
        cur.execute("SELECT COUNT(*) FROM food_requests WHERE status = 'Pending' AND quantity > 0")
        open_reqs = cur.fetchone()[0]
        cur.execute("SELECT COUNT(*) FROM orders")
        pickups = cur.fetchone()[0]
        conn.close()
        return jsonify({
            "ok": True,
            "servings_donated": servings,
            "donations": donations,
            "open_requests": open_reqs,
            "pickups": pickups
        })
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

# -------------------- 2. DONATIONS --------------------
@app.route("/api/donations", methods=["GET", "POST", "OPTIONS"])
def api_donations():
    if request.method == "OPTIONS":
        return ("", 204)
    try:
        conn = get_safe_connection()
        auto_purge_expired(conn)
        cur = conn.cursor()
        now = int(time.time())

        if request.method == "GET":
            cur.execute("""
                SELECT f.*, u.name as donor_name
                FROM food_items f
                LEFT JOIN users u ON f.donor_id = u.id
                WHERE f.is_assigned = 0 AND f.is_cancelled = 0 AND f.is_expired = 0
                ORDER BY f.expiry_time ASC
            """)
            items = []
            for r in cur.fetchall():
                rem_mins = max(0, int((r["expiry_time"] - now) / 60))
                items.append({
                    "id": r["food_id"],
                    "food_name": r["name"],
                    "quantity": r["quantity"],
                    "donor_name": r["donor_name"] or f"User #{r['donor_id']}",
                    "category": r["category"],
                    "diet_type": r["diet_type"],
                    "remaining_mins": rem_mins,
                    "critical": rem_mins <= 30
                })
            conn.close()
            return jsonify({"ok": True, "items": items})

        b = request.get_json(silent=True) or {}
        donor_id = int(b.get("donor_id") or 0)
        qty = int(b.get("quantity") or 0)
        hours = int(b.get("shelf_hours") or 0)
        name = (b.get("food_name") or "").strip()
        cat = (b.get("category") or "").strip()
        diet = (b.get("diet_type") or "").strip()
        ing = (b.get("ingredients") or "").strip()
        spice = (b.get("spice_level") or "").strip()
        notes = (b.get("notes") or "").strip() or "none"

        if not all([name, cat, diet, ing, spice]):
            conn.close()
            return jsonify({"ok": False, "error": "missing required fields: food_name, category, diet_type, ingredients, spice_level"})

        cur.execute("SELECT * FROM users WHERE id = ?", (donor_id,))
        donor = cur.fetchone()
        if not donor:
            conn.close()
            return jsonify({"ok": False, "error": f"Donor ID {donor_id} is not registered — use the Register page first"})
        if qty <= 0 or hours <= 0:
            conn.close()
            return jsonify({"ok": False, "error": "quantity and shelf_hours must be > 0"})

        # Pass to compiled C++ engine (phase2.exe Option 4: Donate Food Surplus, then 0: Exit)
        invoke_cpp_engine(f"4\n{donor_id}\n{name}\n{cat}\n{qty}\n{hours}\n{diet}\n{ing}\n{spice}\n{notes}\n0\n")

        allergens = detect_allergens(ing)
        expiry_time = now + (hours * 3600)

        cur.execute("SELECT COALESCE(MAX(food_id), 0) + 1 FROM food_items")
        next_fid = cur.fetchone()[0]

        cur.execute("""
            INSERT INTO food_items (food_id, name, category, quantity, prep_time, expiry_time, shelf_hours,
                                    diet_type, ingredients, allergens, spice_level, extra_notes, donor_id, donor_address)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (next_fid, name, cat, qty, now, expiry_time, hours, diet, ing, allergens, spice, notes, donor_id, donor["address"]))
        conn.commit()

        run_matching_pipeline(conn)
        conn.close()
        return jsonify({"ok": True, "food_id": next_fid})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

# -------------------- 3. FOOD REQUESTS --------------------
@app.route("/api/requests", methods=["GET", "POST", "OPTIONS"])
def api_requests():
    if request.method == "OPTIONS":
        return ("", 204)
    try:
        conn = get_safe_connection()
        auto_purge_expired(conn)
        cur = conn.cursor()

        if request.method == "GET":
            cur.execute("""
                SELECT r.*, u.name as recipient_name
                FROM food_requests r
                LEFT JOIN users u ON r.recipient_id = u.id
                WHERE r.status = 'Pending' AND r.quantity > 0
                ORDER BY r.request_id ASC
            """)
            items = []
            for r in cur.fetchall():
                desc = f"Looking for: {r['food_name']}"
                alg = (r["allergies"] or "").strip()
                if alg and alg.lower() != "none":
                    desc += f"  |  avoids: {alg}"
                items.append({
                    "id": r["request_id"],
                    "recipient_name": r["recipient_name"] or f"User #{r['recipient_id']}",
                    "food_name": r["food_name"],
                    "quantity": r["quantity"],
                    "status": r["status"],
                    "description": desc
                })
            conn.close()
            return jsonify({"ok": True, "items": items})

        b = request.get_json(silent=True) or {}
        rid = int(b.get("recipient_id") or 0)
        qty = int(b.get("quantity") or 0)
        food_name = (b.get("food_name") or "").strip()
        allergies = (b.get("allergies") or "none").strip() or "none"

        if not food_name:
            conn.close()
            return jsonify({"ok": False, "error": "food_name is required"})

        cur.execute("SELECT * FROM users WHERE id = ? AND role IN ('NGO', 'SHELTER')", (rid,))
        rec = cur.fetchone()
        if not rec:
            conn.close()
            return jsonify({"ok": False, "error": f"Recipient ID {rid} is not registered — register as an NGO or Animal Shelter first"})
        if qty <= 0:
            conn.close()
            return jsonify({"ok": False, "error": "quantity (servings) must be > 0"})

        # Pass to compiled C++ engine (phase2.exe Option 5: Submit Food Request, then 0: Exit)
        invoke_cpp_engine(f"5\n{rid}\n{food_name}\n{qty}\n{allergies}\n0\n")

        cur.execute("SELECT COALESCE(MAX(request_id), 0) + 1 FROM food_requests")
        next_req_id = cur.fetchone()[0]

        cur.execute("""
            INSERT INTO food_requests (request_id, recipient_id, food_name, quantity, allergies, status, request_timestamp)
            VALUES (?, ?, ?, ?, ?, 'Pending', ?)
        """, (next_req_id, rid, food_name, qty, allergies, int(time.time())))
        conn.commit()

        run_matching_pipeline(conn)
        conn.close()
        return jsonify({
            "ok": True,
            "request_id": next_req_id,
            "info": "Request queued — the matching engine runs automatically. Check Pickups for dispatched orders."
        })
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

# -------------------- 4. PICKUPS & TWO-WAY HANDSHAKE --------------------
@app.route("/api/pickups", methods=["GET"])
def api_pickups():
    try:
        conn = get_safe_connection()
        cur = conn.cursor()
        cur.execute("""
            SELECT o.*, f.name as food_name, f.donor_address as address
            FROM orders o
            LEFT JOIN food_items f ON o.food_id = f.food_id
            ORDER BY o.order_id DESC
        """)
        items = []
        for o in cur.fetchall():
            items.append({
                "order_id": o["order_id"],
                "food_name": o["food_name"] or f"Food #{o['food_id']}",
                "address": o["address"] or "",
                "quantity": o["quantity"],
                "fee": o["delivery_cost"],
                "status": o["status"],
                "donor_confirmed": bool(o["donor_confirmed"]),
                "recipient_confirmed": bool(o["recipient_confirmed"])
            })
        conn.close()
        return jsonify({"ok": True, "items": items})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

@app.route("/api/pickups/confirm", methods=["POST", "OPTIONS"])
def api_pickups_confirm():
    if request.method == "OPTIONS":
        return ("", 204)
    try:
        b = request.get_json(silent=True) or {}
        oid = int(b.get("order_id") or 0)
        uid = int(b.get("user_id") or 0)
        role = (b.get("role") or "").strip()

        if oid <= 0 or uid <= 0 or role not in ("donor", "recipient"):
            return jsonify({"ok": False, "error": "need order_id, user_id and role ('donor' or 'recipient')"})

        conn = get_safe_connection()
        cur = conn.cursor()
        cur.execute("SELECT * FROM orders WHERE order_id = ?", (oid,))
        ord_row = cur.fetchone()
        if not ord_row:
            conn.close()
            return jsonify({"ok": False, "error": "[Error] Order ID not found."})

        if role == "donor" and ord_row["donor_id"] == uid:
            cur.execute("UPDATE orders SET donor_confirmed = 1 WHERE order_id = ?", (oid,))
        elif role == "recipient" and ord_row["recipient_id"] == uid:
            cur.execute("UPDATE orders SET recipient_confirmed = 1 WHERE order_id = ?", (oid,))
        else:
            conn.close()
            return jsonify({"ok": False, "error": "[Error] User ID does not match order record."})
        conn.commit()

        cur.execute("SELECT * FROM orders WHERE order_id = ?", (oid,))
        updated = cur.fetchone()
        if updated["donor_confirmed"] and updated["recipient_confirmed"]:
            cur.execute("UPDATE orders SET status = 'Delivered/Fulfilled' WHERE order_id = ?", (oid,))
            conn.commit()

        conn.close()
        return jsonify({"ok": True})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

# -------------------- 5. SPOILED FOOD COMPLAINT (RS. 250 PENALTY) --------------------
@app.route("/api/pickups/complain", methods=["POST", "OPTIONS"])
def api_pickups_complain():
    if request.method == "OPTIONS":
        return ("", 204)
    try:
        b = request.get_json(silent=True) or {}
        oid = int(b.get("order_id") or 0)
        rid = int(b.get("recipient_id") or 0)
        details = (b.get("details") or "Spoiled food reported").strip()

        conn = get_safe_connection()
        cur = conn.cursor()
        cur.execute("SELECT * FROM orders WHERE order_id = ?", (oid,))
        ord_row = cur.fetchone()
        if not ord_row or ord_row["recipient_id"] != rid:
            conn.close()
            return jsonify({"ok": False, "error": "[Error] Invalid Order ID or Recipient ID."})
        if ord_row["is_spoiled"]:
            conn.close()
            return jsonify({"ok": False, "error": "[Error] Complaint already registered for this order."})

        cur.execute("UPDATE orders SET is_spoiled = 1, status = 'Complaint: Spoiled Food' WHERE order_id = ?", (oid,))
        cur.execute("UPDATE users SET penalty_points = penalty_points + 1, penalty_fines = penalty_fines + 250.0 WHERE id = ?",
                    (ord_row["donor_id"],))
        conn.commit()
        conn.close()
        dbms.sync_to_cpp()
        return jsonify({"ok": True, "message": "Penalty of Rs. 250 and 1 strike imposed on Donor."})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

# -------------------- 6. USER REGISTRATION --------------------
@app.route("/api/register", methods=["POST", "OPTIONS"])
def api_register():
    if request.method == "OPTIONS":
        return ("", 204)
    try:
        b = request.get_json(silent=True) or {}
        role = (b.get("role") or "").strip().lower()
        name = (b.get("name") or "").strip()
        contact = (b.get("contact") or "").strip()
        addr = (b.get("address") or "").strip()
        subtype = (b.get("subtype") or "").strip()

        if not all([role, name, contact, addr, subtype]):
            return jsonify({"ok": False, "error": "missing fields: role, name, contact, address, subtype"})
        if len(contact) != 10 or not contact.isdigit():
            return jsonify({"ok": False, "error": "phone number must be exactly 10 digits"})
        if role not in ("donor", "ngo", "shelter"):
            return jsonify({"ok": False, "error": "role must be donor | ngo | shelter"})

        conn = get_safe_connection()
        cur = conn.cursor()
        cur.execute("SELECT 1 FROM users WHERE contact = ?", (contact,))
        if cur.fetchone():
            conn.close()
            return jsonify({"ok": False, "error": "this phone number is already registered"})

        cur.execute("SELECT COALESCE(MAX(id), 0) + 1 FROM users")
        next_id = cur.fetchone()[0]
        cur.execute("""
            INSERT INTO users (id, role, name, contact, address, extra_info, penalty_points, penalty_fines)
            VALUES (?, ?, ?, ?, ?, ?, 0, 0.0)
        """, (next_id, role.upper(), name, contact, addr, subtype))
        conn.commit()
        conn.close()

        # Sync updated SQLite user table immediately to users.dat for phase2.cpp
        dbms.sync_to_cpp()
        return jsonify({"ok": True, "id": next_id, "role": role})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})

if __name__ == "__main__":
    print("=================================================================")
    print(" FoodFlow 3-Tier System Online -> http://localhost:8080")
    print(" Connected: HTML/JS Frontend <-> Flask <-> phase2.exe <-> SQLite")
    print("=================================================================")
    app.run(host="0.0.0.0", port=8080, debug=True)
