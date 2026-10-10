# v17

- Added Home Assistant `Raw Jura Command` template text entity.
- Added `Send Raw Jura Command` template button; editing the field alone never transmits.
- Added `JuraComponent::queue_raw_command()` with runtime validation for empty, overlength, CR/LF, control, and non-ASCII input.
- Raw commands use the existing fixed transaction queue/nonblocking encoder and are rejected while UART TX is floated.
- Enabled `Jura Last Response` by default for raw-command experiments.
- Updated Home Assistant dashboard with raw command field, send button, response, and service-command warning.
- Component version marker bumped to 17.
