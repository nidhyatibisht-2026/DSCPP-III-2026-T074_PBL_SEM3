# DSCPP-III-2026-T074_PBL_SEM3
# FOOD SURPLUS MANAGEMENT AND REDISTRIBUTION SYSTEM

## Brief Description
FoodFlow is an integrated 3-tier web application designed to combat food waste and hunger by seamlessly connecting food donors (restaurants, mess halls, catering services) with recipient NGOs. The platform automates surplus inventory tracking, allergen filtering, FIFO (First-In, First-Out) queuing, and priority matching via a robust C++ backend bridged with a Python Flask API and a modern web frontend.

## Problem Statement & Objective:-
**Problem Statement:** Large quantities of surplus food go to waste daily in institutional canteens, restaurants, and events, while nearby shelters and NGOs struggle with inefficient manual distribution, tracking, and expiry management.
**Objective:** To build an automated, real-time food surplus management system that minimizes delivery delays, enforces strict allergen screening, handles priority queues via efficient data structures, and ensures secure, verifiable pickup handshakes.

## Team Members
* Nidhyati Bisht — Team Lead
* Subhansh Singh Rawat
* Jatin Singh Rawat
* Vartika Gosai

## Technologies & Tools Used:
* **Frontend:** HTML, CSS, JavaScript 
* **Backend Engine:** C++ (Object-Oriented Programming, Data Structures)
* **API Middleware:** Python Flask 
* **Database Management:** SQLite 
* **Version Control:** Git & GitHub

## Project Setup & Installation Instructions

### Prerequisites
* Python 3.x installed with Flask (`pip install flask`)
* A C++ Compiler (Code::Blocks / GCC / MSVC)

### Step-by-Step Local Execution
1. Clone the repository and navigate to the project directory:
2. Ensure the C++ executable (phase2.exe) is compiled from phase2.cpp and placed in the root directory alongside app.py.
3. Initialize or verify the SQLite database: python db_manage2.py
4. Start the Flask API bridge server: py app.py
5. Open your web browser and navigate to: http://localhost:8080

## Major Features & Modules:-
1. User Authentication & Profiles (users.dat + SQLite): Role-based registration for Donors and Recipient NGOs with 10-digit validation.
2. Surplus Food Donation Engine: Real-time logging of meal types, servings, shelf-life expiry, dietary tags (Veg/Non-Veg), and ingredient-level allergen data.
3. Automated Matching & Queueing: Implements FIFO queues and Min-Heap priority sorting to match available food batches with active NGO requests.
4. Two-Way Pickup Handshake: Dual confirmation workflow (Donor ✓ and Recipient ✓) to securely close and update distribution cycles.
5. Live Dashboard Analytics: Real-time statistics tracking total donations, servings distributed, active requests, and pending pickups.

## Current Project Status & Progress:
Phase 1 & 2 Completed: Core C++ data structures (phase2.cpp), SQLite database schema (schema2.sql), and DBMS management wrapper (db_manage2.py) fully implemented.
Integration of backend, DBMS, frontend is done via Python Flask. Phase 2 functional prototype is implemented.
Integration Completed: Flask server (app.py) successfully handles frontend-to-backend communication via local subprocess calls, avoiding CORS blocks.
UI Live: Fully responsive web interface (index.html, style.css, app.js) deployed and tested locally on port 8080.
