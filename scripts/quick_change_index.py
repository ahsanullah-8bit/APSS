import os
import glob

# --- Assume these variables are defined elsewhere in your script ---
labels_dir = 'License_Plates_bb/labels'
old_class_id = 0
new_class_id = 80
# -----------------------------------------------------------------

label_files = glob.glob(os.path.join(labels_dir, '**', '*.txt'), recursive=True)

print(f'Files selected {label_files.__len__()}')
for file_path in label_files:
    modified_lines = []
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                modified_lines.append("")
                continue

            parts = line.split()
            if len(parts) >= 5:
                try:
                    current_class_id = int(parts[0])
                    if current_class_id == old_class_id:
                        parts[0] = str(new_class_id)
                except ValueError:
                    pass
            modified_lines.append(' '.join(parts))

    with open(file_path, 'w') as f:
        for i, m_line in enumerate(modified_lines):
            f.write(m_line)
            if i < len(modified_lines) - 1:
                f.write('\n')