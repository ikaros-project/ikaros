# Idle

## Description

Sleeps during initialization for `init` seconds and during every tick for `duration` seconds. This
module is useful for scheduler, timing, and real-time behavior tests.

![Idle](Idle.svg)

## Parameters

| Name | Description | Type | Default |
| --- | --- | --- | --- |
| duration | Idle time per tick in seconds. | number | 0 |
| init | Startup delay in seconds. | number | 0 |
