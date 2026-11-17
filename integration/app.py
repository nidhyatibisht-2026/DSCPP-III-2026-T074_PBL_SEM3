"""FoodFlow Flask API. Mutations are handled by the C++ system; SQLite serves read views."""

from __future__ import annotations

import atexit
import os
import sqlite3
import threading
import time
from contextlib import contextmanager
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory

from cpp_bridge import CppBridge


ROOT = Path(__file__).resolve().parents[1]
DATABASE = ROOT / "database" / "food_surplus2.db"
FRONTEND = ROOT / "frontend"
os.chdir(ROOT)

app = Flask(__name__, static_folder=None)
cpp_lock = threading.RLock()


def connect_db():
    conn = sqlite3.connect(str(DATABASE), timeout=20.0)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA busy_timeout = 20000;")
    conn.execute("PRAGMA foreign_keys = ON;")
    return conn


@contextmanager
def db_connection():
    conn = connect_db()
    try:
        yield conn
    finally:
        conn.close()


# Set WAL once before the C++ system opens its connection.
_init_conn = sqlite3.connect(str(DATABASE), timeout=20.0)
try:
    _init_conn.execute("PRAGMA journal_mode = WAL;")
    _init_conn.execute("PRAGMA busy_timeout = 20000;")
finally:
    _init_conn.close()

cpp = CppBridge()
atexit.register(cpp.close)


def error(message, status=400):
    return jsonify({"ok": False, "error": str(message)}), status


def json_body():
    return request.get_json(silent=True) or {}


def as_int(value, field):
    try:
        return int(value)
    except (TypeError, ValueError):
        raise ValueError(f"{field} must be a valid integer")


def user_row(conn, user_id):
    return conn.execute("SELECT * FROM users WHERE id = ?", (user_id,)).fetchone()


@app.get("/")
def index():
    return send_from_directory(FRONTEND, "index.html")


@app.get("/<path:filename>")
def frontend_file(filename):
    return send_from_directory(FRONTEND, filename)


@app.get("/api/stats")
def api_stats():
    try:
        with cpp_lock:
            cpp.purge_expired()
        with db_connection() as conn:
            donations, servings = conn.execute(
                "SELECT COUNT(*), COALESCE(SUM(quantity), 0) FROM food_items "
                "WHERE is_assigned = 0 AND is_cancelled = 0 AND is_expired = 0"
            ).fetchone()
            open_requests = conn.execute(
                "SELECT COUNT(*) FROM food_requests WHERE status = 'Pending' AND quantity > 0"
            ).fetchone()[0]
            pickups = conn.execute("SELECT COUNT(*) FROM orders").fetchone()[0]
        return jsonify({
            "ok": True, "servings_donated": servings, "donations": donations,
            "open_requests": open_requests, "pickups": pickups,
        })
    except Exception as exc:
        app.logger.exception("Stats request failed")
        return error(exc, 500)


@app.route("/api/donations", methods=["GET", "POST"])
def api_donations():
    if request.method == "GET":
        try:
            with cpp_lock:
                cpp.purge_expired()
            with db_connection() as conn:
                rows = conn.execute("""
                    SELECT f.*, u.name AS donor_name
                    FROM food_items f
                    LEFT JOIN users u ON f.donor_id = u.id
                    WHERE f.is_assigned = 0 AND f.is_cancelled = 0 AND f.is_expired = 0
                    ORDER BY f.expiry_time ASC
                """).fetchall()
                now = int(time.time())
                items = []
                for row in rows:
                    remaining = max(0, int((row["expiry_time"] - now) / 60))
                    items.append({
                        "id": row["food_id"], "food_name": row["name"],
                        "quantity": row["quantity"],
                        "donor_name": row["donor_name"] or f"User #{row['donor_id']}",
                        "category": row["category"], "diet_type": row["diet_type"],
                        "remaining_mins": remaining, "critical": remaining <= 30,
                    })
            return jsonify({"ok": True, "items": items})
        except Exception as exc:
            app.logger.exception("Donation listing failed")
            return error(exc, 500)

    body = json_body()
    try:
        donor_id = as_int(body.get("donor_id"), "donor_id")
        quantity = as_int(body.get("quantity"), "quantity")
        shelf_hours = as_int(body.get("shelf_hours"), "shelf_hours")
        name = str(body.get("food_name") or "").strip()
        category = str(body.get("category") or "").strip()
        diet = str(body.get("diet_type") or "").strip()
        ingredients = str(body.get("ingredients") or "").strip()
        spice = str(body.get("spice_level") or "").strip()
        notes = str(body.get("notes") or "none").strip() or "none"
        if not all((name, category, diet, ingredients, spice)):
            return error("food_name, category, diet_type, ingredients and spice_level are required")
        if quantity <= 0 or shelf_hours <= 0:
            return error("quantity and shelf_hours must be greater than zero")
        with db_connection() as conn:
            donor = user_row(conn, donor_id)
            if not donor or donor["role"] != "DONOR":
                return error(f"Donor ID {donor_id} is not registered")
            if not donor["is_active"]:
                return error("This donor account is inactive")
        with cpp_lock:
            food_id = cpp.donate_food(
                donor_id, name, category, quantity, shelf_hours,
                diet, ingredients, spice, notes,
            )
        if food_id < 1:
            return error("The C++ system could not save this donation", 409)
        return jsonify({"ok": True, "food_id": food_id})
    except ValueError as exc:
        return error(exc)
    except Exception as exc:
        app.logger.exception("Donation creation failed")
        return error(exc, 500)


