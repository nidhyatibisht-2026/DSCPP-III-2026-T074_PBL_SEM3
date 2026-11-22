const API = "/api";
const pages = document.querySelectorAll(".page");
const title = document.getElementById("pageTitle");

const titles = {
  dashboard: "Dashboard",
  register: "Register",
  donate: "Donate Food",
  requests: "Food Requests",
  pickups: "Pickups",
  adminPortal: "Admin Portal"
};

function showPage(id) {
  if (!id || !titles[id]) return;
  pages.forEach(p => p.classList.toggle("active-page", p.id === id));
  document.querySelectorAll(".nav").forEach(n => n.classList.toggle("active", n.dataset.page === id));
  title.textContent = titles[id] || "Dashboard";
}
document.querySelectorAll(".nav").forEach(n => {
  n.onclick = () => {
    if (!n.dataset.page) return;
    showPage(n.dataset.page);
  };
});
document.querySelectorAll("[data-go]").forEach(n => n.onclick = () => showPage(n.dataset.go));
document.getElementById("refreshBtn").onclick = loadData;

async function get(path) {
  const r = await fetch(API + path);
  const j = await r.json();
  if (!j.ok) throw new Error(j.error || "API error");
  return j;
}
function badge(text, cls = "") { return `<span class="badge ${cls}">${text}</span>` }

function isCompletedPickup(status = "") {
  const s = String(status).toLowerCase();
  return s.includes("delivered") || s.includes("fulfilled");
}

async function loadData() {
  try {
    const [stats, donations, requests, pickups] = await Promise.all([
      get("/stats"), get("/donations"), get("/requests"), get("/pickups")
    ]);

    document.getElementById("statServings").textContent = `${stats.servings_donated} servings`;
    document.getElementById("statDonations").textContent = stats.donations;
    document.getElementById("statRequests").textContent = stats.open_requests;
    document.getElementById("statPickups").textContent = stats.pickups;
    document.getElementById("sideSaved").textContent = `${stats.servings_donated} servings`;

    document.getElementById("recentTable").innerHTML = donations.items.length
      ? donations.items.slice(0, 8).map(d => `
        <tr>
          <td>${escapeHtml(d.food_name)} <small style="color:#8b948e">(${escapeHtml(d.diet_type)})</small></td>
          <td>${d.quantity}</td>
          <td>${escapeHtml(d.donor_name)}</td>
          <td>${d.critical ? badge("Critical · &lt;30 min", "warn") : badge("Available")}</td>
        </tr>`).join("")
      : `<tr><td colspan="4" class="empty">No donations yet.</td></tr>`;

    document.getElementById("requestCards").innerHTML = requests.items.length
      ? requests.items.map(r => `
        <div class="request-card">
          <p class="eyebrow">REQUEST #${r.id}</p>
          <h4>${escapeHtml(r.recipient_name)}</h4>
          <p>${escapeHtml(r.description)}</p>
          <div class="need">${r.quantity} servings</div>
          ${badge(r.status)}
        </div>`).join("")
      : `<div class="request-card"><h4>No open requests</h4><p>New community requests will appear here.</p></div>`;

    const activePickups = pickups.items.filter(p => !isCompletedPickup(p.status));

    document.getElementById("pickupTable").innerHTML = activePickups.length
      ? activePickups.map(p => `
        <tr>
          <td>${escapeHtml(p.food_name)} <small style="color:#8b948e">#${p.order_id}</small></td>
          <td>${escapeHtml(p.address)}</td>
          <td>${p.quantity}</td>
          <td>${badge(p.status)}<br><small style="color:#8b948e">
            Donor: ${p.donor_confirmed ? "✓" : "…"} · Recipient: ${p.recipient_confirmed ? "✓" : "…"}
          </small></td>
          <td>
            <button class="btn-small" data-confirm data-order="${p.order_id}" data-role="donor">Donor ✓</button>
            <button class="btn-small" data-confirm data-order="${p.order_id}" data-role="recipient">Recipient ✓</button>
          </td>
        </tr>`).join("")
      : `<tr><td colspan="5" class="empty">No active pickups — completed orders are hidden from the schedule.</td></tr>`;
  } catch (e) {
    const msg = `<tr><td colspan="5" class="empty">Backend not running. Start FoodFlow using scripts/run_foodflow.bat.</td></tr>`;
    document.getElementById("recentTable").innerHTML = msg;
    document.getElementById("pickupTable").innerHTML = msg;
  }
}
function escapeHtml(s = "") { return String(s).replace(/[&<>"']/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#039;" }[c])) }

