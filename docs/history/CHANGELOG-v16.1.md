# v16.1

Hotfix packaging release for v16.

- No behavioural change to the v16 UART TX Float feature.
- Adds a component version marker (`16.1`) to the Jura component config log.
- Adds an explicit warning in `jura.yaml` that the local `components/jura` directory must be replaced together with the YAML when upgrading.
- v16 YAML requires `JuraComponent::is_tx_floating()` and `set_tx_floating()`, which are not present in the v15 component header.
