# DSCPP-III-2026-T074_PBL_SEM3

# FOOD SURPLUS MANAGEMENT AND REDISTRIBUTION SYSTEM

**FoodFlow** is a food surplus management and redistribution system developed to reduce food wastage by connecting food donors, such as restaurants, institutional canteens and catering services, with recipient organisations such as NGOs and shelters.

The system provides an integrated platform for user registration, profile management, food donations, recipient requests, automated food matching, inventory management, order processing and delivery confirmation. It also includes administrative access and operations for system management.

FoodFlow is developed using a modular C++ application integrated with a Python Flask API, an SQLite database and a web-based frontend.

## Problem Statement

Large quantities of usable food are wasted in restaurants, institutional canteens, hostels and events, while NGOs and shelters often face difficulties accessing available surplus food in a timely manner.

Manual coordination through calls and messages makes it difficult to manage food availability, recipient requests, delivery time, expiry limits and food-related requirements. This can lead to delays, inefficient allocation and unnecessary food wastage.

## Project Objective

To develop a system that facilitates the timely redistribution of surplus food by connecting donors with suitable recipients, maintaining food inventory and requests, and supporting efficient matching based on food availability, quantity, expiry information and recipient requirements.

The project aims to provide a structured, integrated and reliable platform for managing the complete food redistribution process.

## Team Details

**Team ID:** DSCPP-III-2026-T074

**Repository / Project Identifier:** DSCPP-III-2026-T074_PBL_SEM3

## Team Members

- **Nidhyati Bisht** — Team Lead
- Subhansh Singh Rawat
- Jatin Singh Rawat
- Vartika Gosai

## Technologies and Tools Used

| Component                  | Technology                                           |
| -------------------------- | ---------------------------------------------------- |
| Frontend                   | HTML, CSS, JavaScript                                |
| Core application           | C++                                                  |
| Programming concepts       | Object-Oriented Programming and Data Structures      |
| Integration layer          | Python Flask                                         |
| C++ and Python integration | C-compatible shared-library bridge and Python ctypes |
| Database                   | SQLite                                               |
| Build system               | CMake                                                |
| Version control            | Git and GitHub                                       |
| Development environment    | Visual Studio Code, GCC/G++ (MSYS2 UCRT64)           |

## Major Features and Modules

1. **User Registration and Profile Management:** Supports donor and recipient registration and maintains their account and profile information.

2. **Surplus Food Management:** Allows donors to manage food donation details, including food name, quantity, category, expiry information and relevant dietary or allergen details.

3. **Recipient Requests:** Allows recipients to submit food requests with the required information for the matching process.

4. **Automated Food Matching and Queue Management:** Uses data structures such as FIFO queues and a Min-Heap to support request processing, food allocation and expiry-based management.

5. **Order and Delivery Management:** Maintains order information and supports delivery confirmation by donors and recipients.

6. **Admin Login and Administrative Operations:** Provides administrative access and supports system-level operations, including inventory monitoring and the management or review of requests and orders.

7. **Database Management and Validation:** Maintains application records using SQLite and applies database constraints and integrity checks to support consistent data management.

## Project Workflow

The FoodFlow system follows a structured food redistribution workflow:

1. **User Registration:** Donors and recipients register on the platform and maintain their profiles.
2. **Food Donation:** Donors provide surplus food information, including quantity, category and expiry details.
3. **Recipient Requests:** Recipients submit requests based on their food requirements.
4. **Automated Matching:** The system processes available donations and recipient requests using the matching pipeline and relevant data structures.
5. **Order Creation:** Suitable matches are converted into orders for the redistribution process.
6. **Delivery Confirmation:** Donors and recipients confirm the delivery process through the application.
7. **Administrative Operations:** The administrator accesses the system to perform administrative operations and monitor relevant application records.

## Project Architecture

FoodFlow follows a layered architecture that separates the frontend, integration layer, core application and database.

