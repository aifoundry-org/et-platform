#!/usr/bin/env python3

from argparse import ArgumentParser
from pathlib import Path


def assetpack(input: Path, output: Path, varname: str):
    output.parent.mkdir(exist_ok=True)
    data = input.read_bytes()
    size = len(data)
    output.write_text(
        f""".data
.globl {varname}_size
{varname}_size:
    .long {size}
.globl {varname}_data
{varname}_data:
    .incbin "{input}"
"""
    )


if __name__ == "__main__":
    parser = ArgumentParser(description="Create assembly file from binary asset")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("varname", type=str)
    args = parser.parse_args()
    assetpack(args.input, args.output, args.varname)