@app.route("/api/requests", methods=["GET", "POST"])
def api_requests():
    if request.method == "GET":
        try:
            with db_connection() as conn:
                rows = conn.execute("""
                    SELECT r.*, u.name AS recipient_name
                    FROM food_requests r
                    LEFT JOIN users u ON r.recipient_id = u.id
                    WHERE r.status = 'Pending' AND r.quantity > 0
                    ORDER BY r.request_id ASC
                """).fetchall()
                items = []
                for row in rows:
                    description = f"Looking for: {row['food_name']}"
                    allergies = (row["allergies"] or "").strip()
                    if allergies and allergies.lower() != "none":
                        description += f" | avoids: {allergies}"
                    items.append({
                        "id": row["request_id"],
                        "recipient_name": row["recipient_name"] or f"User #{row['recipient_id']}",
                        "food_name": row["food_name"], "quantity": row["quantity"],
                        "status": row["status"], "description": description,
                    })
            return jsonify({"ok": True, "items": items})
        except Exception as exc:
            app.logger.exception("Request listing failed")
            return error(exc, 500)

    body = json_body()
    try:
        recipient_id = as_int(body.get("recipient_id"), "recipient_id")
        quantity = as_int(body.get("quantity"), "quantity")
        food_name = str(body.get("food_name") or "").strip()
        allergies = str(body.get("allergies") or "none").strip() or "none"
        if not food_name:
            return error("food_name is required")
        if quantity <= 0:
            return error("quantity must be greater than zero")
        with db_connection() as conn:
            recipient = user_row(conn, recipient_id)
            if not recipient or recipient["role"] not in ("NGO", "SHELTER"):
                return error(f"Recipient ID {recipient_id} is not registered")
            if not recipient["is_active"]:
                return error("This recipient account is inactive")
        with cpp_lock:
            request_id = cpp.request_food(recipient_id, food_name, quantity, allergies)
        if request_id < 1:
            return error("The C++ system could not save this request", 409)
        return jsonify({"ok": True, "request_id": request_id})
    except ValueError as exc:
        return error(exc)
    except Exception as exc:
        app.logger.exception("Request creation failed")
        return error(exc, 500)


@app.get("/api/pickups")
def api_pickups():
    try:
        with db_connection() as conn:
            rows = conn.execute("""
                SELECT o.*, f.name AS food_name, f.donor_address AS address
                FROM orders o LEFT JOIN food_items f ON o.food_id = f.food_id
                ORDER BY o.order_id DESC
            """).fetchall()
            items = [{
                "order_id": row["order_id"],
                "food_name": row["food_name"] or f"Food #{row['food_id']}",
                "address": row["address"] or "", "quantity": row["quantity"],
                "fee": row["delivery_cost"], "status": row["status"],
                "donor_confirmed": bool(row["donor_confirmed"]),
                "recipient_confirmed": bool(row["recipient_confirmed"]),
            } for row in rows]
        return jsonify({"ok": True, "items": items})
    except Exception as exc:
        app.logger.exception("Pickup listing failed")
        return error(exc, 500)