// Two-way delivery handshake: Donor ✓ / Recipient ✓
document.getElementById("pickupTable").addEventListener("click", async e => {
  const btn = e.target.closest("button[data-confirm]");
  if (!btn) return;
  const uid = prompt(`Enter your User ID to confirm order #${btn.dataset.order} as ${btn.dataset.role}:`);
  if (!uid) return;
  try {
    const r = await fetch(API + "/pickups/confirm", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ order_id: Number(btn.dataset.order), user_id: Number(uid), role: btn.dataset.role })
    });
    const j = await r.json();
    alert(j.ok ? "✓ Confirmation recorded. When both sides confirm, the order closes automatically." : ("✗ " + (j.error || "Failed")));
    loadData();
  } catch { alert("✗ Could not reach the backend."); }
});

async function postJSON(path, data) {
  const r = await fetch(API + path, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(data) });
  return r.json();
}
function setMsg(el, j, okText) {
  el.classList.toggle("error", !j.ok);
  el.textContent = j.ok ? ("✓ " + okText) : ("✗ " + (j.error || "Request failed"));
}

// Donation form -> backend donateFood()
document.getElementById("donationForm").onsubmit = async e => {
  e.preventDefault();
  const msg = document.getElementById("formMessage");
  const f = Object.fromEntries(new FormData(e.target).entries());
  const data = {
    donor_id: Number(f.donor_id),
    food_name: f.food_name,
    category: f.category,
    quantity: Number(f.quantity),
    shelf_hours: Number(f.shelf_hours),
    diet_type: f.diet_type,
    ingredients: f.ingredients,
    spice_level: f.spice_level,
    notes: f.notes || ""
  };
  try {
    const j = await postJSON("/donations", data);
    setMsg(msg, j, `Donation added (Food #${j.food_id}).`);
    if (j.ok) { e.target.reset(); loadData(); }
  } catch {
    msg.classList.add("error");
    msg.textContent = "✗ Could not connect to the FoodFlow backend.";
  }
};

// Register form -> backend registerDonor / registerNGO / registerAnimalShelter
const subtypeLabels = {
  donor: "Donor type",
  ngo: "Diet requirement (veg / all)",
  shelter: "Species (e.g. Dogs / Cats)"
};
document.getElementById("regRole").onchange = e => {
  document.getElementById("regSubtypeLabel").textContent = subtypeLabels[e.target.value];
};
document.getElementById("registerForm").onsubmit = async e => {
  e.preventDefault();
  const msg = document.getElementById("registerMessage");
  const f = Object.fromEntries(new FormData(e.target).entries());
  try {
    const j = await postJSON("/register", {
      role: f.role, name: f.name, contact: f.contact, address: f.address, subtype: f.subtype
    });
    setMsg(msg, j, `Registered! Your ${j.role} ID = ${j.id} — save this ID, it is required to donate or request.`);
    if (j.ok) { e.target.reset(); loadData(); }
  } catch {
    msg.classList.add("error");
    msg.textContent = "✗ Could not connect to the FoodFlow backend.";
  }
};

// Request form -> backend requestFood()
document.getElementById("requestForm").onsubmit = async e => {
  e.preventDefault();
  const msg = document.getElementById("requestMessage");
  const f = Object.fromEntries(new FormData(e.target).entries());
  try {
    const j = await postJSON("/requests", {
      recipient_id: Number(f.recipient_id),
      food_name: f.food_name,
      quantity: Number(f.quantity),
      allergies: f.allergies || "none"
    });
    setMsg(msg, j, `Request #${j.request_id} placed in the queue — check Pickups for matches.`);
    if (j.ok) { e.target.reset(); loadData(); }
  } catch {
    msg.classList.add("error");
    msg.textContent = "✗ Could not connect to the FoodFlow backend.";
  }
};

// ============================================================
// Admin Login Modal, Authentication, Portal & Session State
// ============================================================
const targetPrompts = {
  7: "Enter Food ID to remove/cancel:",
  8: "Enter Request ID to cancel/reject:",
  11: "Enter User ID to deactivate/reactivate:"
};

function openAdminLoginModal() {
  const errEl = document.getElementById("adminLoginError");
  const form = document.getElementById("adminLoginForm");
  const modal = document.getElementById("adminLoginModal");
  if (errEl) errEl.textContent = "";
  if (form) form.reset();
  if (modal) modal.style.display = "flex";
}

function closeAdminLoginModal() {
  const modal = document.getElementById("adminLoginModal");
  if (modal) modal.style.display = "none";
}

