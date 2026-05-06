#!/usr/bin/env python3

import argparse
import bisect
import math
import re
import sys
from dataclasses import dataclass


HEADER_RE = re.compile(r"^\s*\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\]\s*,\s*(\d+)\s*$")
SEGMENT_RE = re.compile(
    r"^\s*\[\s*(-?\d+)\s*,\s*(-?\d+)\s*\]\s*:\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*$"
)


@dataclass(frozen=True)
class InputSegment:
    start: int
    end: int
    prob: float


@dataclass(frozen=True)
class LeafSegment:
    start: int
    end: int
    rate: float

    @property
    def length(self) -> int:
        return self.end - self.start + 1


@dataclass
class TreeNode:
    start: int
    leaf_count: int
    root_mass: float
    active_count: int
    uniform_rate: float | None = None
    left: "TreeNode | None" = None
    right: "TreeNode | None" = None

    @property
    def end(self) -> int:
        return self.start + self.leaf_count - 1


def build_parser() -> argparse.ArgumentParser:
    description = (
        "Read a leaf-space interval description file, build a complete binary tree over the leaves, "
        "solve F(t) = C numerically when needed, then compute the path metric r(n) for one queried leaf."
    )
    epilog = (
        "Input file format:\n"
        "  First non-comment line: [X, Y], NODE_SIZE\n"
        "  Next non-comment lines: [x, y]: p\n"
        "  Comment lines: start with # and are ignored\n\n"
        "Semantics:\n"
        "  The input describes leaf-space probabilities over the integer range [X, Y].\n"
        "  By default [x, y]: value is interpreted as a per-leaf probability.\n"
        "  With --input-scale coefficient, value is interpreted as a per-leaf weight and is\n"
        "  normalized to probability by dividing by sum(length * value) over all segments.\n"
        "  Let L = Y - X + 1. L must be a power of two.\n"
        "  Let levels = log2(L) + 1. Each input leaf probability p is scaled to p / levels.\n"
        "  A tree node probability q(node) is the sum of scaled leaf probabilities in its subtree.\n"
        "  C = 96 * 2^20 / NODE_SIZE.\n"
        "  If M <= C, every path-node probability saturates to 1 and the result is levels.\n"
        "  Otherwise the tool solves sum_nodes (1 - exp(-q(node) * t)) = C and computes\n"
        "  r(n) = sum_path T_E(node), where T_E(node) = p(node) + (1 - p(node)) * o\n"
        "  and p(node) = 1 - exp(-q(node) * t_C).\n\n"
        "Examples:\n"
        "  python3 thesis/key_prob.py intervals.txt --n 123 --o 0.3\n"
        "  python3 thesis/key_prob.py intervals.txt --debug --n 123 --o 0.3\n"
        "  python3 thesis/key_prob.py intervals.txt --o 0.3 <<< \"123\""
    )
    parser = argparse.ArgumentParser(
        description=description,
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    _ = parser.add_argument("input_file", help="path to the interval description file")
    _ = parser.add_argument(
        "--n",
        dest="n_value",
        type=int,
        help="query leaf n in leaf-space; if omitted, the tool reads one integer from stdin",
    )
    _ = parser.add_argument(
        "--input-scale",
        choices=("probability", "coefficient"),
        default="probability",
        help="interpret input values as probabilities or per-leaf coefficients (default: %(default)s)",
    )
    _ = parser.add_argument(
        "--o",
        dest="o_value",
        type=float,
        required=True,
        help="parameter o used in T_E = p + (1 - p) * o",
    )
    _ = parser.add_argument(
        "--eps",
        type=float,
        default=1e-9,
        help="binary-search tolerance for t_C (default: %(default)s)",
    )
    _ = parser.add_argument(
        "--debug",
        action="store_true",
        help="print diagnostic messages to stdout with the [debug] prefix",
    )
    return parser


def debug(enabled: bool, message: str) -> None:
    if enabled:
        print(f"[debug] {message}")


def parse_input_file(path: str) -> tuple[int, int, int, list[InputSegment]]:
    with open(path, "r", encoding="ascii") as infile:
        lines = infile.readlines()

    if not lines:
        raise ValueError("input file is empty")

    header_line_number = None
    header_line = None
    for line_number, raw_line in enumerate(lines, start=1):
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        header_line_number = line_number
        header_line = raw_line.rstrip("\n")
        break

    if header_line is None:
        raise ValueError("input file does not contain a header line")

    header_match = HEADER_RE.match(header_line)
    if header_match is None:
        raise ValueError(
            f"line {header_line_number} must have the form [X, Y], NODE_SIZE"
        )

    x_min = int(header_match.group(1))
    y_max = int(header_match.group(2))
    node_size = int(header_match.group(3))

    if x_min > y_max:
        raise ValueError(
            f"line {header_line_number} has invalid range: X={x_min} is greater than Y={y_max}"
        )
    if node_size <= 0:
        raise ValueError(
            f"line {header_line_number} has invalid NODE_SIZE={node_size}; it must be positive"
        )

    segments: list[InputSegment] = []
    for line_number, raw_line in enumerate(
        lines[header_line_number:], start=header_line_number + 1
    ):
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue

        segment_match = SEGMENT_RE.match(stripped)
        if segment_match is None:
            raise ValueError(f"line {line_number} must have the form [x, y]: p")

        start = int(segment_match.group(1))
        end = int(segment_match.group(2))
        prob = float(segment_match.group(3))

        if start > end:
            raise ValueError(
                f"line {line_number} has invalid segment [{start}, {end}]: x must be <= y"
            )
        if start < x_min or end > y_max:
            raise ValueError(
                f"line {line_number} has segment [{start}, {end}] outside [{x_min}, {y_max}]"
            )
        if prob < 0:
            raise ValueError(
                f"line {line_number} has invalid p={prob}: p must be non-negative"
            )

        segments.append(InputSegment(start=start, end=end, prob=prob))

    segments.sort(key=lambda segment: (segment.start, segment.end))
    for index in range(1, len(segments)):
        previous = segments[index - 1]
        current = segments[index]
        if current.start <= previous.end:
            raise ValueError(
                "segments overlap: "
                f"[{previous.start}, {previous.end}] intersects [{current.start}, {current.end}]"
            )

    return x_min, y_max, node_size, segments


def read_query_n(cli_n_value: int | None) -> int:
    if cli_n_value is not None:
        return cli_n_value

    if sys.stdin.isatty():
        raise ValueError("missing query n: pass --n VALUE or provide one integer via stdin")

    raw_stdin = sys.stdin.read().strip()
    if not raw_stdin:
        raise ValueError("stdin must contain a single integer n")

    tokens = raw_stdin.split()
    if len(tokens) != 1:
        raise ValueError("stdin must contain exactly one integer n")

    try:
        return int(tokens[0])
    except ValueError as exc:
        raise ValueError(f"stdin value {tokens[0]!r} is not a valid integer") from exc


def is_power_of_two(value: int) -> bool:
    return value > 0 and (value & (value - 1)) == 0


def merge_leaf_segments(segments: list[LeafSegment]) -> list[LeafSegment]:
    if not segments:
        return []

    merged = [segments[0]]
    for segment in segments[1:]:
        previous = merged[-1]
        if previous.end + 1 == segment.start and previous.rate == segment.rate:
            merged[-1] = LeafSegment(previous.start, segment.end, previous.rate)
        else:
            merged.append(segment)
    return merged


def build_leaf_segments(
    x_min: int,
    y_max: int,
    input_segments: list[InputSegment],
    input_scale: str,
) -> tuple[int, int, list[LeafSegment]]:
    leaf_count = y_max - x_min + 1
    if not is_power_of_two(leaf_count):
        raise ValueError(
            f"leaf-space size L={leaf_count} must be a power of two for a complete binary tree"
        )

    levels = leaf_count.bit_length()
    normalization_factor = 1.0
    if input_scale == "coefficient":
        total_mass = sum(
            (segment.end - segment.start + 1) * segment.prob for segment in input_segments
        )
        if total_mass <= 0.0:
            raise ValueError(
                "coefficient input requires a positive total mass sum(length * coefficient)"
            )
        normalization_factor = total_mass

    scaled_segments: list[LeafSegment] = []
    cursor = 0

    for segment in input_segments:
        relative_start = segment.start - x_min
        relative_end = segment.end - x_min
        if cursor < relative_start:
            scaled_segments.append(LeafSegment(cursor, relative_start - 1, 0.0))
        scaled_segments.append(
            LeafSegment(
                relative_start,
                relative_end,
                (segment.prob / normalization_factor) / levels,
            )
        )
        cursor = relative_end + 1

    if cursor < leaf_count:
        scaled_segments.append(LeafSegment(cursor, leaf_count - 1, 0.0))

    if not scaled_segments:
        scaled_segments.append(LeafSegment(0, leaf_count - 1, 0.0))

    return leaf_count, levels, merge_leaf_segments(scaled_segments)


def build_tree(
    start: int,
    leaf_count: int,
    leaf_segments: list[LeafSegment],
    segment_starts: list[int],
) -> TreeNode:
    index = bisect.bisect_right(segment_starts, start) - 1
    segment = leaf_segments[index]
    end = start + leaf_count - 1

    if segment.end >= end:
        root_mass = segment.rate * leaf_count
        active_count = 0 if root_mass <= 0.0 else 2 * leaf_count - 1
        return TreeNode(
            start=start,
            leaf_count=leaf_count,
            root_mass=root_mass,
            active_count=active_count,
            uniform_rate=segment.rate,
        )

    half = leaf_count // 2
    left = build_tree(start, half, leaf_segments, segment_starts)
    right = build_tree(start + half, half, leaf_segments, segment_starts)
    root_mass = left.root_mass + right.root_mass
    active_count = left.active_count + right.active_count + (1 if root_mass > 0.0 else 0)
    return TreeNode(
        start=start,
        leaf_count=leaf_count,
        root_mass=root_mass,
        active_count=active_count,
        left=left,
        right=right,
    )


def uniform_subtree_f_value(leaf_count: int, uniform_rate: float, t_value: float) -> float:
    if uniform_rate <= 0.0:
        return 0.0

    total = 0.0
    nodes_at_level = 1
    node_mass = uniform_rate * leaf_count
    current_leaf_count = leaf_count

    while current_leaf_count >= 1:
        total += nodes_at_level * (-math.expm1(-node_mass * t_value))
        if current_leaf_count == 1:
            break
        nodes_at_level *= 2
        node_mass /= 2.0
        current_leaf_count //= 2

    return total


def f_value(node: TreeNode, t_value: float) -> float:
    if node.active_count == 0:
        return 0.0
    if node.uniform_rate is not None:
        return uniform_subtree_f_value(node.leaf_count, node.uniform_rate, t_value)

    root_term = -math.expm1(-node.root_mass * t_value)
    return root_term + f_value(node.left, t_value) + f_value(node.right, t_value)


def solve_t_c(
    root: TreeNode,
    c_value: float,
    eps: float,
    debug_enabled: bool = False,
) -> float:
    if eps <= 0:
        raise ValueError(f"eps must be positive, got {eps}")
    if c_value <= 0:
        return 0.0

    low = 0.0
    high = 1.0
    expansions = 0
    max_expansions = 4096

    for _ in range(max_expansions):
        expansions += 1
        if f_value(root, high) >= c_value:
            break
        high *= 2.0
    else:
        raise RuntimeError("failed to bracket the root for t_C")

    debug(
        debug_enabled,
        f"bracketed t_C in [{low:.17g}, {high:.17g}] after {expansions} expansion steps",
    )

    iterations = 0
    while high - low > eps:
        iterations += 1
        mid = (low + high) / 2.0
        if mid == low or mid == high:
            debug(
                debug_enabled,
                "stopping binary search at float precision limit before reaching eps",
            )
            break
        if f_value(root, mid) < c_value:
            low = mid
        else:
            high = mid

    debug(debug_enabled, f"binary-search iterations={iterations}")
    return (low + high) / 2.0


def collect_path_nodes(node: TreeNode, leaf_index: int) -> list[tuple[int, int, float]]:
    if not (node.start <= leaf_index <= node.end):
        raise ValueError("leaf index is outside the tree root interval")

    path: list[tuple[int, int, float]] = []
    current = node

    while True:
        if current.uniform_rate is not None:
            sub_start = current.start
            sub_leaf_count = current.leaf_count
            while True:
                path.append(
                    (
                        sub_start,
                        sub_start + sub_leaf_count - 1,
                        current.uniform_rate * sub_leaf_count,
                    )
                )
                if sub_leaf_count == 1:
                    return path
                half = sub_leaf_count // 2
                if leaf_index < sub_start + half:
                    sub_leaf_count = half
                else:
                    sub_start += half
                    sub_leaf_count = half

        path.append((current.start, current.end, current.root_mass))
        half = current.leaf_count // 2
        if leaf_index < current.start + half:
            current = current.left
        else:
            current = current.right


def metric_from_path(
    path_nodes: list[tuple[int, int, float]],
    t_c: float,
    o_value: float,
    x_min: int,
    debug_enabled: bool,
) -> float:
    total = 0.0
    for level, (start, end, q_value) in enumerate(path_nodes):
        probability = -math.expm1(-q_value * t_c)
        t_e = probability + (1.0 - probability) * o_value
        total += t_e
        debug(
            debug_enabled,
            "path level={} leaves=[{}, {}] q={} p={} T_E={}".format(
                level,
                x_min + start,
                x_min + end,
                f"{q_value:.17g}",
                f"{probability:.17g}",
                f"{t_e:.17g}",
            ),
        )
    return total


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        x_min, y_max, node_size, input_segments = parse_input_file(args.input_file)
        n_value = read_query_n(args.n_value)
        if n_value < x_min or n_value > y_max:
            raise ValueError(f"input n={n_value} is outside [{x_min}, {y_max}]")

        leaf_count, levels, leaf_segments = build_leaf_segments(
            x_min, y_max, input_segments, args.input_scale
        )
        segment_starts = [segment.start for segment in leaf_segments]
        root = build_tree(0, leaf_count, leaf_segments, segment_starts)
        c_value = (96 * (2**20)) / node_size

        debug(args.debug, f"leaf-range=[{x_min}, {y_max}], NODE_SIZE={node_size}")
        debug(args.debug, f"L={leaf_count}")
        debug(args.debug, f"levels={levels}")
        debug(args.debug, f"compressed leaf segments={len(leaf_segments)}")
        debug(args.debug, f"M={root.active_count}")
        debug(args.debug, f"C={c_value:.17g}")

        if root.active_count <= c_value:
            debug(args.debug, "M <= C, every path-node probability saturates to 1")
            debug(args.debug, f"r({n_value})=levels={levels}")
            print(f"{levels:.17g}")
            return 0

        t_c = solve_t_c(root, c_value, args.eps, args.debug)
        leaf_index = n_value - x_min
        path_nodes = collect_path_nodes(root, leaf_index)
        result = metric_from_path(path_nodes, t_c, args.o_value, x_min, args.debug)

        debug(args.debug, f"t_C={t_c:.17g}")
        debug(args.debug, f"r({n_value})={result:.17g}")
        print(f"{result:.17g}")
        return 0

    except (OSError, ValueError, RuntimeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
