"""Dependency-free reference tests for Jura V1 wire encoding.

These do not replace an ESPHome compile test. They protect the protocol mapping
against accidental changes when refactoring jura.cpp.
"""


def encode_character(value: int) -> bytes:
    encoded = []
    for bit in range(0, 8, 2):
        raw = 0xFF
        if not (value & (1 << bit)):
            raw &= ~(1 << 2)
        if not (value & (1 << (bit + 1))):
            raw &= ~(1 << 5)
        encoded.append(raw)
    return bytes(encoded)


def decode_wire(data: bytes) -> bytes:
    if len(data) % 4:
        raise ValueError("Jura V1 data must contain four raw bytes per logical byte")

    result = bytearray()
    for base in range(0, len(data), 4):
        value = 0
        bit_position = 0
        for raw in data[base : base + 4]:
            if raw & (1 << 2):
                value |= 1 << bit_position
            if raw & (1 << 5):
                value |= 1 << (bit_position + 1)
            bit_position += 2
        result.append(value)
    return bytes(result)


def encode_command(command: str) -> bytes:
    return b"".join(encode_character(c) for c in (command + "\r\n").encode("ascii"))


def test_known_character_encodings():
    # Values from the legacy YAML supplied with the original component.
    assert encode_character(ord("A")) == bytes([0xDF, 0xDB, 0xDB, 0xDF])
    assert encode_character(ord("N")) == bytes([0xFB, 0xFF, 0xDB, 0xDF])
    assert encode_character(ord(":")) == bytes([0xFB, 0xFB, 0xFF, 0xDB])
    assert encode_character(ord("0")) == bytes([0xDB, 0xDB, 0xFF, 0xDB])
    assert encode_character(ord("1")) == bytes([0xDF, 0xDB, 0xFF, 0xDB])
    assert encode_character(ord("\r")) == bytes([0xDF, 0xFF, 0xDB, 0xDB])
    assert encode_character(ord("\n")) == bytes([0xFB, 0xFB, 0xDB, 0xDB])


def test_power_on_command_matches_legacy_yaml():
    expected = bytes(
        [
            0xDF, 0xDB, 0xDB, 0xDF,  # A
            0xFB, 0xFF, 0xDB, 0xDF,  # N
            0xFB, 0xFB, 0xFF, 0xDB,  # :
            0xDB, 0xDB, 0xFF, 0xDB,  # 0
            0xDF, 0xDB, 0xFF, 0xDB,  # 1
            0xDF, 0xFF, 0xDB, 0xDB,  # CR
            0xFB, 0xFB, 0xDB, 0xDB,  # LF
        ]
    )
    assert encode_command("AN:01") == expected


def test_round_trip():
    for command in ["TY:", "IC:", "AN:01", "AN:02", "FA:03", "RE:28", "RT:20", "RR:F0"]:
        wire = encode_command(command)
        assert decode_wire(wire) == (command + "\r\n").encode("ascii")


if __name__ == "__main__":
    test_known_character_encodings()
    test_power_on_command_matches_legacy_yaml()
    test_round_trip()
    print("protocol reference tests: OK")
