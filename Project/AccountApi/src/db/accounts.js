const { GetCommand, PutCommand } = require('@aws-sdk/lib-dynamodb');
const { randomUUID: uuidv4 } = require('node:crypto');
const docClient = require('./client');
const config = require('../config');

const TABLE = config.tableName;

async function getAccount(platform, platformId) {
  const pk = `ACCOUNT#${platform}:${platformId}`;
  const result = await docClient.send(new GetCommand({
    TableName: TABLE,
    Key: { PK: pk, SK: pk },
  }));
  return result.Item || null;
}

async function createAccount(platform, platformId) {
  const now = new Date().toISOString();
  const pk = `ACCOUNT#${platform}:${platformId}`;

  const item = {
    PK: pk,
    SK: pk,
    type: 'account',
    accountId: uuidv4(),
    platform,
    platformId,
    createdAt: now,
    updatedAt: now,
  };

  try {
    await docClient.send(new PutCommand({
      TableName: TABLE,
      Item: item,
      ConditionExpression: 'attribute_not_exists(PK)',
    }));
    return item;
  } catch (err) {
    if (err.name === 'ConditionalCheckFailedException') {
      return getAccount(platform, platformId);
    }
    throw err;
  }
}

module.exports = { getAccount, createAccount };
