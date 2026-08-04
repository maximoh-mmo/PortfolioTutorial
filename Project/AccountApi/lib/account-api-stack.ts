import * as cdk from 'aws-cdk-lib';
import * as dynamodb from 'aws-cdk-lib/aws-dynamodb';
import * as lambda from 'aws-cdk-lib/aws-lambda';
import * as lambdaNodejs from 'aws-cdk-lib/aws-lambda-nodejs';
import * as wafv2 from 'aws-cdk-lib/aws-wafv2';
import * as path from 'path';
import { config } from 'dotenv';

config();

export class AccountApiStack extends cdk.Stack {
  constructor(scope: cdk.App, id: string, props?: cdk.StackProps) {
    super(scope, id, props);

    const table = new dynamodb.Table(this, 'AccountTable', {
      partitionKey: { name: 'PK', type: dynamodb.AttributeType.STRING },
      sortKey: { name: 'SK', type: dynamodb.AttributeType.STRING },
      billingMode: dynamodb.BillingMode.PROVISIONED,
      readCapacity: 25,
      writeCapacity: 25,
      removalPolicy: cdk.RemovalPolicy.DESTROY,
    });

    table.addGlobalSecondaryIndex({
      indexName: 'GSI1',
      partitionKey: { name: 'GSI1PK', type: dynamodb.AttributeType.STRING },
      sortKey: { name: 'GSI1SK', type: dynamodb.AttributeType.STRING },
      readCapacity: 25,
      writeCapacity: 25,
    });

    const fn = new lambdaNodejs.NodejsFunction(this, 'ApiFunction', {
      entry: path.join(__dirname, '..', 'src', 'handler.js'),
      handler: 'handler',
      runtime: lambda.Runtime.NODEJS_22_X,
      memorySize: 256,
      timeout: cdk.Duration.seconds(10),
      environment: {
        TABLE_NAME: table.tableName,
        JWT_SECRET: process.env.JWT_SECRET || '',
        API_KEY: process.env.API_KEY || '',
        AUTH_TOKEN_SECRET: process.env.AUTH_TOKEN_SECRET || '',
      },
      loggingFormat: lambda.LoggingFormat.JSON,
      applicationLogLevel: lambda.ApplicationLogLevel.INFO,
      systemLogLevel: lambda.SystemLogLevel.INFO,
      tracing: lambda.Tracing.ACTIVE,
      bundling: {
        externalModules: ['@aws-sdk/client-dynamodb', '@aws-sdk/lib-dynamodb'],
      },
    });

    table.grantReadWriteData(fn);

    const fnUrl = fn.addFunctionUrl({
      authType: lambda.FunctionUrlAuthType.NONE,
    });

    const webAcl = new wafv2.CfnWebACL(this, 'AccountApiWebAcl', {
      defaultAction: { allow: {} },
      scope: 'REGIONAL',
      visibilityConfig: {
        cloudWatchMetricsEnabled: true,
        metricName: 'AccountApiWebAcl',
        sampledRequestsEnabled: true,
      },
      rules: [
        {
          name: 'RateLimitPerIp',
          priority: 1,
          statement: {
            rateBasedStatement: {
              limit: 100,
              aggregateKeyType: 'IP',
            },
          },
          action: { block: {} },
          visibilityConfig: {
            cloudWatchMetricsEnabled: true,
            metricName: 'RateLimitPerIp',
            sampledRequestsEnabled: true,
          },
        },
      ],
    });

    new wafv2.CfnWebACLAssociation(this, 'AccountApiWebAclAssociation', {
      webAclArn: webAcl.attrArn,
      resourceArn: fnUrl.functionArn,
    });

    new cdk.CfnOutput(this, 'ApiUrl', {
      value: fnUrl.url,
      description: 'Account API Lambda Function URL',
    });
  }
}
