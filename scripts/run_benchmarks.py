#!/usr/bin/env python3
"""
Statistical Benchmark Runner for Beast JSON

Runs benchmarks multiple times and computes statistical metrics:
- Median, Mean, StdDev
- Percentiles (P50, P95, P99)
- 95% Confidence Intervals
"""

import subprocess
import re
import json
import statistics
import math
from typing import List, Dict, Tuple
from pathlib import Path


class BenchmarkResult:
    """Single benchmark run result"""
    def __init__(self, library: str, parse_us: float, serialize_us: float, status: str):
        self.library = library
        self.parse_us = parse_us
        self.serialize_us = serialize_us
        self.status = status


class BenchmarkStats:
    """Statistical analysis of benchmark runs"""
    def __init__(self, values: List[float]):
        self.values = sorted(values)
        self.n = len(values)
        self.mean = statistics.mean(values)
        self.median = statistics.median(values)
        self.stdev = statistics.stdev(values) if self.n > 1 else 0.0
        
        # Percentiles
        self.p50 = self.percentile(50)
        self.p95 = self.percentile(95)
        self.p99 = self.percentile(99)
        
        # 95% Confidence Interval (assuming t-distribution)
        if self.n > 1:
            se = self.stdev / math.sqrt(self.n)
            # t-value for 95% CI (approximation for n > 30)
            t = 1.96 if self.n > 30 else 2.0
            margin = t * se
            self.ci_low = self.mean - margin
            self.ci_high = self.mean + margin
        else:
            self.ci_low = self.ci_high = self.mean
    
    def percentile(self, p: float) -> float:
        """Calculate percentile"""
        k = (self.n - 1) * p / 100
        f = math.floor(k)
        c = math.ceil(k)
        if f == c:
            return self.values[int(k)]
        d0 = self.values[int(f)] * (c - k)
        d1 = self.values[int(c)] * (k - f)
        return d0 + d1
    
    def coefficient_of_variation(self) -> float:
        """CV = stdev / mean"""
        return (self.stdev / self.mean * 100) if self.mean > 0 else 0.0


def parse_benchmark_output(output: str) -> List[BenchmarkResult]:
    """Parse bench_file_io output"""
    results = []
    
    # Pattern: "beast_json           | Parse:      1441.71 μs | Serialize:       447.08 μs | ✓ PASS"
    pattern = r'(\S+)\s+\|\s+Parse:\s+([\d.]+)\s+μs\s+\|\s+Serialize:\s+([\d.]+)\s+μs\s+\|\s+([✓✗])\s+(\w+)'
    
    for match in re.finditer(pattern, output):
        library = match.group(1)
        parse_us = float(match.group(2))
        serialize_us = float(match.group(3))
        status = match.group(5)
        
        results.append(BenchmarkResult(library, parse_us, serialize_us, status))
    
    return results


def run_single_benchmark(binary_path: Path) -> List[BenchmarkResult]:
    """Run benchmark once"""
    # Run from benchmarks directory where twitter.json is located
    cwd = binary_path.parent
    
    result = subprocess.run(
        [f"./{binary_path.name}"],
        capture_output=True,
        text=True,
        timeout=60,
        cwd=cwd
    )
    
    if result.returncode != 0:
        raise RuntimeError(f"Benchmark failed: {result.stderr}")
    
    return parse_benchmark_output(result.stdout)


def run_benchmarks(binary_path: Path, iterations: int = 20) -> Dict[str, List[BenchmarkResult]]:
    """Run benchmarks multiple times"""
    print(f"Running {iterations} iterations...")
    
    all_results = {}
    
    for i in range(iterations):
        print(f"  Iteration {i + 1}/{iterations}...", end='\r', flush=True)
        
        results = run_single_benchmark(binary_path)
        
        for result in results:
            if result.library not in all_results:
                all_results[result.library] = []
            all_results[result.library].append(result)
    
    print()  # New line after progress
    return all_results


def analyze_results(results: Dict[str, List[BenchmarkResult]]) -> Dict[str, Dict]:
    """Compute statistics for each library"""
    analysis = {}
    
    for library, runs in results.items():
        parse_times = [r.parse_us for r in runs]
        serialize_times = [r.serialize_us for r in runs]
        
        analysis[library] = {
            'parse': BenchmarkStats(parse_times),
            'serialize': BenchmarkStats(serialize_times),
            'runs': len(runs)
        }
    
    return analysis


