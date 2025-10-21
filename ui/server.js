/*
  UI server sin dependencias:
  - Sirve estáticos desde ./ui/public (por stream)
  - GET  /health              -> { ok, sim, exists }
  - POST /run  (JSON body)    -> ejecuta ./build/sim y devuelve { ok, log, csv_file, header, rows, sum_result }
  - GET  /csv?file=<nombre>   -> descarga CSV generado (desde ./ui/output)
*/
const http = require('http');
const fs   = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const ROOT    = path.resolve(__dirname, '..');              // raíz del repo
const SIM     = path.join(ROOT, 'build', 'sim');            // ejecutable
const PUB_DIR = path.join(__dirname, 'public');             // estáticos
const OUT_DIR = path.join(__dirname, 'output');             // CSVs
fs.mkdirSync(OUT_DIR, { recursive: true });

function contentType(file) {
  const ext = path.extname(file).toLowerCase();
  return ({
    '.html':'text/html; charset=utf-8',
    '.css' :'text/css; charset=utf-8',
    '.js'  :'application/javascript; charset=utf-8',
    '.csv' :'text/csv; charset=utf-8'
  })[ext] || 'application/octet-stream';
}

function json(res, code, obj) {
  res.writeHead(code, { 'Content-Type':'application/json; charset=utf-8', 'Cache-Control':'no-store' });
  res.end(JSON.stringify(obj));
}

function serveStatic(req, res) {
  const url = new URL(req.url, 'http://localhost');
  let p = url.pathname;
  if (p === '/') p = '/index.html';
  const file = path.normalize(path.join(PUB_DIR, p));
  if (!file.startsWith(PUB_DIR)) return json(res, 403, { ok:false, error:'Forbidden' });

  fs.stat(file, (err, st) => {
    if (err || !st.isFile()) return json(res, 404, { ok:false, error:'Not found' });
    res.writeHead(200, { 'Content-Type': contentType(file), 'Cache-Control':'no-store' });
    const rs = fs.createReadStream(file);
    rs.on('error', () => json(res, 500, { ok:false, error:'read error' }));
    rs.pipe(res);
  });
}

function parseBody(req, cb) {
  let buf=''; req.on('data', c => buf += c);
  req.on('end', () => {
    try { cb(null, JSON.parse(buf || '{}')); }
    catch (e) { cb(e); }
  });
}

// CSV parser robusto (config_str puede traer comas)
function parseCSVSmart(text) {
  const lines = text.trim().split(/\r?\n/);
  if (!lines.length) return { header:[], rows:[] };
  const header = lines[0].split(',');
  const H = header.length;
  const rows = [];
  for (let i=1;i<lines.length;i++){
    const raw = lines[i]; if (!raw) continue;
    let t = raw.split(',');
    if (t.length > H) {
      const extra = t.length - H;
      const cfg = [t[1], ...t.slice(2, 2+extra)].join(',');
      t = [t[0], cfg, ...t.slice(2+extra)];
    }
    if (t.length !== H) continue;
    const o = {};
    for (let j=0;j<H;j++) o[header[j]] = (t[j] ?? '').trim();
    rows.push(o);
  }
  return { header, rows };
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost');

  // Health
  if (req.method === 'GET' && url.pathname === '/health') {
    return json(res, 200, { ok:true, sim:SIM, exists: fs.existsSync(SIM) });
  }

  // Descargar CSV
  if (req.method === 'GET' && url.pathname === '/csv') {
    const name = path.basename(url.searchParams.get('file') || '');
    const file = path.join(OUT_DIR, name);
    if (!name || !fs.existsSync(file)) return json(res, 404, { ok:false, error:'CSV no encontrado' });
    res.writeHead(200, { 'Content-Type':'text/csv; charset=utf-8', 'Cache-Control':'no-store' });
    return fs.createReadStream(file).pipe(res);
  }

  // Ejecutar simulador
  if (req.method === 'POST' && url.pathname === '/run') {
    return parseBody(req, (err, body) => {
      if (err) return json(res, 400, { ok:false, error:'JSON inválido' });
      if (!fs.existsSync(SIM)) return json(res, 400, { ok:false, error:'No existe ./build/sim (corre ./scripts/build.sh)' });

      const arch    = (body.arch==='l1bus') ? 'l1bus' : 'mock';
      const N       = Number(body.N || 32);
      const align32 = !!body.align32;
      const seq     = !!body.seq;
      const debug   = !!body.debug;
      const outName = body.out || `run_${Date.now()}.csv`;
      const outAbs  = path.join(OUT_DIR, path.basename(outName));

      const args = ['--arch', arch, '--N', String(N)];
      if (align32) args.push('--align32');
      if (seq)     args.push('--seq');
      if (debug)   args.push('--debug');
      args.push('--out', outAbs);

      let stdout='', stderr='';
      const child = spawn(SIM, args, { cwd: ROOT });
      child.stdout.on('data', d => stdout += d.toString());
      child.stderr.on('data', d => stderr += d.toString());
      child.on('error', e => json(res, 500, { ok:false, error:e.message }));

      child.on('close', code => {
        if (code !== 0) return json(res, 500, { ok:false, error:`sim exit ${code}`, log: stdout + stderr });
        let csv=''; try { csv = fs.readFileSync(outAbs, 'utf8'); } catch (e) { return json(res, 500, { ok:false, error:'CSV no leído', log: stdout + stderr }); }
        const { header, rows } = parseCSVSmart(csv);
        const sum = rows.reduce((s,r)=> s + Number(r.result||0), 0);
        return json(res, 200, { ok:true, args, log: stdout, csv_file: path.basename(outAbs), header, rows, sum_result: sum });
      });
    });
  }

  // Estáticos
  return serveStatic(req, res);
});

const PORT = 5173;
server.listen(PORT, () => console.log(`UI server en http://localhost:${PORT}`));
