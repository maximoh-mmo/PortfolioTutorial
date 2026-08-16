const { AsyncLocalStorage } = require('node:async_hooks');

const enabled = process.env.ENABLE_METRICS === '1';
const als = new AsyncLocalStorage();

const totals = { requests: 0, rcu: 0, wcu: 0, byPath: {} };
const startTime = Date.now();
let windowStart = startTime;
let windowCount = 0;
let reqPerSec = 0;
let peakReqPerSec = 0;

function estimateConsumed(command, response) {
  const name = command.constructor && command.constructor.name;
  const input = command.input || {};
  const readSize = 4096;
  const writeSize = 1024;
  const rounded = (bytes, unit) => Math.max(1, Math.ceil((bytes || 1) / unit));

  switch (name) {
    case 'GetCommand':
      return {
        rcu: response && response.Item ? rounded(JSON.stringify(response.Item).length, readSize) : 1,
        wcu: 0,
      };
    case 'PutCommand':
      return { rcu: 0, wcu: rounded(JSON.stringify(input.Item || {}).length, writeSize) };
    case 'DeleteCommand':
      return { rcu: 0, wcu: 1 };
    case 'QueryCommand': {
      const items = (response && response.Items) || [];
      const size = items.reduce((s, it) => s + JSON.stringify(it).length, 0);
      const rcu = rounded(size, readSize);
      return { rcu: input.ConsistentRead ? rcu * 2 : rcu, wcu: 0 };
    }
    default:
      return { rcu: 0, wcu: 0 };
  }
}

function recordConsumed(command, response) {
  if (!enabled) return;

  let rcu = 0;
  let wcu = 0;
  const cap = response && response.ConsumedCapacity;
  if (cap) {
    rcu = cap.ReadCapacityUnits || 0;
    wcu = cap.WriteCapacityUnits || 0;
  } else {
    const est = estimateConsumed(command, response);
    rcu = est.rcu;
    wcu = est.wcu;
  }

  totals.rcu += rcu;
  totals.wcu += wcu;

  const ctx = als.getStore();
  if (ctx) {
    ctx.rcu += rcu;
    ctx.wcu += wcu;
  }
}

function metricsMiddleware(req, res, next) {
  if (!enabled) return next();

  const ctx = { rcu: 0, wcu: 0 };
  const start = process.hrtime.bigint();

  als.run(ctx, () => {
    res.on('finish', () => {
      const ms = Number(process.hrtime.bigint() - start) / 1e6;
      const path = req.originalUrl;

      totals.requests += 1;
      totals.byPath[path] = (totals.byPath[path] || 0) + 1;

      windowCount += 1;
      const now = Date.now();
      const elapsed = (now - windowStart) / 1000;
      if (elapsed >= 1) {
        reqPerSec = windowCount / elapsed;
        if (reqPerSec > peakReqPerSec) peakReqPerSec = reqPerSec;
        windowStart = now;
        windowCount = 0;
      }

      console.log(`[metrics] ${req.method} ${path} ${res.statusCode} ${ms.toFixed(1)}ms rcu=${ctx.rcu} wcu=${ctx.wcu}`);
    });
    next();
  });
}

function snapshot() {
  return {
    startedAt: new Date(startTime).toISOString(),
    uptimeSec: Math.round((Date.now() - startTime) / 1000),
    requests: totals.requests,
    rcu: totals.rcu,
    wcu: totals.wcu,
    reqPerSec: Math.round(reqPerSec * 100) / 100,
    peakReqPerSec: Math.round(peakReqPerSec * 100) / 100,
    byPath: totals.byPath,
  };
}

module.exports = { enabled, metricsMiddleware, recordConsumed, snapshot };