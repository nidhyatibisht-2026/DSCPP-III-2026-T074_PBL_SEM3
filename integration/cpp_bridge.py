"""ctypes wrapper for the FoodFlow C++ shared library."""

import ctypes
import os
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class CppBridge:
    def __init__(self):
        self._dll_directory = None
        if os.name == "nt":
            mingw_bin = Path(r"C:\msys64\ucrt64\bin")
            if mingw_bin.is_dir():
                os.environ["PATH"] = str(mingw_bin) + os.pathsep + os.environ.get("PATH", "")
                if hasattr(os, "add_dll_directory"):
                    self._dll_directory = os.add_dll_directory(str(mingw_bin))

        configured_library = os.environ.get("FOODFLOW_BRIDGE_LIBRARY")
        candidates = ([Path(configured_library)] if configured_library else []) + [
            ROOT / "build-mingw" / "bin" / "libfoodflow_bridge.dll",
            ROOT / "build" / "bin" / "foodflow_bridge.dll",
            ROOT / "bin" / "foodflow_bridge.dll",
            ROOT / "build" / "libfoodflow_bridge.so",
            ROOT / "build" / "libfoodflow_bridge.dylib",
        ]
        library_path = next((path for path in candidates if path.is_file()), None)
        if library_path is None:
            expected = "build-mingw/bin/foodflow_bridge.dll"
            raise FileNotFoundError(
                f"FoodFlow C++ bridge library not found. Build it first; expected {expected}."
            )

        self.lib = ctypes.CDLL(str(library_path))
        self._configure_signatures()
        if self.lib.ff_initialize() != 1:
            raise RuntimeError(
                "The C++ FoodFlow system could not initialize. Check the database path and console output."
            )

    @staticmethod
    def _text(value):
        return str(value if value is not None else "").encode("utf-8")

    def _configure_signatures(self):
        c_char = ctypes.c_char_p
        self.lib.ff_initialize.restype = ctypes.c_int
        self.lib.ff_shutdown.restype = None
        self.lib.ff_purge_expired.restype = None

        self.lib.ff_register_user.argtypes = [c_char, c_char, c_char, c_char, c_char]
        self.lib.ff_register_user.restype = ctypes.c_int

        self.lib.ff_donate_food.argtypes = [
            ctypes.c_int, c_char, c_char, ctypes.c_int, ctypes.c_int,
            c_char, c_char, c_char, c_char,
        ]
        self.lib.ff_donate_food.restype = ctypes.c_int

        self.lib.ff_request_food.argtypes = [ctypes.c_int, c_char, ctypes.c_int, c_char]
        self.lib.ff_request_food.restype = ctypes.c_int

        self.lib.ff_cancel_donation.argtypes = [ctypes.c_int, ctypes.c_int]
        self.lib.ff_cancel_donation.restype = ctypes.c_int

        self.lib.ff_confirm_delivery.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
        self.lib.ff_confirm_delivery.restype = ctypes.c_int

        self.lib.ff_file_complaint.argtypes = [ctypes.c_int, ctypes.c_int, c_char]
        self.lib.ff_file_complaint.restype = ctypes.c_int

    def purge_expired(self):
        self.lib.ff_purge_expired()

    def register_user(self, role, name, contact, address, subtype):
        return self.lib.ff_register_user(*map(self._text, (role, name, contact, address, subtype)))

    def donate_food(self, donor_id, name, category, quantity, shelf_hours,
                    diet_type, ingredients, spice, notes):
        return self.lib.ff_donate_food(
            int(donor_id), self._text(name), self._text(category),
            int(quantity), int(shelf_hours), self._text(diet_type),
            self._text(ingredients), self._text(spice), self._text(notes),
        )

    def request_food(self, recipient_id, food_name, quantity, allergies):
        return self.lib.ff_request_food(
            int(recipient_id), self._text(food_name), int(quantity), self._text(allergies)
        )

    def cancel_donation(self, food_id, donor_id):
        return bool(self.lib.ff_cancel_donation(int(food_id), int(donor_id)))

    def confirm_delivery(self, order_id, user_id, is_donor):
        return bool(self.lib.ff_confirm_delivery(int(order_id), int(user_id), int(is_donor)))

    def file_complaint(self, order_id, recipient_id, details):
        return bool(self.lib.ff_file_complaint(
            int(order_id), int(recipient_id), self._text(details)
        ))

    def close(self):
        if getattr(self, "lib", None):
            self.lib.ff_shutdown()
