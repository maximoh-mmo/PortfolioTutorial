const express = require('express');
const jwt = require('jsonwebtoken');
const config = require('../config');

const router = express.Router();

router.post('/validate-token', (req, res) => {
  const { token } = req.body;
  if (!token) {
    return res.status(400).json({ error: 'Token required' });
  }

  try {
    const decoded = jwt.verify(token, config.jwtSecret);
    res.json({
      platform: decoded.platform,
      platformId: decoded.platformId,
    });
  } catch (err) {
    res.status(401).json({ error: 'Invalid or expired token' });
  }
});

module.exports = router;