def print_report(analysis: Dict[str, Dict]):
    """Print formatted statistical report"""
    print("\n" + "=" * 100)
    print("STATISTICAL BENCHMARK REPORT")
    print("=" * 100)
    
    for library, stats in sorted(analysis.items()):
        print(f"\n{library}:")
        print("-" * 100)
        
        # Parse stats
        p = stats['parse']
        print(f"  Parse Time (μs):")
        print(f"    Mean:       {p.mean:8.2f} ± {p.stdev:6.2f} (CV: {p.coefficient_of_variation():.1f}%)")
        print(f"    Median:     {p.median:8.2f}")
        print(f"    95% CI:     [{p.ci_low:8.2f}, {p.ci_high:8.2f}]")
        print(f"    Percentiles: P50={p.p50:7.2f}, P95={p.p95:7.2f}, P99={p.p99:7.2f}")
        
        # Serialize stats
        s = stats['serialize']
        print(f"  Serialize Time (μs):")
        print(f"    Mean:       {s.mean:8.2f} ± {s.stdev:6.2f} (CV: {s.coefficient_of_variation():.1f}%)")
        print(f"    Median:     {s.median:8.2f}")
        print(f"    95% CI:     [{s.ci_low:8.2f}, {s.ci_high:8.2f}]")
        print(f"    Percentiles: P50={s.p50:7.2f}, P95={s.p95:7.2f}, P99={s.p99:7.2f}")
    
    print("\n" + "=" * 100)
    
    # Comparison table
    print("\nCOMPARISON (Parse Median):")
    print("-" * 100)
    
    parse_medians = [(lib, stats['parse'].median) for lib, stats in analysis.items()]
    parse_medians.sort(key=lambda x: x[1])
    
    fastest = parse_medians[0][1]
    
    print(f"{'Rank':<6} {'Library':<20} {'Median (μs)':<15} {'vs Fastest':<15} {'Reliability'}")
    print("-" * 100)
    
    for rank, (library, median) in enumerate(parse_medians, 1):
        cv = analysis[library]['parse'].coefficient_of_variation()
        ratio = median / fastest
        reliability = "Excellent" if cv < 5 else "Good" if cv < 10 else "Fair" if cv < 20 else "Poor"
        
        print(f"{rank:<6} {library:<20} {median:>10.2f} μs   {ratio:>6.2f}x         {reliability} (CV={cv:.1f}%)")
    
    print("=" * 100)


def save_json_report(analysis: Dict[str, Dict], output_path: Path):
    """Save results as JSON"""
    data = {}
    
    for library, stats in analysis.items():
        data[library] = {
            'parse': {
                'mean': stats['parse'].mean,
                'median': stats['parse'].median,
                'stdev': stats['parse'].stdev,
                'p50': stats['parse'].p50,
                'p95': stats['parse'].p95,
                'p99': stats['parse'].p99,
                'ci_low': stats['parse'].ci_low,
                'ci_high': stats['parse'].ci_high,
                'cv': stats['parse'].coefficient_of_variation()
            },
            'serialize': {
                'mean': stats['serialize'].mean,
                'median': stats['serialize'].median,
                'stdev': stats['serialize'].stdev,
                'p50': stats['serialize'].p50,
                'p95': stats['serialize'].p95,
                'p99': stats['serialize'].p99,
                'ci_low': stats['serialize'].ci_low,
                'ci_high': stats['serialize'].ci_high,
                'cv': stats['serialize'].coefficient_of_variation()
            },
            'runs': stats['runs']
        }
    
    with open(output_path, 'w') as f:
        json.dump(data, f, indent=2)
    
    print(f"\nResults saved to: {output_path}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Run statistical benchmarks')
    parser.add_argument('--binary', type=Path, default=Path('build/benchmarks/bench_file_io'),
                        help='Path to benchmark binary')
    parser.add_argument('--iterations', type=int, default=20,
                        help='Number of iterations (default: 20)')
    parser.add_argument('--output', type=Path, default=Path('benchmark_results.json'),
                        help='Output JSON file')
    
    args = parser.parse_args()
    
    if not args.binary.exists():
        print(f"Error: Binary not found: {args.binary}")
        print("Please build with: cmake --build build --target bench_file_io")
        return 1
    
    print(f"Benchmark binary: {args.binary}")
    print(f"Iterations: {args.iterations}")
    print()
    
    # Run benchmarks
    results = run_benchmarks(args.binary, args.iterations)
    
    # Analyze
    analysis = analyze_results(results)
    
    # Report
    print_report(analysis)
    
    # Save JSON
    save_json_report(analysis, args.output)
    
    return 0


if __name__ == '__main__':
    exit(main())
