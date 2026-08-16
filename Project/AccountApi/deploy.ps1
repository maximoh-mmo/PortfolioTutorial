# Deploys the Account API with CLOUD config.
# The local .env holds local-dev values (DYNAMODB_ENDPOINT, ENABLE_METRICS, local secret)
# and must NOT be pushed into the Lambda. This script seeds the stack with the cloud
# values from .env.cloud (if present) or .env.example, which dotenv will not override.
$ErrorActionPreference = 'Stop'

$src = if (Test-Path -LiteralPath ".env.cloud") { ".env.cloud" } else { ".env.example" }
$vars = @{}
Get-Content -LiteralPath $src | ForEach-Object {
  if ($_ -match '^\s*([A-Za-z0-9_]+)=(.*)$') {
    $vars[$matches[1]] = $matches[2]
  }
}

foreach ($key in 'AUTH_TOKEN_SECRET', 'API_KEY', 'JWT_SECRET') {
  if ($vars.ContainsKey($key)) {
    Set-Item -Path "Env:$key" -Value $vars[$key]
    Write-Host "$key = $($vars[$key])"
  }
}

npx cdk deploy --require-approval never