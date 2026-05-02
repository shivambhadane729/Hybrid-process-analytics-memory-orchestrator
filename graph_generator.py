"""
Graph Generator for Adaptive Process Usage Analyzer
Reads process_data.csv and generates 6 visualization graphs.

Usage: python graph_generator.py
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import os
import sys

matplotlib.use('Agg')  # Non-interactive backend

# Style configuration
plt.rcParams.update({
    'figure.facecolor': '#1a1a2e',
    'axes.facecolor': '#16213e',
    'axes.edgecolor': '#e94560',
    'axes.labelcolor': '#eee',
    'text.color': '#eee',
    'xtick.color': '#ccc',
    'ytick.color': '#ccc',
    'grid.color': '#333',
    'font.size': 10,
    'figure.figsize': (12, 7),
})

# Color palette
COLORS = {
    'HOT':  '#e94560',
    'WARM': '#f5a623',
    'COLD': '#0f3460',
}

BAR_COLORS = ['#e94560', '#f5a623', '#533483', '#0f3460', '#16c79a',
              '#ff6b6b', '#48c9b0', '#bb86fc', '#03dac6', '#cf6679']


def load_data(filename='process_data.csv'):
    if not os.path.exists(filename):
        print(f"[ERROR] {filename} not found.")
        print("Run the C++ analyzer first and select option [15] to generate data.")
        sys.exit(1)

    df = pd.read_csv(filename)
    # Sort by score descending and take top 20 for readability
    df = df.sort_values('HotnessScore', ascending=False).head(20)
    return df


def graph1_usage_time(df):
    """Graph 1: Usage Time Per Process"""
    fig, ax = plt.subplots()
    data = df.sort_values('ActiveTimeMin', ascending=True).tail(15)

    colors = [COLORS.get(c, '#333') for c in data['Classification']]
    bars = ax.barh(data['Name'], data['ActiveTimeMin'], color=colors, edgecolor='#eee', linewidth=0.5)

    ax.set_xlabel('Active Time (minutes)')
    ax.set_title('Usage Time Per Process', fontsize=14, fontweight='bold', color='#e94560')
    ax.grid(axis='x', alpha=0.3)

    # Add value labels
    for bar, val in zip(bars, data['ActiveTimeMin']):
        ax.text(bar.get_width() + 0.5, bar.get_y() + bar.get_height()/2,
                f'{val:.0f}m', va='center', fontsize=8, color='#ccc')

    plt.tight_layout()
    plt.savefig('graph1_usage_time.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph1_usage_time.png")


def graph2_frequency(df):
    """Graph 2: Frequency vs Process"""
    fig, ax = plt.subplots()
    data = df.sort_values('FocusCount', ascending=False).head(15)

    colors = [BAR_COLORS[i % len(BAR_COLORS)] for i in range(len(data))]
    bars = ax.bar(range(len(data)), data['FocusCount'], color=colors, edgecolor='#eee', linewidth=0.5)

    ax.set_xticks(range(len(data)))
    ax.set_xticklabels(data['Name'], rotation=45, ha='right', fontsize=8)
    ax.set_ylabel('Focus Count')
    ax.set_title('Frequency vs Process', fontsize=14, fontweight='bold', color='#f5a623')
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig('graph2_frequency.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph2_frequency.png")


def graph3_memory_vs_usage(df):
    """Graph 3: Memory vs Usage (Scatter)"""
    fig, ax = plt.subplots()

    for cls in ['HOT', 'WARM', 'COLD']:
        subset = df[df['Classification'] == cls]
        ax.scatter(subset['ActiveTimeMin'], subset['MemoryMB'],
                   c=COLORS[cls], label=cls, s=100, alpha=0.8, edgecolors='white', linewidth=0.5)

        for _, row in subset.iterrows():
            ax.annotate(row['Name'], (row['ActiveTimeMin'], row['MemoryMB']),
                        fontsize=7, color='#ccc', xytext=(5, 5), textcoords='offset points')

    ax.set_xlabel('Active Time (minutes)')
    ax.set_ylabel('Memory Usage (MB)')
    ax.set_title('Memory vs Usage Analysis', fontsize=14, fontweight='bold', color='#533483')
    ax.legend(framealpha=0.5)
    ax.grid(alpha=0.3)

    plt.tight_layout()
    plt.savefig('graph3_memory_vs_usage.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph3_memory_vs_usage.png")


def graph4_classification_pie(df):
    """Graph 4: HOT/WARM/COLD Pie Chart"""
    fig, ax = plt.subplots()

    counts = df['Classification'].value_counts()
    labels = counts.index.tolist()
    sizes = counts.values.tolist()
    colors = [COLORS.get(l, '#333') for l in labels]

    wedges, texts, autotexts = ax.pie(sizes, labels=labels, colors=colors,
                                       autopct='%1.1f%%', startangle=140,
                                       textprops={'color': '#eee', 'fontsize': 12},
                                       wedgeprops={'edgecolor': '#eee', 'linewidth': 1})

    for autotext in autotexts:
        autotext.set_color('white')
        autotext.set_fontweight('bold')

    ax.set_title('HOT / WARM / COLD Distribution', fontsize=14, fontweight='bold', color='#e94560')

    plt.tight_layout()
    plt.savefig('graph4_classification_pie.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph4_classification_pie.png")


def graph5_recency_timeline(df):
    """Graph 5: Recency Timeline"""
    fig, ax = plt.subplots()
    data = df.sort_values('LastUsedSecAgo', ascending=True).head(15)

    colors = [COLORS.get(c, '#333') for c in data['Classification']]
    bars = ax.barh(data['Name'], data['LastUsedSecAgo'], color=colors, edgecolor='#eee', linewidth=0.5)

    ax.set_xlabel('Seconds Since Last Used')
    ax.set_title('Recency Timeline', fontsize=14, fontweight='bold', color='#16c79a')
    ax.grid(axis='x', alpha=0.3)

    # Add labels
    for bar, val in zip(bars, data['LastUsedSecAgo']):
        if val < 60:
            label = f'{val:.0f}s'
        elif val < 3600:
            label = f'{val/60:.0f}m'
        else:
            label = f'{val/3600:.1f}h'
        ax.text(bar.get_width() + 1, bar.get_y() + bar.get_height()/2,
                label, va='center', fontsize=8, color='#ccc')

    plt.tight_layout()
    plt.savefig('graph5_recency_timeline.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph5_recency_timeline.png")


def graph6_priority_ranking(df):
    """Graph 6: Priority Ranking Bar Graph"""
    fig, ax = plt.subplots()
    data = df.sort_values('HotnessScore', ascending=True).tail(15)

    colors = [COLORS.get(c, '#333') for c in data['Classification']]
    bars = ax.barh(data['Name'], data['HotnessScore'], color=colors, edgecolor='#eee', linewidth=0.5)

    # Add threshold lines
    ax.axvline(x=70, color='#e94560', linestyle='--', alpha=0.7, label='HOT threshold (70)')
    ax.axvline(x=40, color='#f5a623', linestyle='--', alpha=0.7, label='WARM threshold (40)')

    ax.set_xlabel('Hotness Score')
    ax.set_title('Priority Ranking', fontsize=14, fontweight='bold', color='#bb86fc')
    ax.legend(loc='lower right', framealpha=0.5)
    ax.grid(axis='x', alpha=0.3)

    # Add score labels
    for bar, val in zip(bars, data['HotnessScore']):
        ax.text(bar.get_width() + 0.5, bar.get_y() + bar.get_height()/2,
                f'{val:.1f}', va='center', fontsize=8, color='#ccc')

    plt.tight_layout()
    plt.savefig('graph6_priority_ranking.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph6_priority_ranking.png")


def graph7_storage_distribution(df):
    """Graph 7: Storage Layer Distribution"""
    if 'StorageLayer' not in df.columns:
        print("  [SKIP] graph7 - StorageLayer column not found (re-export CSV with option [15])")
        return

    fig, ax = plt.subplots()
    layer_counts = df['StorageLayer'].value_counts()

    layer_order = ['L1_CACHE', 'L2_RAM', 'L3_DISK']
    layer_labels = ['L1 Cache (HOT)', 'L2 RAM (WARM)', 'L3 Disk (COLD)']
    layer_colors = ['#e94560', '#f5a623', '#0f3460']

    counts = [layer_counts.get(l, 0) for l in layer_order]

    bars = ax.bar(layer_labels, counts, color=layer_colors, edgecolor='#eee', linewidth=0.8)

    for bar, val in zip(bars, counts):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.3,
                str(val), ha='center', fontsize=11, color='#eee', fontweight='bold')

    ax.set_ylabel('Number of Processes')
    ax.set_title('Storage Layer Distribution (3-Tier Engine)', fontsize=14,
                 fontweight='bold', color='#bb86fc')
    ax.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig('graph7_storage_distribution.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph7_storage_distribution.png")


def graph8_faults_vs_memory(df):
    """Graph 8: Page Faults vs Memory (Scatter)"""
    if 'PageFaults' not in df.columns:
        print("  [SKIP] graph8 - PageFaults column not found (re-export CSV with option [15])")
        return

    fig, ax = plt.subplots()

    for cls in ['HOT', 'WARM', 'COLD']:
        subset = df[df['Classification'] == cls]
        ax.scatter(subset['MemoryMB'], subset['PageFaults'],
                   c=COLORS[cls], label=cls, s=100, alpha=0.8,
                   edgecolors='white', linewidth=0.5)

        for _, row in subset.iterrows():
            ax.annotate(row['Name'], (row['MemoryMB'], row['PageFaults']),
                        fontsize=7, color='#ccc', xytext=(5, 5), textcoords='offset points')

    ax.set_xlabel('Memory Usage (MB)')
    ax.set_ylabel('Page Fault Count')
    ax.set_title('Page Faults vs Memory Usage', fontsize=14,
                 fontweight='bold', color='#e94560')
    ax.legend(framealpha=0.5)
    ax.grid(alpha=0.3)

    plt.tight_layout()
    plt.savefig('graph8_faults_vs_memory.png', dpi=150, bbox_inches='tight')
    plt.close()
    print("  [OK] graph8_faults_vs_memory.png")


def main():
    print("\n  " + "=" * 50)
    print("  ADAPTIVE PROCESS ANALYZER - Graph Generator")
    print("  " + "=" * 50)
    print()

    df = load_data()
    print(f"  Loaded {len(df)} processes from CSV.\n")

    print("  Generating graphs...\n")
    graph1_usage_time(df)
    graph2_frequency(df)
    graph3_memory_vs_usage(df)
    graph4_classification_pie(df)
    graph5_recency_timeline(df)
    graph6_priority_ranking(df)
    graph7_storage_distribution(df)
    graph8_faults_vs_memory(df)

    print("\n  All graphs generated successfully!")
    print("  Files saved in current directory.\n")


if __name__ == '__main__':
    main()
