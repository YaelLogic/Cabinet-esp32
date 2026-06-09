param(
    [string]$Broker = "localhost",
    [string]$StationId = "03",
    [string]$DoorId = "door-1",
    [string]$ActivityId = "activity-test-1"
)

$topic = "shita/stations/$StationId/doors/command"
$payload = "{`"doorId`":`"$DoorId`",`"activityId`":`"$ActivityId`",`"time`":`"$(Get-Date -Format o)`"}"

mosquitto_pub -h $Broker -t $topic -m $payload -q 1
Write-Host "Published to $topic"
Write-Host $payload

Write-Host "To listen for ESP status:"
Write-Host "mosquitto_sub -h $Broker -t shita/stations/$StationId/doors/status -v"
