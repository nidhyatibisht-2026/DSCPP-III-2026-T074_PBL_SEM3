const API = "http://localhost:8080/api";
const pages = document.querySelectorAll(".page");
const title = document.getElementById("pageTitle");

const titles = {
  dashboard: "Dashboard",
  register: "Register",
  donate: "Donate Food",
  requests: "Food Requests",
  pickups: "Pickups"
};

function showPage(id) {
  pages.forEach(p => p.classList.toggle("active-page", p.id === id));
  document.querySelectorAll(".nav").forEach(n => n.classList.toggle("active", n.dataset.page === id));
  title.textContent = titles[id] || "Dashboard";
}
document.querySelectorAll(".nav").forEach(n => n.onclick = () => showPage(n.dataset.page));
document.querySelectorAll("[data-go]").forEach(n => n.onclick = () => showPage(n.dataset.go));
document.getElementById("refreshBtn").onclick = loadData;

async function get(path) {
  const r = await fetch(API + path);
  const j = await r.json();
  if (!j.ok) throw new Error(j.error || "API error");
  return j;
}
function badge(text, cls = "") { return `<span class="badge ${cls}">${text}</span>` }

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

    document.getElementById("pickupTable").innerHTML = pickups.items.length
      ? pickups.items.map(p => `
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
      : `<tr><td colspan="5" class="empty">No pickups yet — orders appear here once a request is matched.</td></tr>`;
  } catch (e) {
    const msg = `<tr><td colspan="5" class="empty">Backend not running. Start the C++ server on port 8080.</td></tr>`;
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
    msg.textContent = "✗ Could not connect to the C++ backend.";
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
    msg.textContent = "✗ Could not connect to the C++ backend.";
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
    msg.textContent = "✗ Could not connect to the C++ backend.";
  }
};

loadData();
