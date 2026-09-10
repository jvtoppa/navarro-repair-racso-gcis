import pandas as pd

main_df = pd.read_csv('summary_best_level_merged.csv')
size_df = pd.read_csv('results_with_.csv')

# Lowercase mapping for merging
main_df['dataset_lower'] = main_df['dataset'].str.lower()
size_df['dataset_lower'] = size_df['dataset'].str.lower()

# Strip extensions for fallback matching
def clean_name(s):
    s = s.lower()
    for ext in ['.txt', '.pdf', '.50mb']:
        if s.endswith(ext):
            s = s[:-len(ext)]
    return s

main_df['dataset_clean'] = main_df['dataset'].apply(clean_name)
size_df['dataset_clean'] = size_df['dataset'].apply(clean_name)

# First try merging on dataset_lower
merged = pd.merge(main_df, size_df[['dataset_lower', 'text_size(MiB)']], on='dataset_lower', how='left')

# For missing sizes, try merging on clean dataset name
missing_mask = merged['text_size(MiB)'].isnull()
if missing_mask.any():
    clean_map = size_df.set_index('dataset_clean')['text_size(MiB)'].to_dict()
    merged.loc[missing_mask, 'text_size(MiB)'] = merged.loc[missing_mask, 'dataset_clean'].map(clean_map)

# For any remaining missing sizes (e.g. pitches, statistics.pdf not present in size_df), calculate from input_bytes
# Note: 1 MiB = 1048576 bytes
fallback_mask = merged['text_size(MiB)'].isnull()
merged.loc[fallback_mask, 'text_size(MiB)'] = merged.loc[fallback_mask, 'input_bytes'] / (1024 * 1024)

# 1 MiB = 1.048576 MB (or text_size(MiB) * 1.048576)
merged['text_size_mb'] = merged['text_size(MiB)'] * 1.048576

# Time calculations in seconds
comp_time_s = (merged['gcis_comp_time_ms'] / 1000.0) + merged['irepair_comp_time_s']
decomp_time_s = (merged['gcis_decomp_time_ms'] / 1000.0) + merged['irepair_decomp_time_s']

# Calculate metrics
merged['comp_speed_MBs'] = merged['text_size_mb'] / comp_time_s
merged['decomp_speed_MBs'] = merged['text_size_mb'] / decomp_time_s
merged['peak_mem_comp_MB'] = merged[['gcis_comp_peak_mem_bytes', 'irepair_comp_peak_mem_bytes']].max(axis=1) / 1e6

decomp_cols = [c for c in ['gcis_decomp_peak_mem_bytes', 'irepair_decomp_peak_mem_bytes', 'decompression_peak_bytes', 'gcis_decompression_peak_bytes'] if c in merged.columns]
merged['peak_mem_decomp_MB'] = merged[decomp_cols].max(axis=1) / 1e6

merged['compressed_size_MB'] = merged['total_compressed_bytes'] / 1e6

# Extract output columns
final_df = merged[[
    'dataset',
    'level',
    'text_size_mb',
    'comp_speed_MBs',
    'decomp_speed_MBs',
    'peak_mem_comp_MB',
    'peak_mem_decomp_MB',
    'compressed_size_MB'
]]

final_df.to_csv('gcis_repair_metrics_merged.csv', index=False)
print(f"Total rows: {len(final_df)}")
print(final_df.head(10))