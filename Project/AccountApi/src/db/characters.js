const { GetCommand, PutCommand, DeleteCommand, QueryCommand } = require('@aws-sdk/lib-dynamodb');
const docClient = require('./client');
const config = require('../config');

const TABLE = config.tableName;

function accountPK(platform, platformId) {
  return `ACCOUNT#${platform}:${platformId}`;
}

async function getCharacter(platform, platformId, slotIndex) {
  const result = await docClient.send(new GetCommand({
    TableName: TABLE,
    Key: {
      PK: accountPK(platform, platformId),
      SK: `CHARACTER#${slotIndex}`,
    },
  }));
  return result.Item || null;
}

async function putCharacter(platform, platformId, slotIndex, data) {
  const now = new Date().toISOString();
  const item = {
    PK: accountPK(platform, platformId),
    SK: `CHARACTER#${slotIndex}`,
    type: 'character',
    slotIndex: parseInt(slotIndex, 10),
    characterName: data.characterName || '',
    level: data.level || 1,
    experience: data.experience || 0,
    currentZone: data.currentZone || '',
    savedMaxHealth: data.savedMaxHealth ?? 100.0,
    savedPosition: data.savedPosition || { x: 0, y: 0, z: 0 },
    savedRotationYaw: data.savedRotationYaw ?? 0.0,
    inventoryJson: data.inventoryJson || '{}',
    equipmentJson: data.equipmentJson || '{}',
    questsJson: data.questsJson || '{}',
    createdAt: data.createdAt || now,
    updatedAt: now,
  };

  await docClient.send(new PutCommand({
    TableName: TABLE,
    Item: item,
  }));

  return item;
}

async function deleteCharacter(platform, platformId, slotIndex) {
  await docClient.send(new DeleteCommand({
    TableName: TABLE,
    Key: {
      PK: accountPK(platform, platformId),
      SK: `CHARACTER#${slotIndex}`,
    },
  }));
}

async function listCharacters(platform, platformId) {
  const result = await docClient.send(new QueryCommand({
    TableName: TABLE,
    KeyConditionExpression: 'PK = :pk AND begins_with(SK, :prefix)',
    ExpressionAttributeValues: {
      ':pk': accountPK(platform, platformId),
      ':prefix': 'CHARACTER#',
    },
  }));
  return result.Items || [];
}

module.exports = { getCharacter, putCharacter, deleteCharacter, listCharacters };