@app.post("/api/pickups/confirm")
def api_pickups_confirm():
    body = json_body()
    try:
        order_id = as_int(body.get("order_id"), "order_id")
        user_id = as_int(body.get("user_id"), "user_id")
        role = str(body.get("role") or "").strip().lower()
        if role not in ("donor", "recipient"):
            return error("role must be donor or recipient")
        with db_connection() as conn:
            order = conn.execute("SELECT * FROM orders WHERE order_id = ?", (order_id,)).fetchone()
            if not order:
                return error("Order not found", 404)
            expected_id = order["donor_id"] if role == "donor" else order["recipient_id"]
            if expected_id != user_id:
                return error("User ID does not match this order and role", 403)
            if order["is_spoiled"]:
                return error("This order has an active spoiled-food complaint")
        with cpp_lock:
            if not cpp.confirm_delivery(order_id, user_id, role == "donor"):
                return error("Could not record delivery confirmation", 409)
        with db_connection() as conn:
            updated = conn.execute(
                "SELECT donor_confirmed, recipient_confirmed, status FROM orders WHERE order_id = ?",
                (order_id,),
            ).fetchone()
        return jsonify({"ok": True, "donor_confirmed": bool(updated[0]),
                        "recipient_confirmed": bool(updated[1]), "status": updated[2]})
    except ValueError as exc:
        return error(exc)
    except Exception as exc:
        app.logger.exception("Delivery confirmation failed")
        return error(exc, 500)


@app.post("/api/pickups/complain")
def api_pickups_complain():
    body = json_body()
    try:
        order_id = as_int(body.get("order_id"), "order_id")
        recipient_id = as_int(body.get("recipient_id"), "recipient_id")
        details = str(body.get("details") or "").strip()
        if not details:
            return error("Complaint details are required")
        with db_connection() as conn:
            order = conn.execute("SELECT * FROM orders WHERE order_id = ?", (order_id,)).fetchone()
            if not order or order["recipient_id"] != recipient_id:
                return error("Invalid order or recipient", 403)
            if order["is_spoiled"]:
                return error("A complaint has already been filed for this order")
        with cpp_lock:
            if not cpp.file_complaint(order_id, recipient_id, details):
                return error("Could not file complaint", 409)
        return jsonify({"ok": True, "message": "Spoiled-food complaint recorded; donor penalty updated."})
    except ValueError as exc:
        return error(exc)
    except Exception as exc:
        app.logger.exception("Complaint submission failed")
        return error(exc, 500)


@app.post("/api/register")
def api_register():
    body = json_body()
    role = str(body.get("role") or "").strip().lower()
    name = str(body.get("name") or "").strip()
    contact = str(body.get("contact") or "").strip()
    address = str(body.get("address") or "").strip()
    subtype = str(body.get("subtype") or "").strip()
    if role not in ("donor", "ngo", "shelter"):
        return error("role must be donor, ngo or shelter")
    if not all((name, contact, address, subtype)):
        return error("All registration fields are required")
    if len(contact) != 10 or not contact.isdigit():
        return error("Phone number must contain exactly 10 digits")
    try:
        with db_connection() as conn:
            duplicate = conn.execute(
                "SELECT 1 FROM users WHERE contact = ?", (contact,)
            ).fetchone()
            if duplicate:
                return error("Phone number is already registered", 409)
        with cpp_lock:
            user_id = cpp.register_user(role, name, contact, address, subtype)
        if user_id < 1:
            return error("Registration failed. Check the submitted details.", 409)
        return jsonify({"ok": True, "id": user_id, "role": role})
    except Exception as exc:
        app.logger.exception("Registration failed")
        return error(exc, 500)


@app.post("/api/donations/cancel")
def api_cancel_donation():
    body = json_body()
    try:
        food_id = as_int(body.get("food_id"), "food_id")
        donor_id = as_int(body.get("donor_id"), "donor_id")
        with cpp_lock:
            success = cpp.cancel_donation(food_id, donor_id)
        if not success:
            return error("Donation could not be cancelled. Check the food and donor IDs.", 409)
        return jsonify({"ok": True})
    except ValueError as exc:
        return error(exc)
    except Exception as exc:
        app.logger.exception("Donation cancellation failed")
        return error(exc, 500)


if __name__ == "__main__":
    print("FoodFlow is running at http://127.0.0.1:8080")
    print("Frontend -> Flask API -> C++ backend -> SQLite")
    app.run(host="127.0.0.1", port=8080, debug=False, threaded=True)
