(() => {
  const W = 128;
  const H = 160;

  const c = document.getElementById("c");
  const ctx = c.getContext("2d", { willReadFrequently: true });

  // Internal pixel buffer size (actual resolution)
  c.width = W;
  c.height = H;

  const logEl = document.getElementById("log");
  const statusEl = document.getElementById("status");

  const brush = document.getElementById("brush");
  const brushVal = document.getElementById("brushVal");
  const color = document.getElementById("color");
  const bg = document.getElementById("bg");

  const sizeLabel = document.getElementById("sizeLabel");
  sizeLabel.textContent = `${W}×${H}`;

  const log = (s) => {
    const line = `[${new Date().toLocaleTimeString()}] ${s}\n`;
    logEl.textContent += line;
    logEl.scrollTop = logEl.scrollHeight;
  };

  const setStatus = (s) => {
    statusEl.textContent = s;
  };

  // Fill background
  const fillBG = () => {
    ctx.fillStyle = bg.value;
    ctx.fillRect(0, 0, W, H);
  };

  // Start with a filled background
  fillBG();

  // Drawing
  let drawing = false;
  let last = null;

  const getPos = (ev) => {
    const r = c.getBoundingClientRect();
    const x = (ev.clientX - r.left) * (W / r.width);
    const y = (ev.clientY - r.top) * (H / r.height);
    return { x, y };
  };

  const drawLine = (a, b) => {
    ctx.strokeStyle = color.value;
    ctx.lineWidth = parseInt(brush.value, 10);
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    ctx.beginPath();
    ctx.moveTo(a.x, a.y);
    ctx.lineTo(b.x, b.y);
    ctx.stroke();
  };

  const onDown = (ev) => {
    drawing = true;
    last = getPos(ev);
    drawLine(last, last);
  };
  const onMove = (ev) => {
    if (!drawing) return;
    const p = getPos(ev);
    drawLine(last, p);
    last = p;
  };
  const onUp = () => {
    drawing = false;
    last = null;
  };

  // Mouse
  c.addEventListener("mousedown", (e) => onDown(e));
  window.addEventListener("mousemove", (e) => onMove(e));
  window.addEventListener("mouseup", onUp);

  // Touch
  c.addEventListener("touchstart", (e) => {
    e.preventDefault();
    onDown(e.touches[0]);
  }, { passive: false });

  c.addEventListener("touchmove", (e) => {
    e.preventDefault();
    onMove(e.touches[0]);
  }, { passive: false });

  window.addEventListener("touchend", onUp);

  // UI bindings
  brush.addEventListener("input", () => brushVal.textContent = brush.value);
  document.getElementById("btnFill").addEventListener("click", () => {
    fillBG();
    log("Filled background");
  });

  document.getElementById("btnClear").addEventListener("click", () => {
    fillBG();
    log("Cleared canvas");
  });

  document.getElementById("btnClearLog").addEventListener("click", () => {
    logEl.textContent = "";
  });

  document.getElementById("btnCopyLog").addEventListener("click", async () => {
    await navigator.clipboard.writeText(logEl.textContent);
    log("Copied log to clipboard");
  });

  document.getElementById("btnDownload").addEventListener("click", () => {
    c.toBlob((blob) => {
      const a = document.createElement("a");
      a.href = URL.createObjectURL(blob);
      a.download = "esp32_canvas.png";
      a.click();
      URL.revokeObjectURL(a.href);
      log("Downloaded PNG");
    }, "image/png");
  });

  // Device info
  document.getElementById("btnInfo").addEventListener("click", async () => {
    try {
      const [health, info, fs] = await Promise.all([
        fetch("/health").then(r => r.text()),
        fetch("/info").then(r => r.text()),
        fetch("/fs").then(r => r.text()),
      ]);
      log(`/health -> ${health}`);
      log(`/info   -> ${info}`);
      log(`/fs     -> ${fs}`);
      setStatus(`Online • ${info}`);
    } catch (e) {
      log(`Info failed: ${e}`);
      setStatus("Offline");
    }
  });

  // Upload (requires ESP32 POST /upload handler - not yet implemented in firmware)
  document.getElementById("btnUpload").addEventListener("click", async () => {
  setStatus("Uploading…");
  log("Upload started (PNG)");

  try {
    const blob = await new Promise((resolve) => c.toBlob(resolve, "image/png"));

    const fd = new FormData();
    fd.append("file", blob, "canvas.png");

    const res = await fetch("/upload", {
      method: "POST",
      body: fd
    });

    const text = await res.text();
    log(`/upload -> ${res.status} ${text}`);
    setStatus(res.ok ? "Upload complete" : "Upload failed");
  } catch (e) {
    log(`Upload failed: ${e}`);
    setStatus("Upload failed");
  }
});

  // Auto ping once
  (async () => {
    try {
      const health = await fetch("/health").then(r => r.text());
      setStatus(`Online • /health=${health}`);
      log("Connected to ESP32 web server");
    } catch {
      setStatus("Offline");
      log("Could not reach ESP32");
    }
  })();
})();