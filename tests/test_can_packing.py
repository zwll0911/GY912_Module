"""
Unit tests for CAN bus byte packing/unpacking used by the GY912 Navigation Module.

Tests verify the int16_t big-endian encoding for Euler angles (Yaw, Pitch, Roll)
as defined in docs/CAN_PROTOCOL.md.

Run with: python -m pytest tests/test_can_packing.py -v
"""
import struct
import pytest


def pack_angle(degrees: float) -> tuple:
    """Pack a float angle into 2 big-endian bytes (firmware logic)."""
    raw = int(degrees * 100)
    # Clamp to int16 range
    raw = max(-32768, min(32767, raw))
    hi = (raw >> 8) & 0xFF
    lo = raw & 0xFF
    return (hi, lo)


def unpack_angle(hi: int, lo: int) -> float:
    """Unpack 2 big-endian bytes back to degrees (receiver logic)."""
    raw = struct.unpack('>h', bytes([hi, lo]))[0]
    return raw / 100.0


def pack_orientation(yaw: float, pitch: float, roll: float) -> list:
    """Pack 3 Euler angles into a 6-byte CAN payload."""
    y = pack_angle(yaw)
    p = pack_angle(pitch)
    r = pack_angle(roll)
    return [y[0], y[1], p[0], p[1], r[0], r[1]]


def unpack_orientation(data: list) -> dict:
    """Unpack a 6-byte CAN payload into Euler angles."""
    return {
        'yaw': unpack_angle(data[0], data[1]),
        'pitch': unpack_angle(data[2], data[3]),
        'roll': unpack_angle(data[4], data[5]),
    }


# ===================== TESTS =====================

class TestPackAngle:
    """Tests for individual angle packing."""

    def test_zero(self):
        assert pack_angle(0.0) == (0x00, 0x00)

    def test_positive(self):
        hi, lo = pack_angle(145.30)
        assert hi == 0x38
        assert lo == 0xC2

    def test_negative(self):
        hi, lo = pack_angle(-1.10)
        raw = struct.unpack('>h', bytes([hi, lo]))[0]
        assert raw == -110

    def test_small_positive(self):
        hi, lo = pack_angle(2.45)
        assert hi == 0x00
        assert lo == 0xF5

    def test_max_positive(self):
        """Max representable: +327.67°"""
        hi, lo = pack_angle(327.67)
        raw = struct.unpack('>h', bytes([hi, lo]))[0]
        assert raw == 32767

    def test_max_negative(self):
        """Min representable: -327.68°"""
        hi, lo = pack_angle(-327.68)
        raw = struct.unpack('>h', bytes([hi, lo]))[0]
        assert raw == -32768

    def test_overflow_clamped(self):
        """Values beyond ±327.67° should clamp."""
        hi, lo = pack_angle(500.0)
        raw = struct.unpack('>h', bytes([hi, lo]))[0]
        assert raw == 32767  # clamped

    def test_resolution(self):
        """Resolution is 0.01°."""
        hi, lo = pack_angle(0.01)
        raw = struct.unpack('>h', bytes([hi, lo]))[0]
        assert raw == 1


class TestUnpackAngle:
    """Tests for individual angle unpacking."""

    def test_zero(self):
        assert unpack_angle(0x00, 0x00) == 0.0

    def test_positive(self):
        assert unpack_angle(0x38, 0xC2) == pytest.approx(145.30)

    def test_negative(self):
        assert unpack_angle(0xFF, 0x92) == pytest.approx(-1.10)

    def test_twos_complement(self):
        """0xFF92 = -110 in int16 → -1.10°"""
        result = unpack_angle(0xFF, 0x92)
        assert result == pytest.approx(-1.10)


class TestRoundTrip:
    """Tests that pack → unpack returns the original value."""

    @pytest.mark.parametrize("angle", [
        0.0, 1.0, -1.0, 45.0, -45.0, 90.0, -90.0,
        180.0, -180.0, 0.01, -0.01, 123.45, -123.45,
        327.67, -327.67,
    ])
    def test_roundtrip(self, angle):
        hi, lo = pack_angle(angle)
        result = unpack_angle(hi, lo)
        assert result == pytest.approx(angle, abs=0.01)


class TestFullPayload:
    """Tests for complete 6-byte orientation payload."""

    def test_pack_example(self):
        """Test the worked example from CAN_PROTOCOL.md."""
        payload = pack_orientation(145.30, 2.45, -1.10)
        assert payload == [0x38, 0xC2, 0x00, 0xF5, 0xFF, 0x92]

    def test_unpack_example(self):
        result = unpack_orientation([0x38, 0xC2, 0x00, 0xF5, 0xFF, 0x92])
        assert result['yaw'] == pytest.approx(145.30)
        assert result['pitch'] == pytest.approx(2.45)
        assert result['roll'] == pytest.approx(-1.10)

    def test_all_zeros(self):
        payload = pack_orientation(0, 0, 0)
        assert payload == [0, 0, 0, 0, 0, 0]
        result = unpack_orientation(payload)
        assert result == {'yaw': 0.0, 'pitch': 0.0, 'roll': 0.0}

    def test_all_negative(self):
        payload = pack_orientation(-90.0, -45.0, -30.0)
        result = unpack_orientation(payload)
        assert result['yaw'] == pytest.approx(-90.0)
        assert result['pitch'] == pytest.approx(-45.0)
        assert result['roll'] == pytest.approx(-30.0)

    def test_roundtrip_full(self):
        payload = pack_orientation(179.99, -89.50, 0.01)
        result = unpack_orientation(payload)
        assert result['yaw'] == pytest.approx(179.99, abs=0.01)
        assert result['pitch'] == pytest.approx(-89.50, abs=0.01)
        assert result['roll'] == pytest.approx(0.01, abs=0.01)
