# Architecture Decisions and Rationale

## Decision 1: Separate MQTTManager Class
**Rationale**: Maintains single responsibility principle, allows MQTT to be completely disabled without affecting core functionality.

## Decision 2: Event-Driven Integration
**Rationale**: Minimizes changes to existing code, reduces coupling, easier to test.

## Decision 3: Static Memory Allocation
**Rationale**: Predictable memory usage, no fragmentation, suitable for embedded systems.

## Decision 4: Phased Feature Deployment
**Rationale**: Reduces risk, allows monitoring of each feature's impact, easier rollback.

## Decision 5: Configuration via Web UI
**Rationale**: Consistent with existing system, no need for recompilation, user-friendly.
