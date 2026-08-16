const express = require('express');
const accountsDb = require('../db/accounts');
const charactersDb = require('../db/characters');
const authMiddleware = require('../middleware/auth');

const router = express.Router();

router.get('/:platform/:id', authMiddleware, async (req, res) => {
  try {
    const account = await accountsDb.getAccount(req.params.platform, req.params.id);
    if (!account) {
      return res.status(404).json({ error: 'Account not found' });
    }

    const chars = await charactersDb.listCharacters(req.params.platform, req.params.id);

    const slots = [];
    for (let i = 0; i < 3; i++) {
      const char = chars.find(c => c.slotIndex === i);
      slots.push({
        slotIndex: i,
        characterName: char?.characterName || '',
        level: char?.level || 1,
        bOccupied: !!char,
      });
    }

    res.json({
      platform: account.platform,
      platformId: account.platformId,
      slots,
    });
  } catch (err) {
    console.error('GET /account/:platform/:id error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.post('/:platform/:id', authMiddleware, async (req, res) => {
  try {
    const account = await accountsDb.createAccount(req.params.platform, req.params.id);
    res.status(201).json({
      platform: account.platform,
      platformId: account.platformId,
      slots: [],
    });
  } catch (err) {
    console.error('POST /account/:platform/:id error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