- **Frontend:** HTML, CSS and JavaScript provide the browser-based user interface.
- **Integration Layer:** Python Flask exposes the HTTP API. A Python `ctypes` wrapper communicates with the C-compatible bridge exported by the C++ shared library.
- **Core Application:** The modular C++ implementation handles registration, profiles, donations, recipient requests, matching, orders, delivery operations and administrative functionality.
- **Database Layer:** SQLite serves as the central data store. Application mutations are handled by the C++ system, while Flask retrieves data for the web interface.

The web integration executes the matching pipeline in non-interactive mode. If the available food is insufficient for a request, the request remains pending instead of waiting for console input or automatically accepting a partial match.

The existing console application also retains its interactive functionality.

## Phase 3 Development and Improvements

During Phase 3, the project was further developed and integrated into a more organized and maintainable implementation.

The major improvements completed during this phase include:

- **Admin Login and Operations:** Implemented administrative login functionality and administrative operations.
- **Modular C++ Implementation:** Organized the C++ application into separate source (`.cpp`) and header (`.hpp`) files.
- **Code Organization:** Structured the project into dedicated directories for source files, headers, database, frontend, integration, scripts and documentation.
- **Database Integration:** Integrated the modular C++ application with the SQLite database as the central data store.
- **Database Validation:** Reviewed database operations, constraints, foreign-key relationships and integrity to identify and address potential issues.
- **C++ and Python Integration:** Integrated the C++ shared library with the Flask application through a C-compatible bridge and Python `ctypes`.
- **Repository Organization:** Updated the GitHub repository to reflect the modular project structure and maintain the final integrated implementation.
- **Application Testing:** Tested the integrated application workflow, including user registration, food donations, recipient requests, matching, order creation and delivery confirmation.

## Project Setup and Installation

### Prerequisites

- Python 3.x
- Flask (installed by the launcher using `requirements.txt`)
- CMake
- MSYS2 UCRT64 GCC/G++ with C++17 support
- SQLite development library available to the UCRT64 toolchain

### Run the Integrated Application on Windows

1. Open the project folder in File Explorer or Visual Studio Code.
2. Double-click `scripts/run_foodflow.bat`.
3. The launcher configures and builds the C++ console application and shared library, installs the Python requirements and starts the Flask application.
4. Open the following address in your browser:

   `http://127.0.0.1:8080`

The launcher expects the MSYS2 UCRT64 compiler at `C:/msys64/ucrt64/bin/`. If MSYS2 is installed elsewhere, update `MINGW_BIN` in `scripts/run_foodflow.bat` and the corresponding DLL search path in `integration/cpp_bridge.py`.

The CMake build creates the `foodflow` console executable and the `foodflow_bridge` shared library. The Flask application communicates with the C++ system through the shared library rather than launching the older Phase 2 executable.

## Current Project Status and Progress

**Current Stage: Phase 3 — Final Phase**

FoodFlow has progressed from its earlier implementation stages to a modular, integrated application. The major development and integration work for the final phase has been completed and the project is being prepared for its Phase 3 evaluation.

The current implementation includes:

- Donor and recipient registration and profile management.
- Food donation and inventory management.
- Recipient food requests.
- Automated food matching and expiry-based processing.
- Order creation and delivery confirmation.
- Admin login and administrative operations.
- Modular C++ source and header files.
- Integration of the C++ backend with the Flask API and web frontend.
- SQLite database integration, validation and integrity checks.
- An organized GitHub repository containing the integrated project structure.

The application has been tested locally through its integrated web interface. Further refinements and improvements may be carried out as part of the final evaluation and project completion process.

## Team Contribution

The project has been developed collaboratively by all team members. The work includes problem analysis, system design, implementation of data structures and object-oriented components, frontend development, database integration, backend development, testing, debugging and repository management.

## Project Repository

**GitHub Repository:** [DSCPP-III-2026-T074_PBL_SEM3](https://github.com/nidhyatibisht-2026/DSCPP-III-2026-T074_PBL_SEM3)

## Project Documentation

The repository includes the available project reports and presentations from the earlier project phases and the current project phase in the `docs/` directory.
