const { DynamoDBClient } = require('@aws-sdk/client-dynamodb');
const { DynamoDBDocumentClient } = require('@aws-sdk/lib-dynamodb');
const { recordConsumed } = require('../metrics');

const endpoint = process.env.DYNAMODB_ENDPOINT;
const enableMetrics = process.env.ENABLE_METRICS === '1';

const clientConfig = endpoint
  ? {
      endpoint,
      region: process.env.AWS_REGION || 'us-east-1',
      credentials: { accessKeyId: 'local', secretAccessKey: 'local' },
    }
  : {};

const client = new DynamoDBClient(clientConfig);

const docClient = DynamoDBDocumentClient.from(client, {
  marshallOptions: {
    removeUndefinedValues: true,
    convertClassInstanceToMap: true,
  },
  ...(enableMetrics ? { sendCommandOptions: { ReturnConsumedCapacity: 'TOTAL' } } : {}),
});

if (enableMetrics) {
  const send = docClient.send.bind(docClient);
  docClient.send = async (command, options) => {
    const response = await send(command, options);
    recordConsumed(command, response);
    return response;
  };
}

module.exports = docClient;