function updateAdminUI(isAdmin, adminUser = "") {
  const loginBtn = document.getElementById("adminLoginBtn");
  const logoutBtn = document.getElementById("adminLogoutBtn");
  const portalNavBtn = document.getElementById("adminPortalNavBtn");
  const portalStatus = document.getElementById("adminPortalStatus");
  const badge = document.getElementById("adminSessionBadge");
  const section = document.getElementById("adminDashboardSection");
  const welcomeText = document.getElementById("adminWelcomeText");

  if (loginBtn) loginBtn.style.display = isAdmin ? "none" : "";
  if (logoutBtn) logoutBtn.style.display = isAdmin ? "" : "none";
  if (portalNavBtn) portalNavBtn.style.display = isAdmin ? "" : "none";
  if (section) section.style.display = isAdmin ? "block" : "none";

  if (portalStatus && adminUser) {
    portalStatus.textContent = `Signed in as ${adminUser}`;
  }

  if (badge) {
    badge.style.display = isAdmin ? "block" : "none";
    badge.textContent = isAdmin ? `Admin: ${adminUser}` : "";
  }

  if (welcomeText && isAdmin) {
    welcomeText.textContent = `Logged in as ${adminUser}. Authorized access granted to system logs, manual order dispatches, and inventory controls.`;
  }

  if (!isAdmin && document.getElementById("adminPortal")?.classList.contains("active-page")) {
    showPage("dashboard");
  }
}

async function handleAdminLogin(event) {
  event.preventDefault();
  const userInput = document.getElementById("adminUsername");
  const passInput = document.getElementById("adminPassword");
  const errEl = document.getElementById("adminLoginError");

  const username = userInput ? userInput.value.trim() : "";
  const password = passInput ? passInput.value : "";

  try {
    const res = await fetch(API + "/admin/login", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password })
    });
    const data = await res.json();

    if (res.ok && data.authenticated) {
      closeAdminLoginModal();
      updateAdminUI(true, data.admin_user);
      showPage("adminPortal");
    } else if (errEl) {
      errEl.textContent = data.error || data.message || "Invalid Admin ID or password.";
    }
  } catch (_) {
    if (errEl) {
      errEl.textContent = "✗ Could not connect to the FoodFlow backend.";
    }
  }
}

async function logoutAdmin() {
  try {
    await fetch(API + "/admin/logout", { method: "POST" });
  } catch (_) {}
  updateAdminUI(false, "");
}

async function checkAdminSession() {
  try {
    const res = await fetch(API + "/admin/session");
    const data = await res.json();
    updateAdminUI(Boolean(data.authenticated), data.admin_user || "");
  } catch (_) {
    updateAdminUI(false, "");
  }
}

async function runAdminMenuOption(optionId) {
  const titleEl = document.getElementById("adminOutputTitle");
  const boxEl = document.getElementById("adminOutputBox");

  let targetId = null;

  if (targetPrompts[optionId]) {
    const entered = prompt(targetPrompts[optionId]);
    if (entered === null) return;
    const parsed = Number(entered.trim());
    if (!Number.isInteger(parsed) || parsed < 1) {
      if (titleEl) titleEl.textContent = `Option ${optionId}`;
      if (boxEl) boxEl.textContent = "✗ Please enter a valid positive numeric ID.";
      return;
    }
    targetId = parsed;
  }

  if (titleEl) titleEl.textContent = `Running Option ${optionId}...`;
  if (boxEl) boxEl.textContent = "Please wait...";

  try {
    const res = await fetch(`${API}/admin/menu/${optionId}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ target_id: targetId })
    });
    const data = await res.json();

    if (titleEl && data.title) titleEl.textContent = data.title;
    if (!res.ok || !data.ok) {
      if (boxEl) boxEl.textContent = `✗ ${data.error || "Action failed"}`;
      return;
    }

    if (boxEl) {
      if (data.message) {
        boxEl.textContent = `✓ ${data.message}`;
      } else if (Array.isArray(data.items) && data.items.length > 0) {
        boxEl.textContent = JSON.stringify(data.items, null, 2);
      } else {
        boxEl.textContent = "No records found.";
      }
    }
    loadData();
  } catch (_) {
    if (boxEl) boxEl.textContent = "✗ Could not connect to the FoodFlow backend.";
  }
}

document.addEventListener("click", (e) => {
  if (e.target.closest("#adminLoginBtn")) {
    e.preventDefault();
    openAdminLoginModal();
    return;
  }
  if (e.target.closest("#adminLogoutBtn")) {
    e.preventDefault();
    logoutAdmin();
    return;
  }
  if (e.target.closest("#cancelAdminLoginBtn")) {
    e.preventDefault();
    closeAdminLoginModal();
    return;
  }
  if (e.target.id === "adminLoginModal") {
    closeAdminLoginModal();
    return;
  }
  const optBtn = e.target.closest(".admin-opt-btn");
  if (optBtn) {
    e.preventDefault();
    const opt = Number(optBtn.dataset.action);
    if (opt) runAdminMenuOption(opt);
  }
});

document.addEventListener("submit", (e) => {
  if (e.target && e.target.id === "adminLoginForm") {
    handleAdminLogin(e);
  }
});

loadData();
checkAdminSession();