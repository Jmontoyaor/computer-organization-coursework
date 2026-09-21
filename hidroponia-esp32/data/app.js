const MAX_POINTS = 60;

const state = {
  ph: [],
  temperature: [],
  tds: [],
  waterLevel: [],
};

const ranges = {
  ph: { min: 5.5, max: 6.5, unit: "", label: "pH" },
  temperature: { min: 18, max: 26, unit: "°C", label: "Temperatura" },
  tds: { min: 560, max: 840, unit: " ppm", label: "TDS / EC" },
  waterLevel: { min: 30, max: 100, unit: "%", label: "Nivel de agua" },
};

const colors = {
  ph: "#4f8ef7",
  temperature: "#f76c4f",
  tds: "#3ddc84",
  waterLevel: "#c78ef7",
};

function statusFor(key, value) {
  if (value === null || value === undefined || Number.isNaN(value)) return "unknown";
  const r = ranges[key];
  return value < r.min || value > r.max ? "alert" : "ok";
}

function pushPoint(key, value) {
  const arr = state[key];
  arr.push(value);
  if (arr.length > MAX_POINTS) arr.shift();
}

function drawChart(canvas, data, min, max, color) {
  const ctx = canvas.getContext("2d");
  const dpr = window.devicePixelRatio || 1;
  const w = (canvas.width = canvas.clientWidth * dpr);
  const h = (canvas.height = canvas.clientHeight * dpr);
  ctx.clearRect(0, 0, w, h);

  const validData = data.filter((v) => v !== null && v !== undefined && !Number.isNaN(v));
  if (validData.length < 2) return;

  const dataMin = Math.min(min, ...validData);
  const dataMax = Math.max(max, ...validData);
  const span = dataMax - dataMin || 1;

  ctx.beginPath();
  ctx.lineWidth = 2 * dpr;
  ctx.strokeStyle = color;
  ctx.lineJoin = "round";

  let started = false;
  data.forEach((value, i) => {
    if (value === null || value === undefined || Number.isNaN(value)) return;
    const x = (i / (MAX_POINTS - 1)) * w;
    const y = h - ((value - dataMin) / span) * h;
    if (!started) {
      ctx.moveTo(x, y);
      started = true;
    } else {
      ctx.lineTo(x, y);
    }
  });
  ctx.stroke();
}

function updateCard(key, value) {
  const status = statusFor(key, value);
  const valueEl = document.querySelector(`[data-value="${key}"]`);
  const cardEl = document.querySelector(`[data-card="${key}"]`);
  const r = ranges[key];
  valueEl.textContent =
    value === null || value === undefined || Number.isNaN(value) ? "--" : `${value}${r.unit}`;
  cardEl.dataset.status = status;
}

function formatUptime(seconds) {
  if (seconds === null || seconds === undefined) return "--";
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = Math.floor(seconds % 60);
  return `${h}h ${m}m ${s}s`;
}

function redrawAll() {
  Object.keys(state).forEach((key) => {
    const canvas = document.querySelector(`[data-chart="${key}"]`);
    drawChart(canvas, state[key], ranges[key].min, ranges[key].max, colors[key]);
  });
}

function connect() {
  const host = location.hostname || "192.168.4.1";
  const socket = new WebSocket(`ws://${host}/ws`);
  const statusBadge = document.getElementById("connection-status");

  socket.onopen = () => {
    statusBadge.textContent = "Conectado";
    statusBadge.dataset.status = "ok";
  };

  socket.onclose = () => {
    statusBadge.textContent = "Desconectado — reintentando...";
    statusBadge.dataset.status = "alert";
    setTimeout(connect, 2000);
  };

  socket.onerror = () => socket.close();

  socket.onmessage = (event) => {
    let reading;
    try {
      reading = JSON.parse(event.data);
    } catch (e) {
      return;
    }

    Object.keys(ranges).forEach((key) => {
      const value = reading[key] ?? null;
      pushPoint(key, value);
      updateCard(key, value);
    });
    redrawAll();

    document.getElementById("uptime").textContent = formatUptime(reading.uptime);
  };
}

window.addEventListener("resize", redrawAll);

connect();
