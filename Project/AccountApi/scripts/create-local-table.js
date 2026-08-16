require('dotenv').config();
const { DynamoDBClient, CreateTableCommand } = require('@aws-sdk/client-dynamodb');
const config = require('../src/config');

const endpoint = process.env.DYNAMODB_ENDPOINT;

const client = new DynamoDBClient(
  endpoint
    ? { endpoint, region: 'us-east-1', credentials: { accessKeyId: 'local', secretAccessKey: 'local' } }
    : {}
);

async function main() {
  await client.send(
    new CreateTableCommand({
      TableName: config.tableName,
      AttributeDefinitions: [
        { AttributeName: 'PK', AttributeType: 'S' },
        { AttributeName: 'SK', AttributeType: 'S' },
      ],
      KeySchema: [
        { AttributeName: 'PK', KeyType: 'HASH' },
        { AttributeName: 'SK', KeyType: 'RANGE' },
      ],
      ProvisionedThroughput: { ReadCapacityUnits: 25, WriteCapacityUnits: 25 },
    })
  );
  console.log(`Created local table '${config.tableName}' (provisioned 25/25)`);
}

main().catch((err) => {
  if (err.name === 'ResourceInUseException') {
    console.log(`Table '${config.tableName}' already exists`);
    process.exit(0);
  }
  console.error(err);
  process.exit(1);
});