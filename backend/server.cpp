//======================================================================
// server.cpp  —  HTTP/JSON API wrapper around the EXISTING console
//                backend in 10.cpp  (10.cpp is NOT modified at all)
//
// BUILD  (put httplib.h and json.hpp in this same folder first):
//   Linux/macOS :  g++ -std=c++17 -O2 -pthread server.cpp -o server
//   Windows/MGW :  g++ -std=c++17 -O2 -pthread server.cpp -o server.exe -lws2_32
//
// RUN:
//   ./server     ->  API served at http://localhost:8080/api
//   (the frontend app.js already points at this URL)
//
// Single-header dependencies to download once:
//   https://github.com/yhirose/cpp-httplib  ->  httplib.h
//   https://github.com/nlohmann/json        ->  json.hpp (single_include)
//======================================================================

#include "httplib.h"
#include "json.hpp"

// 1. Pre-include every header 10.cpp uses (plus whatever httplib/json pull
//    in) so the private->public trick below affects ONLY 10.cpp's own
//    classes — never the standard library.
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <limits>
#include <vector>
#include <queue>
#include <cctype>
#include <mutex>

// 2. Include the backend with its main() renamed and its private members
//    exposed to THIS translation unit. The file 10.cpp on disk is left
//    byte-for-byte untouched.
#define main foodrescue_cli_main
#define private public
#define protected public
#include "10.cpp"
#undef private
#undef protected
#undef main

using json = nlohmann::json;
using namespace httplib;

// ---------------------------------------------------------------------
// Global system instance + mutex (httplib serves requests on threads)
// ---------------------------------------------------------------------
static FoodRescueSystem g_system;   // constructor loads users.dat
static std::mutex       g_mutex;

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
static std::string userName(int id) {
    User *u = g_system.findUser(id);
    return u ? u->getName() : ("User #" + std::to_string(id));
}
static std::string foodName(int id) {
    FoodItem *f = g_system.inventory.findById(id);
    return f ? f->name : ("Food #" + std::to_string(id));
}
static std::string foodAddress(int id) {
    FoodItem *f = g_system.inventory.findById(id);
    return f ? f->donorAddress : "";
}
static std::string firstErrorLine(std::ostringstream &oss) {
    std::istringstream in(oss.str());
    std::string line;
    while (std::getline(in, line))
        if (line.find("[Error]") != std::string::npos) return line;
    return "";
}

// Serialize access to the backend, and capture stdout so the console
// menus don't pollute the server log. "[Error] ..." lines are surfaced
// back to the frontend as JSON.
template <typename F>
static json guarded(F f) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::ostringstream captured;
    std::streambuf *old = std::cout.rdbuf(captured.rdbuf());
    json res;
    try { res = f(captured); }
    catch (const std::exception &e) {
        res = {{"ok", false}, {"error", std::string("backend exception: ") + e.what()}};
    }
    std::cout.rdbuf(old);
    return res;
}

static void sendJson(Response &res, const json &j) {
    res.status = 200;
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Content-Type", "application/json");
    res.body = j.dump();
}
static bool parseBody(const Request &req, Response &res, json &out) {
    out = json::parse(req.body, nullptr, false);
    if (out.is_discarded()) {
        sendJson(res, {{"ok", false}, {"error", "invalid JSON body"}});
        return false;
    }
    return true;
}
static bool needStr(const json &b, const char *k, std::string &v) {
    if (!b.contains(k) || !b[k].is_string() || b[k].get<std::string>().empty()) return false;
    v = b[k].get<std::string>();
    return true;
}

