import * as cdk from 'aws-cdk-lib';
import * as dynamodb from 'aws-cdk-lib/aws-dynamodb';
import * as lambda from 'aws-cdk-lib/aws-lambda';
import * as lambdaNodejs from 'aws-cdk-lib/aws-lambda-nodejs';
import * as path from 'path';
import { config } from 'dotenv';

config();

export class AccountApiStack extends cdk.Stack {
  constructor(scope: cdk.App, id: string, props?: cdk.StackProps) {
    super(scope, id, props);

    // Provisioned billing keeps the table inside the DynamoDB free tier (25 RCU / 25 WCU).
    // Auto-scaling (min 1) keeps the provisioned-capacity-hour meter near its minimum while
    // idle instead of burning the full 25/hr around the clock.
    const table = new dynamodb.Table(this, 'AccountTable', {
      partitionKey: { name: 'PK', type: dynamodb.AttributeType.STRING },
      sortKey: { name: 'SK', type: dynamodb.AttributeType.STRING },
      billingMode: dynamodb.BillingMode.PROVISIONED,
      readCapacity: 1,
      writeCapacity: 1,
      removalPolicy: cdk.RemovalPolicy.DESTROY,
    });

    table.autoScaleReadCapacity({ minCapacity: 1, maxCapacity: 5 })
      .scaleOnUtilization({ targetUtilizationPercent: 70 });
    table.autoScaleWriteCapacity({ minCapacity: 1, maxCapacity: 5 })
      .scaleOnUtilization({ targetUtilizationPercent: 70 });

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

    new cdk.CfnOutput(this, 'ApiUrl', {
      value: fnUrl.url,
      description: 'Account API Lambda Function URL',
    });
  }
}
