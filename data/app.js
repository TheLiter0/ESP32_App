(() => {
  const W=240, H=180, MAX_UNDO=20;

  // ── Canvas ────────────────────────────────────────────────────────────────
  const c=document.getElementById('c'), ghost=document.getElementById('ghost');
  const ctx=c.getContext('2d',{willReadFrequently:true}), gctx=ghost.getContext('2d');
  c.width=ghost.width=W; c.height=ghost.height=H;

  // ── Zoom ─────────────────────────────────────────────────────────────────
  let zoom = 1.0;         // 1x to 4x
  const ZOOM_MIN = 1.0, ZOOM_MAX = 4.0, ZOOM_STEP = 0.25;
  let panX = 0, panY = 0; // pan offset when zoomed
  let panning = false, panStart = null;

  const canvasFrame = document.querySelector('.canvasFrame');

  const applyZoom = () => {
    // Scale the canvasFrame; the canvas elements inside scale with it
    canvasFrame.style.transform = `scale(${zoom})`;
    canvasFrame.style.transformOrigin = 'top center';
    // When zoomed, allow the area to be taller so it doesn't clip
    canvasFrame.parentElement.style.minHeight =
      zoom > 1 ? (canvasFrame.offsetHeight * zoom + 32) + 'px' : '';
  };

  // Mouse wheel zoom
  document.querySelector('.canvasArea').addEventListener('wheel', e => {
    e.preventDefault();
    const delta = e.deltaY < 0 ? ZOOM_STEP : -ZOOM_STEP;
    zoom = Math.min(ZOOM_MAX, Math.max(ZOOM_MIN, zoom + delta));
    applyZoom();
  }, { passive: false });

  // Pinch-to-zoom on touch
  let lastPinchDist = null;
  document.querySelector('.canvasArea').addEventListener('touchstart', e => {
    if (e.touches.length === 2) {
      lastPinchDist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
    }
  }, { passive: true });
  document.querySelector('.canvasArea').addEventListener('touchmove', e => {
    if (e.touches.length === 2 && lastPinchDist !== null) {
      const dist = Math.hypot(
        e.touches[0].clientX - e.touches[1].clientX,
        e.touches[0].clientY - e.touches[1].clientY
      );
      const ratio = dist / lastPinchDist;
      zoom = Math.min(ZOOM_MAX, Math.max(ZOOM_MIN, zoom * ratio));
      lastPinchDist = dist;
      applyZoom();
    }
  }, { passive: true });
  document.querySelector('.canvasArea').addEventListener('touchend', () => {
    lastPinchDist = null;
  }, { passive: true });

  // ── State ─────────────────────────────────────────────────────────────────
  let tool='pen', brushShape='round', brushSize=6, opacity=1;
  let undoStack=[], redoStack=[], drawing=false, startPt=null, last=null;

  // ── DOM ───────────────────────────────────────────────────────────────────
  const colorEl=document.getElementById('color');
  const bgEl=document.getElementById('bg');
  const brushEl=document.getElementById('brush');
  const brushValEl=document.getElementById('brushVal');
  const opEl=document.getElementById('opacity');
  const opValEl=document.getElementById('opacityVal');
  const logEl=document.getElementById('log');
  const statusEl=document.getElementById('status');
  const posEl=document.getElementById('cursorPos');
  const activeTool=document.getElementById('activeTool');
  const activeToolName=document.getElementById('activeToolName');

  const log=s=>{logEl.textContent+=`[${new Date().toLocaleTimeString()}] ${s}\n`;logEl.scrollTop=logEl.scrollHeight};
  const setStatus=s=>{statusEl.textContent=s};

  // ── Tool icons/names ──────────────────────────────────────────────────────
  const TOOL_META={
    pen:{icon:'✏️',name:'Pen'},eraser:{icon:'🧹',name:'Eraser'},
    fill:{icon:'🪣',name:'Fill'},eyedrop:{icon:'🔬',name:'Pick'},
    line:{icon:'╱',name:'Line'},rect:{icon:'▭',name:'Rect'},
    rectFill:{icon:'■',name:'Rect+'},ellipse:{icon:'⬭',name:'Oval'},
    ellipseFill:{icon:'⬤',name:'Oval+'},triangle:{icon:'△',name:'Tri'},
    triFill:{icon:'▲',name:'Tri+'},arrow:{icon:'→',name:'Arrow'},
    star:{icon:'★',name:'Star'},starFill:{icon:'⭐',name:'Star+'},spray:{icon:'💨',name:'Spray'},
  };

  // ── Undo/Redo ─────────────────────────────────────────────────────────────
  const saveUndo=()=>{undoStack.push(ctx.getImageData(0,0,W,H));if(undoStack.length>MAX_UNDO)undoStack.shift();redoStack=[]};
  const undo=()=>{if(!undoStack.length)return;redoStack.push(ctx.getImageData(0,0,W,H));ctx.putImageData(undoStack.pop(),0,0)};
  const redo=()=>{if(!redoStack.length)return;undoStack.push(ctx.getImageData(0,0,W,H));ctx.putImageData(redoStack.pop(),0,0)};

  // ── Palette ───────────────────────────────────────────────────────────────
  const PALETTE=['#000000','#ffffff','#ff0000','#00ff00','#0000ff','#ffff00',
    '#ff8800','#ff00ff','#00ffff','#8800ff','#ff0055','#00ff88',
    '#884400','#004488','#558800','#666666','#aaaaaa','#ffccaa',
    '#aaccff','#ffaacc','#ccffaa','#ffd700','#40e0d0','#dc143c'];
  const paletteEl=document.getElementById('palette');
  PALETTE.forEach(hex=>{
    const sw=document.createElement('button');
    sw.className='swatch';sw.style.background=hex;sw.title=hex;
    sw.addEventListener('click',e=>{if(e.shiftKey)bgEl.value=hex;else colorEl.value=hex});
    paletteEl.appendChild(sw);
  });

  // ── Popup open/close ──────────────────────────────────────────────────────
  const popup=document.getElementById('toolPopup');
  const overlay=document.getElementById('popupOverlay');
  const openPopup=()=>{popup.classList.remove('hidden');overlay.classList.remove('hidden')};
  const closePopup=()=>{popup.classList.add('hidden');overlay.classList.add('hidden')};
  document.getElementById('btnOpenTools').addEventListener('click',openPopup);
  overlay.addEventListener('click',closePopup);
  document.addEventListener('keydown',e=>{if(e.key==='Escape')closePopup()});

  // ── Tool selection ────────────────────────────────────────────────────────
  document.querySelectorAll('.popTool').forEach(btn=>{
    btn.addEventListener('click',()=>{
      document.querySelectorAll('.popTool').forEach(b=>b.classList.remove('active'));
      btn.classList.add('active');
      tool=btn.dataset.tool;
      const m=TOOL_META[tool]||{icon:'✏️',name:tool};
      activeTool.textContent=m.icon;
      activeToolName.textContent=m.name;
      document.getElementById('brushSec').style.display=
        (tool==='pen'||tool==='eraser'||tool==='spray')?'':'none';
      ghost.style.cursor=tool==='eyedrop'?'crosshair':tool==='fill'?'cell':'crosshair';
      closePopup();
    });
  });

  // ── Brush shape ───────────────────────────────────────────────────────────
  document.querySelectorAll('.chipBtn[data-shape]').forEach(btn=>{
    btn.addEventListener('click',()=>{
      document.querySelectorAll('.chipBtn[data-shape]').forEach(b=>b.classList.remove('active'));
      btn.classList.add('active');brushShape=btn.dataset.shape;
    });
  });

  // ── Swap colours ──────────────────────────────────────────────────────────
  document.getElementById('btnSwapColors').addEventListener('click',()=>{
    const t=colorEl.value;colorEl.value=bgEl.value;bgEl.value=t;
  });

  // ── Sliders ───────────────────────────────────────────────────────────────
  brushEl.addEventListener('input',()=>{brushSize=parseInt(brushEl.value);brushValEl.textContent=brushSize});
  opEl.addEventListener('input',()=>{opacity=parseInt(opEl.value)/100;opValEl.textContent=opEl.value+'%'});

  // ── Drawing helpers ───────────────────────────────────────────────────────
  const getPos=ev=>{
    const r=c.getBoundingClientRect();
    return{x:Math.round((ev.clientX-r.left)*(W/r.width)),y:Math.round((ev.clientY-r.top)*(H/r.height))};
  };

  const applyStyle=(cx,eraser=false)=>{
    cx.globalAlpha=eraser?1:opacity;
    cx.strokeStyle=eraser?bgEl.value:colorEl.value;
    cx.fillStyle=eraser?bgEl.value:colorEl.value;
    cx.lineWidth=brushSize;
    cx.lineCap=brushShape==='round'?'round':'square';
    cx.lineJoin=brushShape==='round'?'round':'miter';
  };

  const hexToRgb=hex=>{const v=parseInt(hex.slice(1),16);return[(v>>16)&255,(v>>8)&255,v&255]};

  // Flood fill
  const floodFill=(sx,sy,fillHex)=>{
    const img=ctx.getImageData(0,0,W,H),data=img.data;
    const idx=(x,y)=>(y*W+x)*4;
    const i0=idx(sx,sy);
    const tr=data[i0],tg=data[i0+1],tb=data[i0+2],ta=data[i0+3];
    const[fr,fg,fb]=hexToRgb(fillHex);
    if(tr===fr&&tg===fg&&tb===fb&&ta===255)return;
    const matches=i=>data[i]===tr&&data[i+1]===tg&&data[i+2]===tb&&data[i+3]===ta;
    const paint=i=>{data[i]=fr;data[i+1]=fg;data[i+2]=fb;data[i+3]=255};
    const stack=[idx(sx,sy)],vis=new Uint8Array(W*H);
    vis[sy*W+sx]=1;
    while(stack.length){
      const ci=stack.pop();if(!matches(ci))continue;paint(ci);
      const px=(ci/4)%W,py=Math.floor(ci/4/W);
      for(const ni of[px>0?ci-4:-1,px<W-1?ci+4:-1,py>0?ci-W*4:-1,py<H-1?ci+W*4:-1]){
        if(ni<0)continue;const nx=(ni/4)%W,ny=Math.floor(ni/4/W);
        if(!vis[ny*W+nx]){vis[ny*W+nx]=1;stack.push(ni);}
      }
    }
    ctx.putImageData(img,0,0);
  };

  const pickColor=(x,y)=>{const d=ctx.getImageData(x,y,1,1).data;return'#'+[d[0],d[1],d[2]].map(v=>v.toString(16).padStart(2,'0')).join('')};

  const clearGhost=()=>gctx.clearRect(0,0,W,H);

  // Draw star polygon helper
  const starPath=(cx,cy,r,n,inner)=>{
    ctx.beginPath();
    for(let i=0;i<n*2;i++){
      const angle=i*Math.PI/n - Math.PI/2;
      const rad=i%2===0?r:inner;
      i===0?ctx.moveTo(cx+rad*Math.cos(angle),cy+rad*Math.sin(angle))
           :ctx.lineTo(cx+rad*Math.cos(angle),cy+rad*Math.sin(angle));
    }
    ctx.closePath();
  };

  const drawGhost=(a,b)=>{
    clearGhost();applyStyle(gctx);gctx.globalAlpha=Math.min(opacity,.8);
    gctx.beginPath();
    if(tool==='line'){gctx.moveTo(a.x,a.y);gctx.lineTo(b.x,b.y);gctx.stroke();}
    else if(tool==='rect'){gctx.strokeRect(a.x,a.y,b.x-a.x,b.y-a.y);}
    else if(tool==='rectFill'){gctx.fillRect(a.x,a.y,b.x-a.x,b.y-a.y);}
    else if(tool==='ellipse'||tool==='ellipseFill'){
      const rx=Math.abs(b.x-a.x)/2,ry=Math.abs(b.y-a.y)/2;
      const cx=a.x+(b.x-a.x)/2,cy=a.y+(b.y-a.y)/2;
      gctx.ellipse(cx,cy,rx||1,ry||1,0,0,Math.PI*2);
      tool==='ellipseFill'?gctx.fill():gctx.stroke();
    }
    else if(tool==='triangle'||tool==='triFill'){
      gctx.moveTo((a.x+b.x)/2,a.y);gctx.lineTo(b.x,b.y);gctx.lineTo(a.x,b.y);gctx.closePath();
      tool==='triFill'?gctx.fill():gctx.stroke();
    }
    else if(tool==='arrow'){
      gctx.moveTo(a.x,a.y);gctx.lineTo(b.x,b.y);gctx.stroke();
      const ang=Math.atan2(b.y-a.y,b.x-a.x),hs=Math.max(8,brushSize*2);
      gctx.beginPath();
      gctx.moveTo(b.x,b.y);
      gctx.lineTo(b.x-hs*Math.cos(ang-0.4),b.y-hs*Math.sin(ang-0.4));
      gctx.moveTo(b.x,b.y);
      gctx.lineTo(b.x-hs*Math.cos(ang+0.4),b.y-hs*Math.sin(ang+0.4));
      gctx.stroke();
    }
  };

  // ── Pointer events ────────────────────────────────────────────────────────
  const onDown=ev=>{
    const p=getPos(ev);
    if(tool==='eyedrop'){colorEl.value=pickColor(p.x,p.y);return;}
    if(tool==='fill'){saveUndo();floodFill(p.x,p.y,colorEl.value);return;}
    if(tool==='spray'){saveUndo();}
    else{saveUndo();}
    drawing=true;startPt=p;last=p;
    if(tool==='pen'||tool==='eraser'){
      applyStyle(ctx,tool==='eraser');
      ctx.beginPath();ctx.arc(p.x,p.y,brushSize/2,0,Math.PI*2);ctx.fill();
    }
  };

  let sprayInterval=null;
  const doSpray=(p)=>{
    applyStyle(ctx);ctx.globalAlpha=0.08;
    const r=brushSize*2;
    for(let i=0;i<20;i++){
      const a=Math.random()*Math.PI*2,d=Math.random()*r;
      ctx.fillRect(p.x+d*Math.cos(a),p.y+d*Math.sin(a),1,1);
    }
  };

  const onMove=ev=>{
    const p=getPos(ev);posEl.textContent=`${p.x}, ${p.y}`;
    if(!drawing)return;
    if(tool==='pen'||tool==='eraser'){
      applyStyle(ctx,tool==='eraser');
      ctx.beginPath();ctx.moveTo(last.x,last.y);ctx.lineTo(p.x,p.y);ctx.stroke();
      last=p;
    } else if(tool==='spray'){
      doSpray(p);
    } else{
      drawGhost(startPt,p);
    }
  };

  const onUp=ev=>{
    if(!drawing)return;drawing=false;
    const p=ev?getPos(ev):last;
    clearGhost();
    if(!p||!startPt)return;
    applyStyle(ctx);ctx.beginPath();
    if(tool==='line'){ctx.moveTo(startPt.x,startPt.y);ctx.lineTo(p.x,p.y);ctx.stroke();}
    else if(tool==='rect'){ctx.strokeRect(startPt.x,startPt.y,p.x-startPt.x,p.y-startPt.y);}
    else if(tool==='rectFill'){ctx.fillRect(startPt.x,startPt.y,p.x-startPt.x,p.y-startPt.y);}
    else if(tool==='ellipse'||tool==='ellipseFill'){
      const rx=Math.abs(p.x-startPt.x)/2,ry=Math.abs(p.y-startPt.y)/2;
      const cx=startPt.x+(p.x-startPt.x)/2,cy=startPt.y+(p.y-startPt.y)/2;
      ctx.ellipse(cx,cy,rx||1,ry||1,0,0,Math.PI*2);
      tool==='ellipseFill'?ctx.fill():ctx.stroke();
    }
    else if(tool==='triangle'||tool==='triFill'){
      ctx.moveTo((startPt.x+p.x)/2,startPt.y);ctx.lineTo(p.x,p.y);ctx.lineTo(startPt.x,p.y);
      ctx.closePath();tool==='triFill'?ctx.fill():ctx.stroke();
    }
    else if(tool==='arrow'){
      ctx.moveTo(startPt.x,startPt.y);ctx.lineTo(p.x,p.y);ctx.stroke();
      const ang=Math.atan2(p.y-startPt.y,p.x-startPt.x),hs=Math.max(8,brushSize*2);
      ctx.beginPath();
      ctx.moveTo(p.x,p.y);ctx.lineTo(p.x-hs*Math.cos(ang-0.4),p.y-hs*Math.sin(ang-0.4));
      ctx.moveTo(p.x,p.y);ctx.lineTo(p.x-hs*Math.cos(ang+0.4),p.y-hs*Math.sin(ang+0.4));
      ctx.stroke();
    }
    else if(tool==='star'||tool==='starFill'){
      const r=Math.hypot(p.x-startPt.x,p.y-startPt.y);
      starPath(startPt.x,startPt.y,r,5,r*0.4);
      tool==='starFill'?ctx.fill():ctx.stroke();
    }
    startPt=null;last=null;
  };

  ghost.addEventListener('mousedown',onDown);
  ghost.addEventListener('mousemove',onMove);
  window.addEventListener('mouseup',ev=>onUp(ev));
  ghost.addEventListener('touchstart',e=>{e.preventDefault();onDown(e.touches[0])},{passive:false});
  ghost.addEventListener('touchmove',e=>{e.preventDefault();onMove(e.touches[0])},{passive:false});
  window.addEventListener('touchend',e=>onUp(e.changedTouches[0]));

  // ── Keyboard shortcuts ────────────────────────────────────────────────────
  document.addEventListener('keydown',e=>{
    if(e.target.tagName==='TEXTAREA'||e.target.tagName==='INPUT')return;
    if((e.ctrlKey||e.metaKey)&&e.key==='z'){e.preventDefault();undo();}
    if((e.ctrlKey||e.metaKey)&&(e.key==='y'||(e.shiftKey&&e.key==='z'))){e.preventDefault();redo();}
    const map={p:'pen',e:'eraser',f:'fill',l:'line',r:'rect',o:'ellipse',
               i:'eyedrop',t:'triangle',s:'star',a:'spray'};
    if(map[e.key]&&!e.ctrlKey&&!e.metaKey){
      document.querySelector(`.popTool[data-tool="${map[e.key]}"]`)?.click();
    }
    if(e.key==='['){brushEl.value=Math.max(1,brushSize-1);brushEl.dispatchEvent(new Event('input'));}
    if(e.key===']'){brushEl.value=Math.min(40,brushSize+1);brushEl.dispatchEvent(new Event('input'));}
  });

  // ── Buttons ───────────────────────────────────────────────────────────────
  // Zoom controls
  document.getElementById('btnZoomIn') ?.addEventListener('click', () => {
    zoom = Math.min(ZOOM_MAX, zoom + ZOOM_STEP); applyZoom();
  });
  document.getElementById('btnZoomOut')?.addEventListener('click', () => {
    zoom = Math.max(ZOOM_MIN, zoom - ZOOM_STEP); applyZoom();
  });
  document.getElementById('btnZoomReset')?.addEventListener('click', () => {
    zoom = 1; applyZoom();
  });

  document.getElementById('btnUndo').addEventListener('click',undo);
  document.getElementById('btnRedo').addEventListener('click',redo);
  document.getElementById('btnClear').addEventListener('click',()=>{
    saveUndo();ctx.fillStyle=bgEl.value;ctx.globalAlpha=1;ctx.fillRect(0,0,W,H);log('Cleared');
  });
  document.getElementById('btnFillBG').addEventListener('click',()=>{
    saveUndo();ctx.globalAlpha=1;ctx.fillStyle=colorEl.value;ctx.fillRect(0,0,W,H);log('Filled canvas with foreground colour');
  });
  document.getElementById('btnClearLog').addEventListener('click',()=>{logEl.textContent=''});
  document.getElementById('btnCopyLog').addEventListener('click',async()=>{
    await navigator.clipboard.writeText(logEl.textContent);log('Copied');
  });
  document.getElementById('btnDownload').addEventListener('click',()=>{
    c.toBlob(blob=>{
      const a=document.createElement('a');a.href=URL.createObjectURL(blob);
      a.download='esp32_canvas.png';a.click();URL.revokeObjectURL(a.href);log('Downloaded PNG');
    },'image/png');
  });
  document.getElementById('btnInfo').addEventListener('click',async()=>{
    try{
      const[h,info,fs]=await Promise.all([fetch('/health').then(r=>r.text()),fetch('/info').then(r=>r.text()),fetch('/fs').then(r=>r.text())]);
      log(`${info}  ${fs}`);
    }catch(e){log('Info failed: '+e);}
  });

  // ── Tabs ──────────────────────────────────────────────────────────────────
  document.querySelectorAll('.tab').forEach(btn=>{
    btn.addEventListener('click',()=>{
      document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
      document.querySelectorAll('.tabPanel').forEach(p=>p.classList.remove('active'));
      btn.classList.add('active');
      document.getElementById('tab-'+btn.dataset.tab).classList.add('active');
      if(btn.dataset.tab==='gallery')loadGallery();
      if(btn.dataset.tab==='settings')loadSettings();
    });
  });

  // ── Upload ────────────────────────────────────────────────────────────────
  const toRgb565=()=>{
    const px=ctx.getImageData(0,0,W,H).data,buf=new Uint8Array(W*H*2);
    for(let i=0;i<W*H;i++){
      const r=px[i*4],g=px[i*4+1],b=px[i*4+2];
      const v=((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3);
      buf[i*2]=(v>>8)&0xFF;buf[i*2+1]=v&0xFF;
    }
    return buf;
  };
  document.getElementById('btnUpload').addEventListener('click',async()=>{
    const btn=document.getElementById('btnUpload');
    btn.disabled=true;btn.textContent='Uploading…';log('Converting…');
    try{
      const buf=toRgb565();
      const port=(location.port===''||location.port==='80')?81:+location.port+1;
      const url=`${location.protocol}//${location.hostname}:${port}/upload`;
      log(`Sending ${buf.byteLength} bytes…`);
      const res=await fetch(url,{method:'POST',headers:{'Content-Type':'application/octet-stream'},body:buf});
      const j=await res.json().catch(()=>({}));
      if(res.ok){log(`Uploaded ✓${j.id?' → '+j.id:''}`);setStatus(`Online · uploaded ${new Date().toLocaleTimeString()}`);}
      else log('Upload failed: '+JSON.stringify(j));
    }catch(e){log('Error: '+e);}
    finally{btn.disabled=false;btn.textContent='⬆ Upload';}
  });

  // ── Gallery ───────────────────────────────────────────────────────────────
  const galleryGrid=document.getElementById('galleryGrid');
  const loadGallery=async()=>{
    galleryGrid.innerHTML='<div class="galleryEmpty">Loading…</div>';
    try{
      const[ir,fr]=await Promise.all([fetch('/api/images'),fetch('/fs')]);
      const list=await ir.json(),fsText=await fr.text();
      const m=fsText.match(/total=(\d+).*used=(\d+)/);
      if(m){
        const tot=+m[1],used=+m[2],pct=Math.round(used/tot*100);
        let sb=document.getElementById('storageBar');
        if(!sb){sb=document.createElement('div');sb.id='storageBar';
          sb.style.cssText='padding:6px 14px;font-size:11px;color:var(--muted)';
          galleryGrid.parentElement.insertBefore(sb,galleryGrid);}
        sb.innerHTML=`Storage: ${(used/1024).toFixed(0)} / ${(tot/1024).toFixed(0)} KB `+
          `<span style="color:${pct>85?'#ff6a6a':'#7cffc5'}">${pct}% used · ${((tot-used)/1024).toFixed(0)} KB free</span>`;
      }
      if(!list||list.length===0){galleryGrid.innerHTML='<div class="galleryEmpty">No saved images yet.</div>';return;}
      galleryGrid.innerHTML='';
      list.forEach(img=>{
        const card=document.createElement('div');card.className='galleryCard';card.dataset.id=img.id;
        card.innerHTML=`<div class="galleryThumb"><span>🖼</span></div>
          <div class="galleryInfo"><div class="galleryName">${img.name||img.id}</div>
          <div class="galleryMeta">${img.width}×${img.height} · ${(img.size/1024).toFixed(1)} KB</div></div>
          <button class="btn sm" data-action="show" data-id="${img.id}">Show</button>
          <button class="btn ghost sm" data-action="delete" data-id="${img.id}" style="color:var(--danger)">✕</button>`;
        card.querySelector('[data-action="show"]').addEventListener('click',async e=>{
          const b=e.currentTarget;b.disabled=true;b.textContent='…';
          try{const r=await fetch('/api/images/show',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id:b.dataset.id})});
            const j=await r.json();b.textContent=j.ok?'✓':'Err';}catch{b.textContent='Err';}
          setTimeout(()=>{b.disabled=false;b.textContent='Show';},2000);
        });
        card.querySelector('[data-action="delete"]').addEventListener('click',async e=>{
          const b=e.currentTarget,id=b.dataset.id;if(!confirm(`Delete ${id}?`))return;
          b.disabled=true;b.textContent='…';
          try{const r=await fetch('/api/images/delete',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id})});
            const j=await r.json();if(j.ok){document.querySelector(`.galleryCard[data-id="${id}"]`)?.remove();loadGallery();}
            else{b.textContent='Err';setTimeout(()=>{b.disabled=false;b.textContent='✕';},2000);}}
          catch{b.textContent='Err';setTimeout(()=>{b.disabled=false;b.textContent='✕';},2000);}
        });
        galleryGrid.appendChild(card);
      });
    }catch(e){galleryGrid.innerHTML=`<div class="galleryEmpty">Error: ${e}</div>`;}
  };
  document.getElementById('btnRefreshGallery').addEventListener('click',loadGallery);
  document.getElementById('btnReindexGallery').addEventListener('click',async()=>{
    const btn=document.getElementById('btnReindexGallery');btn.disabled=true;btn.textContent='…';
    try{const r=await fetch('/api/images/reindex',{method:'POST'});const j=await r.json();
      btn.textContent=j.ok?`✓ ${j.count}`:'Err';}catch{btn.textContent='Err';}
    setTimeout(()=>{btn.disabled=false;btn.textContent='🔧 Repair';},2000);loadGallery();
  });

  // ── Settings ──────────────────────────────────────────────────────────────
  const loadSettings=async()=>{
    try{
      const d=await fetch('/api/settings').then(r=>r.json());
      document.getElementById('settingClock').checked=!!d.clockEnabled;
      document.getElementById('settingSlideshow').checked=!!d.slideshowEnabled;
      document.getElementById('settingInterval').value=d.slideshowIntervalSec||30;
      document.getElementById('settingStatusMode').value=d.statusMode||'wifi';
      if (d.weatherLat)  document.getElementById('settingWeatherLat').value  = d.weatherLat;
      if (d.weatherLon)  document.getElementById('settingWeatherLon').value  = d.weatherLon;
      if (d.weatherCity) document.getElementById('settingWeatherCity').value = d.weatherCity;
      if (d.dateColor) document.getElementById('tcDate').value = '#' + d.dateColor;
      if (d.timeColor) document.getElementById('tcTime').value = '#' + d.timeColor;
      if (d.modeColor) document.getElementById('tcMode').value = '#' + d.modeColor;
      if (d.ipColor) document.getElementById('tcIp').value = '#' + d.ipColor;
      if (d.quoteColor) document.getElementById('tcQuote').value = '#' + d.quoteColor;
      if (d.weatherCityColor) document.getElementById('tcWeatherCity').value = '#' + d.weatherCityColor;
      if (d.weatherTempColor) document.getElementById('tcWeatherTemp').value = '#' + d.weatherTempColor;
      updateWeatherVisibility();
      if (d.topBarColor) {
        const v='#'+d.topBarColor;
        document.getElementById('settingTopBarColor').value=v;
        document.getElementById('settingTopBarHex').textContent=v;
      }
      if (d.statusBarColor) {
        const v='#'+d.statusBarColor;
        document.getElementById('settingStatusBarColor').value=v;
        document.getElementById('settingStatusBarHex').textContent=v;
      }
    }catch{document.getElementById('settingsStatus').textContent='Load failed';}
    // Load quotes
    try{
      const qData=await fetch('/config/quotes.json').then(r=>r.json());
      if(Array.isArray(qData))
        document.getElementById('settingQuotes').value=qData.join('\n');
    }catch{}
  };

  // Live hex label update for colour pickers
  // Show/hide weather fields based on status mode selection
  const updateWeatherVisibility = () => {
    const isWeather = document.getElementById('settingStatusMode').value === 'weather';
    ['weatherSection','weatherCoordsRow','weatherFetchRow'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.style.display = isWeather ? 'flex' : 'none';
    });
  };
  document.getElementById('settingStatusMode').addEventListener('change', updateWeatherVisibility);

  // Weather fetch function — uses Open-Meteo (free, no API key)
  const fetchWeather = async (lat, lon, city) => {
    const statusEl = document.getElementById('weatherStatus');
    if (statusEl) statusEl.textContent = 'Fetching weather…';
    try {
      // Open-Meteo free API — no key needed
      // Use 'weather_code' (newer API name) with fallback to 'weathercode'
      const url = `https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}` +
                  `&current=temperature_2m,relative_humidity_2m,weather_code` +
                  `&temperature_unit=fahrenheit&forecast_days=1`;

      const res = await fetch(url);
      if (!res.ok) throw new Error(`Open-Meteo returned ${res.status}`);

      const rawText = await res.text();
      let data;
      try { data = JSON.parse(rawText); }
      catch { throw new Error('Open-Meteo response was not JSON: ' + rawText.slice(0, 80)); }

      const cur = data.current;
      if (!cur) throw new Error('No current data in response: ' + JSON.stringify(data).slice(0, 120));

      // weather_code is the current field name; weathercode was the old one
      const code = cur.weather_code ?? cur.weathercode ?? 0;

      const WMO = {
        0:'Clear',1:'Mostly Clear',2:'Partly Cloudy',3:'Cloudy',
        45:'Foggy',48:'Icy Fog',51:'Light Drizzle',53:'Drizzle',55:'Heavy Drizzle',
        61:'Light Rain',63:'Rain',65:'Heavy Rain',71:'Light Snow',73:'Snow',75:'Heavy Snow',
        77:'Snow Grains',80:'Showers',81:'Showers',82:'Heavy Showers',
        85:'Snow Showers',86:'Heavy Snow Showers',
        95:'Thunderstorm',96:'Thunderstorm',99:'Thunderstorm',
      };
      const condition = WMO[code] || `Code ${code}`;
      const temp      = Math.round(cur.temperature_2m ?? 0);
      const humidity  = Math.round(cur.relative_humidity_2m ?? 0);

      log(`Open-Meteo: ${temp}°F ${condition} (code ${code}) ${humidity}% humidity`);

      // POST condensed data to ESP32
      const payload = {
        temp:      String(temp),
        condition: condition,
        humidity:  String(humidity),
        city:      city,
      };
      const postRes = await fetch('/api/weather', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify(payload),
      });

      // Read response as text first to avoid JSON.parse crash on unexpected response
      const postText = await postRes.text();
      let j = {};
      try { j = JSON.parse(postText); }
      catch { throw new Error('ESP32 weather save returned: ' + postText.slice(0, 80)); }

      if (statusEl) statusEl.textContent = j.ok
        ? `${temp}°F · ${condition} · ${humidity}% humidity`
        : `Save failed: ${JSON.stringify(j)}`;
      if (j.ok) log(`Weather saved: ${temp}°F ${condition} in ${city}`);

    } catch (e) {
      if (statusEl) statusEl.textContent = 'Error: ' + e.message;
      log('Weather error: ' + e);
    }
  };

  document.getElementById('btnFetchWeather')?.addEventListener('click', () => {
    const lat  = document.getElementById('settingWeatherLat').value;
    const lon  = document.getElementById('settingWeatherLon').value;
    const city = document.getElementById('settingWeatherCity').value;
    fetchWeather(lat, lon, city);
  });

  // Auto-fetch weather every 10 minutes when in weather mode
  setInterval(async () => {
    if (document.getElementById('settingStatusMode')?.value !== 'weather') return;
    const lat  = document.getElementById('settingWeatherLat')?.value;
    const lon  = document.getElementById('settingWeatherLon')?.value;
    const city = document.getElementById('settingWeatherCity')?.value;
    if (lat && lon) fetchWeather(lat, lon, city);
  }, 10 * 60 * 1000);

  ['TopBar','StatusBar'].forEach(name => {
    const pick = document.getElementById('setting'+name+'Color');
    const lbl  = document.getElementById('setting'+name+'Hex');
    if (pick && lbl) {
      pick.addEventListener('input', () => { lbl.textContent = pick.value; });
    }
  });

  document.getElementById('btnNextSlide').addEventListener('click',async()=>{
    const btn=document.getElementById('btnNextSlide');btn.disabled=true;btn.textContent='…';
    try{const r=await fetch('/api/slideshow/next',{method:'POST'});const j=await r.json();btn.textContent=j.ok?'✓':'Err';}
    catch{btn.textContent='Err';}
    setTimeout(()=>{btn.disabled=false;btn.textContent='▶ Next image';},1000);
  });

  document.getElementById('btnSaveSettings').addEventListener('click',async()=>{
    const statusEl2=document.getElementById('settingsStatus');
    statusEl2.textContent='Saving…';

    // 1. Save device settings
    const payload={
      clockEnabled:document.getElementById('settingClock').checked,
      slideshowEnabled:document.getElementById('settingSlideshow').checked,
      slideshowIntervalSec:parseInt(document.getElementById('settingInterval').value,10),
      statusMode:document.getElementById('settingStatusMode').value,
      weatherLat: document.getElementById('settingWeatherLat')?.value  || '42.93',
      weatherLon: document.getElementById('settingWeatherLon')?.value  || '-85.68',
      weatherCity:document.getElementById('settingWeatherCity')?.value || 'Grand Rapids',
      dateColor: (document.getElementById('tcDate')?.value||'#82a0dc').replace('#',''),
      timeColor: (document.getElementById('tcTime')?.value||'#f0f0c8').replace('#',''),
      modeColor: (document.getElementById('tcMode')?.value||'#50d2b4').replace('#',''),
      ipColor: (document.getElementById('tcIp')?.value||'#7896c8').replace('#',''),
      quoteColor: (document.getElementById('tcQuote')?.value||'#c8bea0').replace('#',''),
      weatherCityColor: (document.getElementById('tcWeatherCity')?.value||'#8cd2ff').replace('#',''),
      weatherTempColor: (document.getElementById('tcWeatherTemp')?.value||'#ffdc64').replace('#',''),
      topBarColor:document.getElementById('settingTopBarColor').value.replace('#',''),
      statusBarColor:document.getElementById('settingStatusBarColor').value.replace('#',''),
    };
    try{
      const r=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
      const j=await r.json();
      if(!j.ok){statusEl2.textContent='Settings save failed';return;}
    }catch(e){statusEl2.textContent='Error: '+e;return;}

    // 2. Save quotes via dedicated endpoint
    const rawQuotes=document.getElementById('settingQuotes').value;
    const quotesArr=rawQuotes.split('\n').map(s=>s.trim()).filter(s=>s.length>0);
    try{
      await fetch('/api/quotes',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(quotesArr)});
    }catch{}

    statusEl2.textContent='Saved ✓';
    // Auto-fetch weather after saving if weather mode is selected
    if (payload.statusMode === 'weather') {
      fetchWeather(payload.weatherLat, payload.weatherLon, payload.weatherCity);
    }
    setTimeout(()=>{statusEl2.textContent='';},3000);
  });

  // ── Init ──────────────────────────────────────────────────────────────────
  ctx.fillStyle=bgEl.value;ctx.globalAlpha=1;ctx.fillRect(0,0,W,H);

  (async()=>{
    try{await fetch('/health');setStatus('Online · connected');log('Connected');}
    catch{setStatus('Offline');log('Could not reach ESP32');}
  })();
})();