// ---------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------
int main() {
    // The matching engine pauses for keyboard input ("Accept partial
    // quantity? 1/2") when supply < demand. Over HTTP nobody can type an
    // answer, so we pre-authorize "1" (accept partial) many times.
    // The unanswered remainder stays in the FIFO queue, and any future
    // surplus is matched automatically. To decline partials instead,
    // change the "1" below to "2".
    static std::string canned;
    for (int i = 0; i < 100000; ++i) canned += "1\n";
    static std::istringstream cannedStream(canned);
    std::cin.rdbuf(cannedStream.rdbuf());

    Server svr;

    // CORS preflight (the browser sends OPTIONS before each POST)
    svr.Options(R"(.*)", [](const Request &, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    // -------------------- STATS --------------------
    svr.Get("/api/stats", [](const Request &, Response &res) {
        sendJson(res, guarded([&](std::ostringstream &) {
            g_system.autoPurgeExpiredFood();
            int donations = 0, servings = 0;
            g_system.inventory.forEach([&](FoodItem &f) {
                if (!f.isAssigned && !f.isCancelled && !f.isExpired) {
                    ++donations;
                    servings += f.quantity;
                }
            });
            return json{{"ok", true},
                        {"servings_donated", servings},
                        {"donations", donations},
                        {"open_requests", static_cast<int>(g_system.requestQueue.size())},
                        {"pickups", static_cast<int>(g_system.orderHistory.size())}};
        }));
    });

    // -------------------- DONATIONS (active inventory) --------------------
    svr.Get("/api/donations", [](const Request &, Response &res) {
        sendJson(res, guarded([&](std::ostringstream &) {
            g_system.autoPurgeExpiredFood();
            json arr = json::array();
            g_system.inventory.forEach([&](FoodItem &f) {
                if (f.isAssigned || f.isCancelled || f.isExpired) return;
                double mins = f.getRemainingMinutes();
                arr.push_back({
                    {"id", f.foodID},
                    {"food_name", f.name},
                    {"quantity", f.quantity},
                    {"donor_name", userName(f.donorID)},
                    {"category", f.category},
                    {"diet_type", f.dietType},
                    {"remaining_mins", static_cast<int>(mins)},
                    {"critical", mins <= 30.0}   // MinHeap urgency: 30-min rule
                });
            });
            return json{{"ok", true}, {"items", arr}};
        }));
    });

    svr.Post("/api/donations", [](const Request &req, Response &res) {
        json b;
        if (!parseBody(req, res, b)) return;
        sendJson(res, guarded([&](std::ostringstream &cap) {
            int donorId = b.value("donor_id", 0);
            int qty     = b.value("quantity", 0);
            int hours   = b.value("shelf_hours", 0);
            std::string name, cat, diet, ing, spice;
            if (!needStr(b, "food_name", name)   || !needStr(b, "category", cat) ||
                !needStr(b, "diet_type", diet)   || !needStr(b, "ingredients", ing) ||
                !needStr(b, "spice_level", spice))
                return json{{"ok", false},
                            {"error", "missing required fields: food_name, category, diet_type, ingredients, spice_level"}};
            std::string notes = b.value("notes", "");

            if (donorId <= 0 || !g_system.userExists(donorId))
                return json{{"ok", false}, {"error", "Donor ID " + std::to_string(donorId) +
                                           " is not registered — use the Register page first"}};
            if (qty <= 0)   return json{{"ok", false}, {"error", "quantity (servings) must be > 0"}};
            if (hours <= 0) return json{{"ok", false}, {"error", "shelf_hours must be > 0"}};

            int id = g_system.donateFood(donorId, name, cat, qty, hours, diet, ing, spice, notes);
            std::string err = firstErrorLine(cap);
            json r{{"ok", (id != -1) && err.empty()}, {"food_id", id}};
            if (!r["ok"].get<bool>()) r["error"] = err.empty() ? "donation failed" : err;
            return r;
        }));
    });

    // -------------------- REQUESTS (FIFO queue) --------------------
    svr.Get("/api/requests", [](const Request &, Response &res) {
        sendJson(res, guarded([&](std::ostringstream &) {
            g_system.autoPurgeExpiredFood();
            json arr = json::array();
            std::queue<FoodRequest> q = g_system.requestQueue;   // snapshot
            while (!q.empty()) {
                FoodRequest r = q.front(); q.pop();
                std::string desc = "Looking for: " + r.foodName;
                std::string alg  = toLower(trim(r.allergyRestrictions));
                if (!alg.empty() && alg != "none")
                    desc += "  |  avoids: " + r.allergyRestrictions;
                arr.push_back({
                    {"id", r.requestID},
                    {"recipient_name", userName(r.recipientID)},
                    {"food_name", r.foodName},
                    {"quantity", r.requestedQuantity},
                    {"status", r.status},
                    {"description", desc}
                });
            }
            return json{{"ok", true}, {"items", arr}};
        }));
    });

    svr.Post("/api/requests", [](const Request &req, Response &res) {
        json b;
        if (!parseBody(req, res, b)) return;
        sendJson(res, guarded([&](std::ostringstream &cap) {
            int rid = b.value("recipient_id", 0);
            int qty = b.value("quantity", 0);
            std::string food;
            std::string allergies = b.value("allergies", "none");
            if (!needStr(b, "food_name", food))
                return json{{"ok", false}, {"error", "food_name is required"}};
            if (!g_system.recipientExists(rid))
                return json{{"ok", false}, {"error", "Recipient ID " + std::to_string(rid) +
                                           " is not registered — register as an NGO or Animal Shelter first"}};
            if (qty <= 0)
                return json{{"ok", false}, {"error", "quantity (servings) must be > 0"}};

            int id = g_system.requestFood(rid, food, qty, allergies);
            std::string err = firstErrorLine(cap);
            json r{{"ok", (id != -1) && err.empty()}, {"request_id", id}};
            if (!r["ok"].get<bool>()) r["error"] = err.empty() ? "request failed" : err;
            else r["info"] = "Request queued — the matching engine runs automatically. Check Pickups for dispatched orders.";
            return r;
        }));
    });

    // -------------------- PICKUPS (order ledger) --------------------
    svr.Get("/api/pickups", [](const Request &, Response &res) {
        sendJson(res, guarded([&](std::ostringstream &) {
            json arr = json::array();
            for (const auto &o : g_system.orderHistory) {
                arr.push_back({
                    {"order_id", o.orderID},
                    {"food_name", foodName(o.foodID)},
                    {"address", foodAddress(o.foodID)},
                    {"quantity", o.quantity},
                    {"fee", o.deliveryCost},
                    {"status", o.status},
                    {"donor_confirmed", o.donorConfirmed},
                    {"recipient_confirmed", o.recipientConfirmed}
                });
            }
            return json{{"ok", true}, {"items", arr}};
        }));
    });

    svr.Post("/api/pickups/confirm", [](const Request &req, Response &res) {
        json b;
        if (!parseBody(req, res, b)) return;
        sendJson(res, guarded([&](std::ostringstream &cap) {
            int oid = b.value("order_id", 0);
            int uid = b.value("user_id", 0);
            std::string role = b.value("role", "");
            if (oid <= 0 || uid <= 0 || (role != "donor" && role != "recipient"))
                return json{{"ok", false}, {"error", "need order_id, user_id and role ('donor' or 'recipient')"}};
            g_system.confirmDelivery(oid, uid, role == "donor");
            std::string err = firstErrorLine(cap);
            json r{{"ok", err.empty()}};
            if (!err.empty()) r["error"] = err;
            return r;
        }));
    });

    // -------------------- REGISTRATION --------------------
    svr.Post("/api/register", [](const Request &req, Response &res) {
        json b;
        if (!parseBody(req, res, b)) return;
        sendJson(res, guarded([&](std::ostringstream &) {
            std::string role, name, contact, addr, subtype;
            if (!needStr(b, "role", role)       || !needStr(b, "name", name) ||
                !needStr(b, "contact", contact) || !needStr(b, "address", addr) ||
                !needStr(b, "subtype", subtype))
                return json{{"ok", false},
                            {"error", "missing fields: role, name, contact, address, subtype"}};
            if (contact.size() != 10 ||
                !std::all_of(contact.begin(), contact.end(),
                             [](unsigned char c){ return std::isdigit(c) != 0; }))
                return json{{"ok", false}, {"error", "phone number must be exactly 10 digits"}};
            if (g_system.isPhoneRegistered(contact))
                return json{{"ok", false}, {"error", "this phone number is already registered"}};

            int id = -1;
            if (role == "donor")             id = g_system.registerDonor(name, contact, addr, subtype);
            else if (role == "ngo")          id = g_system.registerNGO(name, contact, addr, subtype);
            else if (role == "shelter")      id = g_system.registerAnimalShelter(name, contact, addr, subtype);
            else return json{{"ok", false}, {"error", "role must be donor | ngo | shelter"}};

            return json{{"ok", true}, {"id", id}, {"role", role}};
        }));
    });

    std::cout << "FoodRescue API ready  ->  http://localhost:8080/api\n"
                 "  GET  /api/stats  /api/donations  /api/requests  /api/pickups\n"
                 "  POST /api/register  /api/donations  /api/requests  /api/pickups/confirm\n";
    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "[Error] could not bind port 8080\n";
        return 1;
    }
    return 0;
}
