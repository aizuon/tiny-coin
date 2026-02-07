#!/usr/bin/env python3
import sys


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.metal> <output.inc>", file=sys.stderr)
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    with open(input_path, "r") as f:
        shader_source = f.read()

    with open(output_path, "w") as f:
        f.write('R"METAL_SHADER(\n')
        f.write(shader_source)
        f.write(')METAL_SHADER"\n')


if __name__ == "__main__":
    main()
