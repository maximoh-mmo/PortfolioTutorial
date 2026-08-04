const crypto = require('crypto');
const config = require('../config');

function decodeStoreToken(token, platform, platformId) {
  if (!token || !config.authTokenSecret) return null;

  const parts = String(token).split('.');
  if (parts.length !== 2) return null;

  let payloadBytes;
  try {
    payloadBytes = Buffer.from(parts[0], 'base64');
  } catch (err) {
    return null;
  }
  const payload = payloadBytes.toString('utf8');

  const expected = crypto
    .createHmac('sha256', config.authTokenSecret)
    .update(payloadBytes)
    .digest('base64');

  if (expected !== parts[1]) return null;

  const fields = payload.split('|');
  if (fields.length !== 4) return null;

  const tokenPlatformId = fields[0];
  const tokenPlatform = fields[1];
  const tokenSlotIndex = parseInt(fields[2], 10);
  const expiry = parseInt(fields[3], 10);

  if (!expiry || expiry < Math.floor(Date.now() / 1000)) return null;

  if (tokenPlatform !== platform || tokenPlatformId !== platformId) return null;

  return { platform: tokenPlatform, platformId: tokenPlatformId, slotIndex: tokenSlotIndex };
}

module.exports = function authMiddleware(req, res, next) {
  const apiKey = req.headers['x-api-key'];
  if (!apiKey || apiKey !== config.apiKey) {
    return res.status(401).json({ error: 'Unauthorized' });
  }

  const storeToken = decodeStoreToken(req.headers['x-store-token'], req.params.platform, req.params.id);
  if (!storeToken) {
    return res.status(403).json({ error: 'Forbidden: invalid or mismatched store token' });
  }

  req.storeAuth = storeToken;
  next();
};
