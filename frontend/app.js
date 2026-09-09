const API = "http://localhost:8080/api";
const pages = document.querySelectorAll(".page");
const title = document.getElementById("pageTitle");

function showPage(id){
  pages.forEach(p=>p.classList.toggle("active-page",p.id===id));
  document.querySelectorAll(".nav").forEach(n=>n.classList.toggle("active",n.dataset.page===id));
  title.textContent = id==="donate" ? "Donate Food" : id==="requests" ? "Food Requests" : id==="pickups" ? "Pickups" : "Dashboard";
}
document.querySelectorAll(".nav").forEach(n=>n.onclick=()=>showPage(n.dataset.page));
document.querySelectorAll("[data-go]").forEach(n=>n.onclick=()=>showPage(n.dataset.go));
document.getElementById("refreshBtn").onclick=loadData;

async function get(path){
  const r=await fetch(API+path);
  if(!r.ok) throw new Error("API error");
  return r.json();
}
function badge(text){return `<span class="badge">${text}</span>`}

async function loadData(){
  try{
    const [stats, donations, requests, pickups] = await Promise.all([
      get("/stats"), get("/donations"), get("/requests"), get("/pickups")
    ]);
    document.getElementById("statKg").textContent = `${Number(stats.food_kg||0).toFixed(1)} kg`;
    document.getElementById("statDonations").textContent = stats.donations||0;
    document.getElementById("statRequests").textContent = stats.open_requests||0;
    document.getElementById("statPickups").textContent = stats.pickups||0;
    document.getElementById("sideSaved").textContent = `${Number(stats.food_kg||0).toFixed(1)} kg`;

    document.getElementById("recentTable").innerHTML = donations.length ? donations.slice(0,8).map(d=>`
      <tr><td>${escapeHtml(d.food_name)}</td><td>${d.quantity_kg} kg</td><td>${escapeHtml(d.donor_name)}</td><td>${badge(d.status)}</td></tr>`).join("") :
      `<tr><td colspan="4" class="empty">No donations yet.</td></tr>`;

    document.getElementById("requestCards").innerHTML = requests.length ? requests.map(r=>`
      <div class="request-card"><p class="eyebrow">REQUEST #${r.id}</p><h4>${escapeHtml(r.organization_name)}</h4>
      <p>${escapeHtml(r.description||"Food required")}</p><div class="need">${r.quantity_kg} kg needed</div>${badge(r.status)}</div>`).join("") :
      `<div class="request-card"><h4>No open requests</h4><p>New community requests will appear here.</p></div>`;

    document.getElementById("pickupTable").innerHTML = pickups.length ? pickups.map(p=>`
      <tr><td>${escapeHtml(p.food_name)}</td><td>${escapeHtml(p.pickup_address)}</td><td>${escapeHtml(p.pickup_time||"Not scheduled")}</td><td>${badge(p.status)}</td></tr>`).join("") :
      `<tr><td colspan="4" class="empty">No pickups yet.</td></tr>`;
  }catch(e){
    document.getElementById("recentTable").innerHTML=`<tr><td colspan="4" class="empty">Backend not running. Start the C++ server on port 8080.</td></tr>`;
  }
}
function escapeHtml(s=""){return String(s).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#039;"}[c]))}

document.getElementById("donationForm").onsubmit=async e=>{
  e.preventDefault();
  const msg=document.getElementById("formMessage");
  const data=Object.fromEntries(new FormData(e.target).entries());
  data.quantity_kg=Number(data.quantity_kg);
  try{
    const r=await fetch(API+"/donations",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(data)});
    if(!r.ok) throw new Error();
    msg.textContent="✓ Donation added successfully.";
    e.target.reset(); loadData();
  }catch(err){msg.textContent="Could not connect to the C++ backend."}
};
loadData();