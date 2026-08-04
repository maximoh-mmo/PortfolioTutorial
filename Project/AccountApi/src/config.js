module.exports = {
  tableName: process.env.TABLE_NAME || 'onset-accounts-dev',
  jwtSecret: process.env.JWT_SECRET || 'dev-secret',
  apiKey: process.env.API_KEY || 'dev-api-key',
  authTokenSecret: process.env.AUTH_TOKEN_SECRET || '',
};
