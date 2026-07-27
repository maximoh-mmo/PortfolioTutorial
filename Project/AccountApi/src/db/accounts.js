const { GetCommand, PutCommand } = require('@aws-sdk/lib-dynamodb');
const { v4: uuidv4 } = require('uuid');
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

  if (platform === 'steam') {
    item.GSI1PK = `STEAM#${platformId}`;
    item.GSI1SK = pk;
  }

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

async function getAccountBySteam(steamId) {
  const { QueryCommand } = require('@aws-sdk/lib-dynamodb');
  const result = await docClient.send(new QueryCommand({
    TableName: TABLE,
    IndexName: 'GSI1',
    KeyConditionExpression: 'GSI1PK = :pk',
    ExpressionAttributeValues: {
      ':pk': `STEAM#${steamId}`,
    },
    Limit: 1,
  }));
  return result.Items?.[0] || null;
}

module.exports = { getAccount, createAccount, getAccountBySteam };
