#!/usr/bin/env python3
"""Regenerate the starter seeds under mayhem/<target>/testsuite/ (independent EBML encoder)."""
import os, struct, zlib

HERE = os.path.dirname(os.path.abspath(__file__))


def vint(n):
    for length in range(1, 9):
        if n < (1 << (7 * length)) - 1:
            return (n | (1 << (7 * length))).to_bytes(length, "big")
    raise ValueError(n)


def el(eid, payload):
    return eid + vint(len(payload)) + payload


def uint(v):
    return v.to_bytes(max(1, (v.bit_length() + 7) // 8), "big")


ROOT = bytes.fromhex("1b4d5a01")
leaf = lambda i, p: el(bytes.fromhex(i), p)
deep = leaf("a3", leaf("93", b"deep") + leaf("94", "d\u00e9j\u00e0".encode()))
sub = leaf("a0", leaf("91", uint(7)) + leaf("4792", b"\x00\x01\x02") + deep)
body = (leaf("81", uint(300)) + leaf("82", b"\xfe") + leaf("83", b"abc") + leaf("84", "h\u00e9llo".encode())
        + leaf("85", struct.pack(">f", 1.5)) + leaf("86", struct.pack(">q", -978307200 * 10**9))
        + leaf("4687", b"\xde\xad\xbe") + sub)
tree = el(ROOT, body)

head_body = (leaf("4286", b"\x01") + leaf("42f7", b"\x01") + leaf("42f2", b"\x04") + leaf("42f3", b"\x08")
             + leaf("4282", b"matroska") + leaf("4287", b"\x04") + leaf("4285", b"\x02"))
head = el(bytes.fromhex("1a45dfa3"), head_body)
crc = zlib.crc32(body) & 0xFFFFFFFF
tree_crc = el(ROOT, leaf("bf", struct.pack("<I", crc)) + body)
void = leaf("ec", b"\x00" * 5)
inf_sub = ROOT + b"\xff" + leaf("81", uint(1)) + bytes.fromhex("a0") + b"\x01\xff\xff\xff\xff\xff\xff\xff" + leaf("91", uint(2)) + leaf("81", uint(3))
mem_tree_dummy = el(ROOT, leaf("81", uint(1)) + leaf("c8", b"unknown") + void + leaf("83", b"x"))

seeds = {
    "fuzz_parse": {
        "tree": tree, "head": head, "head_tree": head + tree, "tree_crc": tree_crc,
        "infinite_sub": inf_sub, "unknown_and_void": mem_tree_dummy,
    },
    "fuzz_elements": {
        "uint": b"\x00" + uint(300), "sint": b"\x01\xfe", "string": b"\x02abc", "utf8": b"\x03h\xc3\xa9llo",
        "float32": b"\x04" + struct.pack(">f", 1.5), "float64": b"\x04" + struct.pack(">d", -2.25),
        "date": b"\x05" + struct.pack(">q", 123456789000), "binary": b"\x06\xde\xad\xbe\xef",
    },
    "fuzz_memio": {
        "write_read": bytes([8, 0, 10]) + b"0123456789" + bytes([1, 3, 2, 0, 128, 4, 5, 5, 1, 0]),
        "write_seek_read": bytes([0, 0, 40]) + bytes(range(40)) + bytes([2, 1, 0, 1, 3, 1, 9, 2]),
        "fill_from_source": bytes([0, 3, 4, 1, 1, 2, 4, 0, 2, 129, 128, 1, 5, 0, 5, 9]) + b"abcdefghijklmnopqrstuvwxyz",
    },
}
for target, files in seeds.items():
    d = os.path.join(HERE, target, "testsuite")
    os.makedirs(d, exist_ok=True)
    for name, data in files.items():
        with open(os.path.join(d, name), "wb") as f:
            f.write(data)
