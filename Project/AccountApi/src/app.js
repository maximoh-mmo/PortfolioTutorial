const express = require('express');
const authMiddleware = require('./middleware/auth');
const accountsRouter = require('./routes/accounts');
const charactersRouter = require('./routes/characters');
const authRouter = require('./routes/auth');

const app = express();

app.use(express.json());

app.get('/health', (_req, res) => {
  res.json({ status: 'ok' });
});

app.use('/auth', authRouter);
app.use('/account', authMiddleware);
app.use('/account', accountsRouter);
app.use('/account', charactersRouter);

module.exports = app;
