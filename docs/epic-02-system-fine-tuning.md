# Epic 2: System Fine-tuning and Additional Features

**Epic ID**: EPIC-002  
**Priority**: High  
**Status**: In Planning  
**Target Release**: MVP Phase 2  

## Epic Overview

Complete the system enhancement with Russian language localization, QR code display functionality for easy access, Modbus register 899 safety implementation, and critical performance improvements to the event logs viewer.

## Business Value

- **Accessibility**: Russian interface for local operators
- **Ease of Use**: QR code scanning for instant web access
- **Safety**: Secure Modbus configuration commands
- **Performance**: Non-blocking event log viewing
- **User Experience**: Seamless multi-language support

## Success Criteria

1. ✅ Complete Russian translation of all UI elements
2. ✅ QR code displays device URL on OLED
3. ✅ WiFi setup QR in AP mode
4. ✅ Modbus register 899 fully functional with safety
5. ✅ Event logs viewer doesn't suspend controller
6. ✅ Language switching persists across reboots
7. ✅ All documentation available in Russian

## Technical Approach

- Translation framework with file-based approach
- QR code library integration with memory optimization
- Client-side CSV parsing for event logs
- Modbus command validation with timeout protection
- Maintain English for debug/Serial output

## Dependencies

- QR code generation library
- Translation files infrastructure
- Existing OLED display system
- Modbus RTU implementation
- Web UI framework

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| QR library memory usage | High | Test impact, use minimal version |
| Translation completeness | Medium | Professional review of Russian text |
| Modbus safety | High | Confirmation sequences, timeouts |
| Browser performance | Medium | Pagination, virtual scrolling |

## Stories

1. **Story 2.1**: Implement Russian translation framework
2. **Story 2.2**: Translate all web interface pages
3. **Story 2.3**: Add QR code display functionality
4. **Story 2.4**: Implement WiFi setup QR for AP mode
5. **Story 2.5**: Complete Modbus register 899 implementation
6. **Story 2.6**: Add safety mechanisms and logging
7. **Story 2.7**: Refactor event logs viewer to prevent suspension
8. **Story 2.8**: Final User Manual Review and Alignment

## Testing Strategy

- Native Russian speaker validation
- QR code scanning with multiple devices
- Modbus simulator testing (ModbusPoll)
- Performance testing of new logs viewer
- Cross-browser compatibility testing

## Documentation Requirements

- Complete Russian translation of user manual
- QR code usage instructions with images
- Modbus command reference
- Language switching guide
- Performance optimization notes

## Definition of Done

- [ ] All UI text available in Russian
- [ ] QR codes scannable from 30cm distance
- [ ] Modbus commands validated and safe
- [ ] Event viewer handles 10,000+ entries
- [ ] Russian documentation complete
- [ ] All existing features remain functional