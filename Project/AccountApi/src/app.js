const express = require('express');
const authMiddleware = require('./middleware/auth');
const accountsRouter = require('./routes/accounts');
const charactersRouter = require('./routes/characters');
const authRouter = require('./routes/auth');
const { enabled: metricsEnabled, metricsMiddleware, snapshot } = require('./metrics');

const app = express();

app.use(express.json());

if (metricsEnabled) {
  app.use(metricsMiddleware);
  app.get('/local/metrics', (_req, res) => {
    res.json(snapshot());
  });
}

app.get('/health', (_req, res) => {
  res.json({ status: 'ok' });
});

app.use('/auth', authRouter);
app.use('/account', accountsRouter);
app.use('/account', charactersRouter);

module.exports = app;
