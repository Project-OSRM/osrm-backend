var cf = require('@mapbox/cloudfriend');
var package_json = require('../package.json')

module.exports = {
  AWSTemplateFormatVersion: '2010-09-09',
  Description: 'user for publishing to s3://mapbox-node-binary/' + package_json.name,
  Resources: {
    User: {
      Type: 'AWS::IAM::User',
      Properties: {
        Policies: [
          {
            PolicyName: 'list',
            PolicyDocument: {
              Statement: [
                {
                  Action: ['s3:ListBucket'],
                  Effect: 'Allow',
                  Resource: 'arn:aws:s3:::mapbox-node-binary',
                  Condition : {
                    StringLike : {
                      "s3:prefix": [ package_json.name + "/*"]
                    }
                  }
                }
              ]
            }
          },
          {
            PolicyName: 'publish',
            PolicyDocument: {
              Statement: [
                {
                  Action: ['s3:DeleteObject', 's3:GetObject', 's3:GetObjectAcl', 's3:PutObject', 's3:PutObjectAcl'],
                  Effect: 'Allow',
                  Resource: 'arn:aws:s3:::mapbox-node-binary/' + package_json.name + '/*'
                }
              ]
            }
          }
        ]
      }
    },
    AccessKey: {
      Type: 'AWS::IAM::AccessKey',
      Properties: {
        UserName: cf.ref('User')
      }
    }
  },
  Outputs: {
    AccessKeyId: {
      Value: cf.ref('AccessKey')
    },
    SecretAccessKey: {
      Value: cf.getAtt('AccessKey', 'SecretAccessKey')
    }
  }
};
