const express = require('express');
const charactersDb = require('../db/characters');

const router = express.Router();

router.get('/:platform/:id/character/:slot', async (req, res) => {
  try {
    const slot = parseInt(req.params.slot, 10);
    const char = await charactersDb.getCharacter(req.params.platform, req.params.id, slot);
    if (!char) {
      return res.status(404).json({ error: 'Character not found' });
    }
    delete char.PK;
    delete char.SK;
    delete char.type;
    res.json(char);
  } catch (err) {
    console.error('GET /character error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.post('/:platform/:id/character/:slot', async (req, res) => {
  try {
    const slot = parseInt(req.params.slot, 10);
    const existing = await charactersDb.getCharacter(req.params.platform, req.params.id, slot);
    if (existing) {
      return res.status(409).json({ error: 'Slot already occupied' });
    }

    const char = await charactersDb.putCharacter(req.params.platform, req.params.id, slot, {
      characterName: req.body.characterName || '',
    });
    delete char.PK;
    delete char.SK;
    delete char.type;
    res.status(201).json(char);
  } catch (err) {
    console.error('POST /character error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.put('/:platform/:id/character/:slot', async (req, res) => {
  try {
    const slot = parseInt(req.params.slot, 10);
    const char = await charactersDb.putCharacter(req.params.platform, req.params.id, slot, req.body);
    delete char.PK;
    delete char.SK;
    delete char.type;
    res.json(char);
  } catch (err) {
    console.error('PUT /character error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

router.delete('/:platform/:id/character/:slot', async (req, res) => {
  try {
    const slot = parseInt(req.params.slot, 10);
    await charactersDb.deleteCharacter(req.params.platform, req.params.id, slot);
    res.status(204).end();
  } catch (err) {
    console.error('DELETE /character error:', err);
    res.status(500).json({ error: 'Internal server error' });
  }
});

module.exports = router;
